#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "MainApp.hpp"
#include "HardwareGPIO.hpp"
#include "Settings.hpp"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "main.hpp"
#include "oledui/UICore.hpp"
#include "oledui/UIHome.hpp"
#include "oledui/UIManager.hpp"

#define TAG "MainApp"

#define INIT_RELAY(_gpio) { .gpio = _gpio, .isConnected = false, .isFired = false, .eGeneralState = MainApp::Idle }

MainApp g_app;

void MainApp::Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    HARDWAREGPIO_ClearRelayBus();

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->u32Index = i;
        pSRelay->isConnected = false;
        pSRelay->isFired = false;
        pSRelay->isEN = false;
    }
}

void MainApp::Run()
{
    bool bSanityOn = false;
    TickType_t ttSanityTicks = 0;

    // Wait until it get disarmed before starting the program.
    if (HARDWAREGPIO_ReadMasterPowerSense())
    {
        UIMANAGER_Goto(UIMANAGER_EMENU_ErrorPleaseDisarm);

        while(HARDWAREGPIO_ReadMasterPowerSense())
        {
            HARDWAREGPIO_SetSanityLED(bSanityOn, false);
            bSanityOn = !bSanityOn;
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }

    // Force reset ...
    m_sCmd.eCmd = ECmd::None;
    UIMANAGER_Goto(UIMANAGER_EMENU_Home);

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
                StartFullOutputCalibrationTask();
                break;
            case ECmd::Fire:
                StartFire(sCmd.uArg.sFire);
                break;
            case ECmd::None:
                break;
            default:
                break;
        }

        // Check for disarming condition
        const bool bIsMasterSwitchON = HARDWAREGPIO_ReadMasterPowerSense();

        if (!m_sState.bIsArmed && bIsMasterSwitchON)
        {
            m_sState.bIsArmed = true;
            ESP_LOGI(TAG, "Master switch is armed");
            m_sState.eGeneralState = EGeneralState::Armed;

            UIMANAGER_Goto(UIMANAGER_EMENU_ArmedReady);
        }
        else if (m_sState.bIsArmed && !bIsMasterSwitchON)
        {
            m_sState.bIsArmed = false;
            ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
            m_sState.eGeneralState = EGeneralState::DisarmedMasterSwitchOff;

            UIMANAGER_Goto(UIMANAGER_EMENU_Home);
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - ttSanityTicks) > pdMS_TO_TICKS(m_sState.bIsArmed ? 100 : 500))
        {
            ttSanityTicks = xTaskGetTickCount();
            HARDWAREGPIO_SetSanityLED(bSanityOn, m_sState.bIsArmed);
            bSanityOn = !bSanityOn;
        }

        // Update LEDs
        /* Refresh the strip to send data */
        for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
            UpdateLED(i, false);

        HARDWAREGPIO_RefreshLEDStrip();

        CheckUserInput();

        // Update LEDs
        UIMANAGER_RunTick();

        vTaskDelay(1);
    }
}

bool MainApp::StartCheckConnections()
{
    if (m_xHandle != NULL)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (m_sState.bIsArmed)
    {
        ESP_LOGE(TAG, "Cannot check connection when the system is armed");
        m_sState.eGeneralState = MainApp::EGeneralState::CheckingConnectionError;
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

    pMainApp->m_sState.eGeneralState = MainApp::EGeneralState::CheckingConnection;
    pMainApp->m_sState.dProgressOfOne = 0.0d;

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HARDWAREGPIO_ClearRelayBus();

    uint32_t u32LastAreaIndex = 0;

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[i];

        const uint32_t u32AreaIndex = HARDWAREGPIO_GetRelayArea(i);

        pSRelay->isFired = false;
        pSRelay->isConnected = false;

        // Activate the relay ...
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);
        // Give it some time to detect
        // go to the next one if the return current is detected or wait maximum 200ms
        int ticksMax = 8;
        vTaskDelay(pdMS_TO_TICKS(200));
        do
        {
            pSRelay->isConnected = HARDWAREGPIO_ReadConnectionSense();
            vTaskDelay(pdMS_TO_TICKS(10));
            ticksMax--;
        } while (!pSRelay->isConnected && ticksMax > 0);

        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, false);

        // Give it some time when it change area to be sure no ghost detection happens
        if (u32LastAreaIndex != u32AreaIndex)
        {
            // Ensure capacitor won't keep it on.
            vTaskDelay(pdMS_TO_TICKS(200));
            u32LastAreaIndex = u32AreaIndex;
        }

        pMainApp->m_sState.dProgressOfOne = (double)(i+1)/(double)HWCONFIG_OUTPUT_COUNT;
    }

    ESP_LOGI(TAG, "Check connection completed");
    pMainApp->m_sState.eGeneralState = MainApp::EGeneralState::CheckingConnectionOK;
    HARDWAREGPIO_ClearRelayBus();
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    pMainApp->m_sState.dProgressOfOne = 1.0d;

    pMainApp->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartFire(MainApp::SFire sFire)
{
    if (m_xHandle != NULL)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    if (sFire.u32OutputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        m_sState.eGeneralState = MainApp::EGeneralState::FiringUnknownError;
        return false;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    if (!m_sState.bIsArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.eGeneralState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
        return false;
    }

    ESP_LOGI(TAG, "Fire command issued for output index: %"PRIu32, sFire.u32OutputIndex);

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
    const uint32_t u32OutputIndex = pFireParam->u32OutputIndex;

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    pMainApp->m_sState.eGeneralState = MainApp::EGeneralState::Firing;

    ESP_LOGI(TAG, "Firing in progress: %"PRIu32, u32OutputIndex);

    MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[u32OutputIndex];

    pSRelay->isEN = true;

    // Enable master power
    HARDWAREGPIO_WriteMasterPowerRelay(true);

    ESP_LOGI(TAG, "Firing on output index: %"PRIu32, u32OutputIndex);
    const int32_t s32FireHoldTimeMS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, true);
    vTaskDelay(pdMS_TO_TICKS(s32FireHoldTimeMS));
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, false);

    pSRelay->isEN = false;
    pSRelay->isFired = true;

    pMainApp->m_sState.eGeneralState = MainApp::EGeneralState::FiringOK;
    ESP_LOGI(TAG, "Firing is done");
    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    free((void*)pFireParam);
    pMainApp->m_xHandle = NULL;
    vTaskDelete(NULL);
}

bool MainApp::StartFullOutputCalibrationTask()
{
    if (m_xHandle != NULL)
    {
        ESP_LOGE(TAG, "Already doing a job");
        return false;
    }

    // Should't be able to works when the power is present
    if (m_sState.bIsArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.eGeneralState = MainApp::EGeneralState::FiringMasterSwitchWrongStateError;
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
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HARDWAREGPIO_ClearRelayBus();

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MainApp::SRelay* pSRelay = &pMainApp->m_sOutputs[i];

        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);
        // Ensure the jumper is not there yet
        bool bIsDetect = HARDWAREGPIO_ReadConnectionSense();
        while(bIsDetect)
        {
            ESP_LOGI(TAG, "Waiting for jumper on output %" PRIu32 " to get disconnected ... ", pSRelay->u32Index+1);
            vTaskDelay(pdMS_TO_TICKS(250));
            bIsDetect = HARDWAREGPIO_ReadConnectionSense();
        }
        ESP_LOGI(TAG, "Jumper is disconnected");
        // Give some time to insert it
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, false);

        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t u32Count = 0;
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);

        // Ensure the jumper is connected
        bool bIsDetect2 = HARDWAREGPIO_ReadConnectionSense();
        while(!bIsDetect2)
        {
            u32Count++;
            vTaskDelay(1);
            bIsDetect2 = HARDWAREGPIO_ReadConnectionSense();
        }

        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, false);

        ESP_LOGI(TAG, "Output %" PRIu32 ", count: %" PRIu32 " ms", pSRelay->u32Index+1, (uint32_t)pdTICKS_TO_MS(u32Count));
    }

    // Ensure the power if really off
    HARDWAREGPIO_ClearRelayBus();
    HARDWAREGPIO_WriteMasterPowerRelay(false);

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

void MainApp::ExecFullOutputCalibration()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::OutputCalib };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::ExecFire(uint32_t u32OutputIndex)
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MainApp::SCmd sCmd = { .eCmd = MainApp::ECmd::Fire, .uArg = { .sFire = { .u32OutputIndex = u32OutputIndex } } };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MainApp::UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh)
{
    MainApp::SRelay* pSRelay = &m_sOutputs[u32OutputIndex];

    MainApp::EOutputState eOutputState = GetOutputState(pSRelay);
    if (eOutputState == MainApp::EOutputState::Enabled)
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 0, 200, 0);
    else if (eOutputState == MainApp::EOutputState::Fired) // White for fired
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 100, 100, 0);
    else if (eOutputState == MainApp::EOutputState::Connected) // YELLOW for connected
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 200, 200, 0);
    else // minimal white illuminiation
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 10, 10, 10);

    if (bForceRefresh)
        HARDWAREGPIO_RefreshLEDStrip();
}

void MainApp::CheckUserInput()
{
    // Encoder move
    static TickType_t ttEncoderSwitchTicks = 0;
    if ( !HARDWAREGPIO_IsEncoderSwitchON() && ttEncoderSwitchTicks != 0 )
    {
        // 100 ms check maximum ...
        if ( (xTaskGetTickCount() - ttEncoderSwitchTicks) > pdMS_TO_TICKS(100) )
        {
            UIMANAGER_EncoderMove(UICORE_EBTNEVENT_Click, 0);
        }
        ttEncoderSwitchTicks = 0;
    }
    else if (HARDWAREGPIO_IsEncoderSwitchON() && ttEncoderSwitchTicks == 0)
    {
        ttEncoderSwitchTicks = xTaskGetTickCount();
    }

    const int32_t s32Count = HARDWAREGPIO_GetEncoderCount();
    if (s32Count != 0)
        UIMANAGER_EncoderMove(UICORE_EBTNEVENT_EncoderClick, s32Count);
}

MainApp::SRelay MainApp::GetRelayState(uint32_t u32OutputIndex)
{
    return m_sOutputs[u32OutputIndex];
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
    return m_sState.bIsArmed;
}

MainApp::EGeneralState MainApp::GetGeneralState()
{
    return m_sState.eGeneralState;
}

double MainApp::GetProgress()
{
    return m_sState.dProgressOfOne;
}