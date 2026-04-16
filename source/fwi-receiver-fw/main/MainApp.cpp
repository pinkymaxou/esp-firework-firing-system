#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "MainApp.hpp"
#include "HWGPIO.hpp"
#include "Settings.hpp"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "main.hpp"
#include "oledui/UIBase.hpp"
#include "oledui/UIHome.hpp"
#include "oledui/UIManager.hpp"

#define TAG "MainApp"


MainApp g_app;

void MainApp::Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );

    HWGPIO::writeMasterPowerRelay(false);
    HWGPIO::clearRelayBus();

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* relay = &m_outputs[i];
        relay->index = i;
        relay->isConnected = false;
        relay->isFired = false;
        relay->isEN = false;
    }
}

void MainApp::Run()
{
    bool sanity_on = false;
    TickType_t sanity_ticks = 0;

    // Wait until it get disarmed before starting the program.
    if (HWGPIO::readMasterPowerSense())
    {
        g_uiMgr.Goto(UIManager::EMenu::ErrorPleaseDisarm);

        while(HWGPIO::readMasterPowerSense())
        {
            HWGPIO::setSanityLED(sanity_on, false);
            sanity_on = !sanity_on;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }

    // Force reset ...
    ESP_LOGI(TAG, "Going to home");
    m_cmd.cmd = ECmd::None;
    g_uiMgr.Goto(UIManager::EMenu::Home);

    while (true)
    {
        xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
        const SCmd cmd = m_cmd;
        m_cmd.cmd = ECmd::None;
        xSemaphoreGive(m_xSemaphoreHandle);

        // Command from another thread
        switch (cmd.cmd)
        {
            case ECmd::TestConnections:
                // Cannot test connection while armed
                StartTestConnections();
                break;
            case ECmd::OutputCalib:
                StartFullOutputCalibration();
                break;
            case ECmd::Fire:
                StartFire(cmd.arg.fire);
                break;
            case ECmd::LiveCheckContinuity:
                StartLiveCheckContinuity();
                break;
            case ECmd::None:
                break;
            default:
                break;
        }

        // Check for disarming condition
        const bool is_master_switch_on = HWGPIO::readMasterPowerSense();

        if (!m_state.isArmed && is_master_switch_on)
        {
            m_state.isArmed = true;
            ESP_LOGI(TAG, "Master switch is armed");
            m_state.generalState = EGeneralState::Armed;

            g_uiMgr.Goto(UIManager::EMenu::ArmedReady);
        }
        else if (m_state.isArmed && !is_master_switch_on)
        {
            m_state.isArmed = false;
            ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
            m_state.generalState = EGeneralState::DisarmedMasterSwitchOff;

            g_uiMgr.Goto(UIManager::EMenu::Home);
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - sanity_ticks) > pdMS_TO_TICKS(m_state.isArmed ? 250 : 500))
        {
            sanity_ticks = xTaskGetTickCount();
            HWGPIO::setSanityLED(sanity_on, m_state.isArmed);
            sanity_on = !sanity_on;
        }

        // Update LEDs
        /* Refresh the strip to send data */
        for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
            UpdateLED(i, false);

        HWGPIO::refreshLEDStrip();

        CheckUserInput();

        // Update LEDs
        g_uiMgr.RunTick();

        vTaskDelay(1);
    }
}

bool MainApp::StartTestConnections()
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (m_state.isArmed)
    {
        ESP_LOGE(TAG, "Cannot test connection when the system is armed");
        m_state.generalState = MainApp::EGeneralState::CheckingConnectionError;
        return false;
    }

    ESP_LOGI(TAG, "Testing connections ...");

    /* Create the task, storing the handle. */
    const BaseType_t x_returned = xTaskCreate(
        TestConnectionsTask,    /* Function that implements the task. */
        "TestConnections",      /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,           /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */

    assert(pdPASS == x_returned);
    return true;
}

void MainApp::TestConnectionsTask(void* param)
{
    MainApp* main_app = (MainApp*)param;

    main_app->m_isOperationCancelled = false;

    main_app->m_state.generalState = MainApp::EGeneralState::CheckingConnection;
    main_app->m_state.progressOfOne = 0.0d;

    // Master power relay shouln'd be active during check
    HWGPIO::writeMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO::clearRelayBus();

    uint32_t last_area_index = 0;

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* relay = &main_app->m_outputs[i];

        const uint32_t area_index = HWGPIO::getRelayArea(i);

        relay->isFired = false;
        relay->isConnected = false;

        const bool is_master_switch_on = HWGPIO::readMasterPowerSense();
        if (is_master_switch_on)
        {
            // Abort is the master switch is activated
            break;
        }

        if (main_app->m_isOperationCancelled)
        {
            break;
        }

        // Activate the relay ...
        HWGPIO::writeSingleRelay(relay->index, true);
        // Give it some time to detect
        // go to the next one if the return current is detected or wait maximum 200ms
        int ticks_max = 8;
        vTaskDelay(pdMS_TO_TICKS(200));
        do
        {
            relay->isConnected = HWGPIO::readConnectionSense();
            vTaskDelay(pdMS_TO_TICKS(10));
            ticks_max--;
        } while (!relay->isConnected && ticks_max > 0);

        HWGPIO::writeSingleRelay(relay->index, false);

        // Give it some time when it change area to be sure no ghost detection happens
        if (last_area_index != area_index)
        {
            // Ensure capacitor won't keep it on.
            vTaskDelay(pdMS_TO_TICKS(200));
            last_area_index = area_index;
        }

        main_app->m_state.progressOfOne = (double)(i+1)/(double)HWCONFIG_OUTPUT_COUNT;
    }

    ESP_LOGI(TAG, "Test connections completed");
    main_app->m_state.generalState = MainApp::EGeneralState::CheckingConnectionOK;
    HWGPIO::clearRelayBus();
    HWGPIO::writeMasterPowerRelay(false);

    main_app->m_state.progressOfOne = 1.0d;

    main_app->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartFire(MainApp::SFire fire)
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (fire.outputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        m_state.generalState = MainApp::EGeneralState::FiringUnknownError;
        return false;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    if (!m_state.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_state.generalState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
        return false;
    }

    ESP_LOGI(TAG, "Fire command issued for output index: %" PRIu32, fire.outputIndex);

    MainApp::SFire* fire_param = (MainApp::SFire*)malloc(sizeof(MainApp::SFire));
    *fire_param = fire;

    /* Create the task, storing the handle. */
    const BaseType_t x_returned = xTaskCreate(
        FireTask,               /* Function that implements the task. */
        "Fire",                 /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )fire_param,   /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(pdPASS == x_returned);
    return true;
}

void MainApp::FireTask(void* param)
{
    MainApp* main_app = (MainApp*)&g_app;

    const MainApp::SFire* fire_param = (const MainApp::SFire*)param;
    const uint32_t output_index = fire_param->outputIndex;

    HWGPIO::writeMasterPowerRelay(false);
    main_app->m_state.generalState = MainApp::EGeneralState::Firing;

    ESP_LOGI(TAG, "Firing in progress: %" PRIu32, output_index);

    MainApp::SRelay* relay = &main_app->m_outputs[output_index];

    relay->isEN = true;

    // Enable master power
    HWGPIO::writeMasterPowerRelay(true);

    ESP_LOGI(TAG, "Firing on output index: %" PRIu32, output_index);
    const int32_t fire_hold_ms = NVSJSON_GetValueInt32(&Settings::g_handle, Settings::FiringHoldTimeMS);
    HWGPIO::writeSingleRelay(output_index, true);
    vTaskDelay(pdMS_TO_TICKS(fire_hold_ms));
    HWGPIO::writeSingleRelay(output_index, false);

    relay->isEN = false;
    relay->isFired = true;

    main_app->m_state.generalState = MainApp::EGeneralState::FiringOK;
    ESP_LOGI(TAG, "Firing is done");
    // Master power relay shouln'd be active during check
    HWGPIO::writeMasterPowerRelay(false);

    free((void*)fire_param);
    main_app->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartLiveCheckContinuity()
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    // Should't be able to works when the power is present
    if (m_state.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_state.generalState = MainApp::EGeneralState::LiveCheckContinuity;
        return false;
    }

    ESP_LOGI(TAG, "Live check continuity command issued");

    /* Create the task, storing the handle. */
    const BaseType_t x_returned = xTaskCreate(
        LiveCheckContinuityTask,/* Function that implements the task. */
        "LiveCheckCont",    /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(pdPASS == x_returned);
    return true;
}

void MainApp::LiveCheckContinuityTask(void* param)
{
    MainApp* main_app = (MainApp*)&g_app;

    // Master power relay shouln'd be active during check
    HWGPIO::writeMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO::clearRelayBus();

    main_app->m_isOperationCancelled = false;
    main_app->m_state.generalState = MainApp::EGeneralState::LiveCheckContinuity;

    MainApp::SRelay* relay = &main_app->m_outputs[0];
    HWGPIO::writeSingleRelay(relay->index, true);

    do
    {
        const bool is_master_switch_on = HWGPIO::readMasterPowerSense();
        if (is_master_switch_on)
        {
            break; // Cancel if the switch is on
        }

        main_app->m_state.isContinuityCheckOK = HWGPIO::readConnectionSense();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    while(!main_app->m_isOperationCancelled);

    HWGPIO::writeSingleRelay(relay->index, false);

    main_app->m_state.generalState = MainApp::EGeneralState::Idle;
    main_app->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartFullOutputCalibration()
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    // Should't be able to works when the power is present
    if (m_state.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_state.generalState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
        return false;
    }

    ESP_LOGI(TAG, "Ouput calibration command issued");

    /* Create the task, storing the handle. */
    const BaseType_t x_returned = xTaskCreate(
        FullOutputCalibrationTask,/* Function that implements the task. */
        "OutputCalibration",    /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(pdPASS == x_returned);
    return true;
}

void MainApp::FullOutputCalibrationTask(void* param)
{
    MainApp* main_app = (MainApp*)param;

    // Master power relay shouln'd be active during check
    HWGPIO::writeMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO::clearRelayBus();

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* relay = &main_app->m_outputs[i];

        HWGPIO::writeSingleRelay(relay->index, true);
        // Ensure the jumper is not there yet
        bool is_detect = HWGPIO::readConnectionSense();
        while(is_detect)
        {
            ESP_LOGI(TAG, "Waiting for jumper on output %" PRIu32 " to get disconnected ... ", relay->index+1);
            vTaskDelay(pdMS_TO_TICKS(250));
            is_detect = HWGPIO::readConnectionSense();
        }
        ESP_LOGI(TAG, "Jumper is disconnected");
        // Give some time to insert it
        HWGPIO::writeSingleRelay(relay->index, false);

        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t count = 0;
        HWGPIO::writeSingleRelay(relay->index, true);

        // Ensure the jumper is connected
        bool is_detect2 = HWGPIO::readConnectionSense();
        while(!is_detect2)
        {
            count++;
            vTaskDelay(1);
            is_detect2 = HWGPIO::readConnectionSense();
        }

        HWGPIO::writeSingleRelay(relay->index, false);

        ESP_LOGI(TAG, "Output %" PRIu32 ", count: %" PRIu32 " ms", relay->index+1, (uint32_t)pdTICKS_TO_MS(count));
    }

    // Ensure the power if really off
    HWGPIO::clearRelayBus();
    HWGPIO::writeMasterPowerRelay(false);

    main_app->m_xHandle = NULL;
    vTaskDelete(NULL);
}

void MainApp::ExecTestConnections()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd cmd = { .cmd = MainApp::ECmd::TestConnections };
    m_cmd = cmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecLiveCheckContinuity()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd cmd = { .cmd = MainApp::ECmd::LiveCheckContinuity };
    m_cmd = cmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecFullOutputCalibration()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd cmd = { .cmd = MainApp::ECmd::OutputCalib };
    m_cmd = cmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecFire(uint32_t output_index)
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd cmd = { .cmd = MainApp::ECmd::Fire, .arg = { .fire = { .outputIndex = output_index } } };
    m_cmd = cmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecCancel()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    m_isOperationCancelled = true;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::UpdateLED(uint32_t output_index, bool force_refresh)
{
    MainApp::SRelay* relay = &m_outputs[output_index];

    const MainApp::EOutputState output_state = GetOutputState(relay);
    if (MainApp::EOutputState::Enabled == output_state)
        HWGPIO::setOutputRelayStatusColor(output_index, 0, 200, 0);
    else if (MainApp::EOutputState::Fired == output_state) // White for fired
        HWGPIO::setOutputRelayStatusColor(output_index, 100, 100, 0);
    else if (MainApp::EOutputState::Connected == output_state) // YELLOW for connected
        HWGPIO::setOutputRelayStatusColor(output_index, 200, 200, 0);
    else // minimal white illuminiation
        HWGPIO::setOutputRelayStatusColor(output_index, 10, 10, 10);

    if (force_refresh)
        HWGPIO::refreshLEDStrip();
}

void MainApp::CheckUserInput()
{
    // Encoder move
    static TickType_t encoder_switch_ticks = 0;
    if ( !HWGPIO::isEncoderSwitchON() && encoder_switch_ticks != 0 )
    {
        // 100 ms check maximum ...
        if ( (xTaskGetTickCount() - encoder_switch_ticks) > pdMS_TO_TICKS(100) )
        {
            g_uiMgr.EncoderMove(UIBase::BTEvent::Click, 0);
        }
        encoder_switch_ticks = 0;
    }
    else if (HWGPIO::isEncoderSwitchON() && 0 == encoder_switch_ticks)
    {
        encoder_switch_ticks = xTaskGetTickCount();
    }

    const int32_t count = HWGPIO::getEncoderCount();
    if (0 != count)
        g_uiMgr.EncoderMove(UIBase::BTEvent::EncoderClick, count);
}

MainApp::SRelay MainApp::GetRelayState(uint32_t output_index)
{
    return m_outputs[output_index];
}

MainApp::EOutputState MainApp::GetOutputState(const MainApp::SRelay* relay)
{
    if (relay->isEN)
        return MainApp::EOutputState::Enabled;
    if (relay->isFired) // White for fired
        return MainApp::EOutputState::Fired;
    if (relay->isConnected) // YELLOW for connected
        return MainApp::EOutputState::Connected;
    return MainApp::EOutputState::Idle;
}

bool MainApp::IsArmed()
{
    return m_state.isArmed;
}

MainApp::EGeneralState MainApp::GetGeneralState()
{
    return m_state.generalState;
}

bool MainApp::GetContinuityTest()
{
    return m_state.isContinuityCheckOK;
}

double MainApp::GetProgress()
{
    return m_state.progressOfOne;
}
