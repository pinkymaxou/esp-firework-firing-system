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

    HWGPIO_WriteMasterPowerRelay(false);
    HWGPIO_ClearRelayBus();

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->index = i;
        pSRelay->isConnected = false;
        pSRelay->isFired = false;
        pSRelay->isEN = false;
    }
}

void MainApp::Run()
{
    bool sanity_on = false;
    TickType_t ttSanityTicks = 0;

    // Wait until it get disarmed before starting the program.
    if (HWGPIO_ReadMasterPowerSense())
    {
        g_uiMgr.Goto(UIManager::EMenu::ErrorPleaseDisarm);

        while(HWGPIO_ReadMasterPowerSense())
        {
            HWGPIO_SetSanityLED(sanity_on, false);
            sanity_on = !sanity_on;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }

    // Force reset ...
    ESP_LOGI(TAG, "Going to home");
    m_sCmd.eCmd = ECmd::None;
    g_uiMgr.Goto(UIManager::EMenu::Home);

    while (true)
    {
        xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
        SCmd sCmd = m_sCmd;
        m_sCmd.eCmd = ECmd::None;
        xSemaphoreGive(m_xSemaphoreHandle);

        // Command from another thread
        switch (sCmd.eCmd)
        {
            case ECmd::CheckConnections:
                // Cannot check connection while armed
                StartCheckConnections();
                break;
            case ECmd::OutputCalib:
                StartFullOutputCalibration();
                break;
            case ECmd::Fire:
                StartFire(sCmd.uArg.sFire);
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
        const bool is_master_switch_on = HWGPIO_ReadMasterPowerSense();

        if (!m_sState.isArmed && is_master_switch_on)
        {
            m_sState.isArmed = true;
            ESP_LOGI(TAG, "Master switch is armed");
            m_sState.generalState = EGeneralState::Armed;

            g_uiMgr.Goto(UIManager::EMenu::ArmedReady);
        }
        else if (m_sState.isArmed && !is_master_switch_on)
        {
            m_sState.isArmed = false;
            ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
            m_sState.generalState = EGeneralState::DisarmedMasterSwitchOff;

            g_uiMgr.Goto(UIManager::EMenu::Home);
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - ttSanityTicks) > pdMS_TO_TICKS(m_sState.isArmed ? 100 : 500))
        {
            ttSanityTicks = xTaskGetTickCount();
            HWGPIO_SetSanityLED(sanity_on, m_sState.isArmed);
            sanity_on = !sanity_on;
        }

        // Update LEDs
        /* Refresh the strip to send data */
        for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
            UpdateLED(i, false);

        HWGPIO_RefreshLEDStrip();

        CheckUserInput();

        // Update LEDs
        g_uiMgr.RunTick();

        vTaskDelay(1);
    }
}

bool MainApp::StartCheckConnections()
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (m_sState.isArmed)
    {
        ESP_LOGE(TAG, "Cannot check connection when the system is armed");
        m_sState.generalState = MainApp::EGeneralState::CheckingConnectionError;
        return false;
    }

    ESP_LOGI(TAG, "Checking connections ...");

    /* Create the task, storing the handle. */
    const BaseType_t xReturned = xTaskCreate(
        CheckConnectionsTask,   /* Function that implements the task. */
        "CheckConnections",     /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,           /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */

    assert(xReturned == pdPASS);
    return true;
}

void MainApp::CheckConnectionsTask(void* pParam)
{
    MainApp* pMainApp = (MainApp*)pParam;

    pMainApp->m_isOperationCancelled = false;

    pMainApp->m_sState.generalState = MainApp::EGeneralState::CheckingConnection;
    pMainApp->m_sState.progressOfOne = 0.0d;

    // Master power relay shouln'd be active during check
    HWGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO_ClearRelayBus();

    uint32_t last_area_index = 0;

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[i];

        const uint32_t area_index = HWGPIO_GetRelayArea(i);

        pSRelay->isFired = false;
        pSRelay->isConnected = false;

        const bool is_master_switch_on = HWGPIO_ReadMasterPowerSense();
        if (is_master_switch_on)
        {
            // Abort is the master switch is activated
            break;
        }

        if (pMainApp->m_isOperationCancelled)
        {
            break;
        }

        // Activate the relay ...
        HWGPIO_WriteSingleRelay(pSRelay->index, true);
        // Give it some time to detect
        // go to the next one if the return current is detected or wait maximum 200ms
        int ticksMax = 8;
        vTaskDelay(pdMS_TO_TICKS(200));
        do
        {
            pSRelay->isConnected = HWGPIO_ReadConnectionSense();
            vTaskDelay(pdMS_TO_TICKS(10));
            ticksMax--;
        } while (!pSRelay->isConnected && ticksMax > 0);

        HWGPIO_WriteSingleRelay(pSRelay->index, false);

        // Give it some time when it change area to be sure no ghost detection happens
        if (last_area_index != area_index)
        {
            // Ensure capacitor won't keep it on.
            vTaskDelay(pdMS_TO_TICKS(200));
            last_area_index = area_index;
        }

        pMainApp->m_sState.progressOfOne = (double)(i+1)/(double)HWCONFIG_OUTPUT_COUNT;
    }

    ESP_LOGI(TAG, "Check connection completed");
    pMainApp->m_sState.generalState = MainApp::EGeneralState::CheckingConnectionOK;
    HWGPIO_ClearRelayBus();
    HWGPIO_WriteMasterPowerRelay(false);

    pMainApp->m_sState.progressOfOne = 1.0d;

    pMainApp->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartFire(MainApp::SFire sFire)
{
    if (NULL != m_xHandle)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (sFire.outputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        m_sState.generalState = MainApp::EGeneralState::FiringUnknownError;
        return false;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    if (!m_sState.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.generalState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
        return false;
    }

    ESP_LOGI(TAG, "Fire command issued for output index: %" PRIu32, sFire.outputIndex);

    MainApp::SFire* pCopyFire = (MainApp::SFire*)malloc(sizeof(MainApp::SFire));
    *pCopyFire = sFire;

    /* Create the task, storing the handle. */
    const BaseType_t xReturned = xTaskCreate(
        FireTask,               /* Function that implements the task. */
        "Fire",                 /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )pCopyFire,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(xReturned == pdPASS);
    return true;
}

void MainApp::FireTask(void* pParam)
{
    MainApp* pMainApp = (MainApp*)&g_app;

    const MainApp::SFire* pFireParam = (const MainApp::SFire*)pParam;
    const uint32_t output_index = pFireParam->outputIndex;

    HWGPIO_WriteMasterPowerRelay(false);
    pMainApp->m_sState.generalState = MainApp::EGeneralState::Firing;

    ESP_LOGI(TAG, "Firing in progress: %" PRIu32, output_index);

    MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[output_index];

    pSRelay->isEN = true;

    // Enable master power
    HWGPIO_WriteMasterPowerRelay(true);

    ESP_LOGI(TAG, "Firing on output index: %" PRIu32, output_index);
    const int32_t fire_hold_ms = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    HWGPIO_WriteSingleRelay(output_index, true);
    vTaskDelay(pdMS_TO_TICKS(fire_hold_ms));
    HWGPIO_WriteSingleRelay(output_index, false);

    pSRelay->isEN = false;
    pSRelay->isFired = true;

    pMainApp->m_sState.generalState = MainApp::EGeneralState::FiringOK;
    ESP_LOGI(TAG, "Firing is done");
    // Master power relay shouln'd be active during check
    HWGPIO_WriteMasterPowerRelay(false);

    free((void*)pFireParam);
    pMainApp->m_xHandle = NULL;
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
    if (m_sState.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.generalState = MainApp::EGeneralState::LiveCheckContinuity;
        return false;
    }

    ESP_LOGI(TAG, "Live check continuity command issued");

    /* Create the task, storing the handle. */
    const BaseType_t xReturned = xTaskCreate(
        LiveCheckContinuityTask,/* Function that implements the task. */
        "LiveCheckCont",    /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(xReturned == pdPASS);
    return true;
}

void MainApp::LiveCheckContinuityTask(void* pParam)
{
    MainApp* pMainApp = (MainApp*)&g_app;

    // Master power relay shouln'd be active during check
    HWGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO_ClearRelayBus();

    pMainApp->m_isOperationCancelled = false;
    pMainApp->m_sState.generalState = MainApp::EGeneralState::LiveCheckContinuity;

    MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[0];
    HWGPIO_WriteSingleRelay(pSRelay->index, true);

    do
    {
        const bool is_master_switch_on = HWGPIO_ReadMasterPowerSense();
        if (is_master_switch_on)
        {
            break; // Cancel if the switch is on
        }

        pMainApp->m_sState.isContinuityCheckOK = HWGPIO_ReadConnectionSense();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    while(!pMainApp->m_isOperationCancelled);

    HWGPIO_WriteSingleRelay(pSRelay->index, false);

    pMainApp->m_sState.generalState = MainApp::EGeneralState::Idle;
    pMainApp->m_xHandle = NULL;
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
    if (m_sState.isArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.generalState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
        return false;
    }

    ESP_LOGI(TAG, "Ouput calibration command issued");

    /* Create the task, storing the handle. */
    const BaseType_t xReturned = xTaskCreate(
        FullOutputCalibrationTask,/* Function that implements the task. */
        "OutputCalibration",    /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )this,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */
    assert(xReturned == pdPASS);
    return true;
}

void MainApp::FullOutputCalibrationTask(void* pParam)
{
    MainApp* pMainApp = (MainApp*)pParam;

    // Master power relay shouln'd be active during check
    HWGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HWGPIO_ClearRelayBus();

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[i];

        HWGPIO_WriteSingleRelay(pSRelay->index, true);
        // Ensure the jumper is not there yet
        bool is_detect = HWGPIO_ReadConnectionSense();
        while(is_detect)
        {
            ESP_LOGI(TAG, "Waiting for jumper on output %" PRIu32 " to get disconnected ... ", pSRelay->index+1);
            vTaskDelay(pdMS_TO_TICKS(250));
            is_detect = HWGPIO_ReadConnectionSense();
        }
        ESP_LOGI(TAG, "Jumper is disconnected");
        // Give some time to insert it
        HWGPIO_WriteSingleRelay(pSRelay->index, false);

        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t count = 0;
        HWGPIO_WriteSingleRelay(pSRelay->index, true);

        // Ensure the jumper is connected
        bool is_detect2 = HWGPIO_ReadConnectionSense();
        while(!is_detect2)
        {
            count++;
            vTaskDelay(1);
            is_detect2 = HWGPIO_ReadConnectionSense();
        }

        HWGPIO_WriteSingleRelay(pSRelay->index, false);

        ESP_LOGI(TAG, "Output %" PRIu32 ", count: %" PRIu32 " ms", pSRelay->index+1, (uint32_t)pdTICKS_TO_MS(count));
    }

    // Ensure the power if really off
    HWGPIO_ClearRelayBus();
    HWGPIO_WriteMasterPowerRelay(false);

    pMainApp->m_xHandle = NULL;
    vTaskDelete(NULL);
}

void MainApp::ExecCheckConnections()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::CheckConnections };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecLiveCheckContinuity()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::LiveCheckContinuity };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecFullOutputCalibration()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::OutputCalib };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecFire(uint32_t output_index)
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::Fire, .uArg = { .sFire = { .outputIndex = output_index } } };
    m_sCmd = sCmd;
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
    MainApp::SRelay* pSRelay = &m_sOutputs[output_index];

    MainApp::EOutputState eOutputState = GetOutputState(pSRelay);
    if (MainApp::EOutputState::Enabled == eOutputState)
        HWGPIO_SetOutputRelayStatusColor(output_index, 0, 200, 0);
    else if (MainApp::EOutputState::Fired == eOutputState) // White for fired
        HWGPIO_SetOutputRelayStatusColor(output_index, 100, 100, 0);
    else if (MainApp::EOutputState::Connected == eOutputState) // YELLOW for connected
        HWGPIO_SetOutputRelayStatusColor(output_index, 200, 200, 0);
    else // minimal white illuminiation
        HWGPIO_SetOutputRelayStatusColor(output_index, 10, 10, 10);

    if (force_refresh)
        HWGPIO_RefreshLEDStrip();
}

void MainApp::CheckUserInput()
{
    // Encoder move
    static TickType_t ttEncoderSwitchTicks = 0;
    if ( !HWGPIO_IsEncoderSwitchON() && ttEncoderSwitchTicks != 0 )
    {
        // 100 ms check maximum ...
        if ( (xTaskGetTickCount() - ttEncoderSwitchTicks) > pdMS_TO_TICKS(100) )
        {
            g_uiMgr.EncoderMove(UIBase::BTEvent::Click, 0);
        }
        ttEncoderSwitchTicks = 0;
    }
    else if (HWGPIO_IsEncoderSwitchON() && ttEncoderSwitchTicks == 0)
    {
        ttEncoderSwitchTicks = xTaskGetTickCount();
    }

    const int32_t count = HWGPIO_GetEncoderCount();
    if (0 != count)
        g_uiMgr.EncoderMove(UIBase::BTEvent::EncoderClick, count);
}

MainApp::SRelay MainApp::GetRelayState(uint32_t output_index)
{
    return m_sOutputs[output_index];
}

MainApp::EOutputState MainApp::GetOutputState(const MainApp::SRelay* pSRelay)
{
    if (pSRelay->isEN)
        return MainApp::EOutputState::Enabled;
    if (pSRelay->isFired) // White for fired
        return MainApp::EOutputState::Fired;
    if (pSRelay->isConnected) // YELLOW for connected
        return MainApp::EOutputState::Connected;
    return MainApp::EOutputState::Idle;
}

bool MainApp::IsArmed()
{
    return m_sState.isArmed;
}

MainApp::EGeneralState MainApp::GetGeneralState()
{
    return m_sState.generalState;
}

bool MainApp::GetContinuityTest()
{
    return m_sState.isContinuityCheckOK;
}

double MainApp::GetProgress()
{
    return m_sState.progressOfOne;
}
