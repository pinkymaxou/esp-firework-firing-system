#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "MainApp.h"
#include "HWConfig.h"
#include "HardwareGPIO.h"
#include "Settings.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "main.h"
#include "ui/UICore.h"
#include "ui/UIHome.h"
#include "ui/UIManager.h"

#define TAG "MainApp"

typedef struct
{
    bool bIsArmed;
    // TickType_t ttArmedTicks;

    MAINAPP_EGENERALSTATE eGeneralState;
} SState;

#define INIT_RELAY(_gpio) { .gpio = _gpio, .isConnected = false, .isFired = false, .eGeneralState = MAINAPP_EGENERALSTATE_Idle }

static MAINAPP_SRelay m_sOutputs[HWCONFIG_OUTPUT_COUNT];
static SState m_sState = { .bIsArmed = false/*, .ttArmedTicks = 0*/ };
static bool m_bIsRefreshOLED = true;

// static int32_t m_s32AutodisarmTimeoutMin = 0;

// Input commands
static MAINAPP_SCmd m_sCmd = { .eCmd = MAINAPP_ECMD_None };

// Semaphore
static StaticSemaphore_t m_xSemaphoreCreateMutex;
static SemaphoreHandle_t m_xSemaphoreHandle;

// Launching related tasks
static TaskHandle_t m_xHandle = NULL;

// Firing related
static bool StartCheckConnections();
static void CheckConnectionsTask(void* pParam);

static bool StartFire(MAINAPP_SFire sFire);
static void FireTask(void* pParam);

static void UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh);
static void UpdateOLED();

void MAINAPP_Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    HARDWAREGPIO_ClearRelayBus();

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MAINAPP_SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->u32Index = i;
        pSRelay->isConnected = false;
        pSRelay->isFired = false;
        pSRelay->isEN = false;
    }

    UpdateOLED();
}

void MAINAPP_Run()
{
    bool bSanityOn = false;
    TickType_t ttSanityTicks = 0;
    TickType_t ttUpdateOLEDTick = 0;

    UIMANAGER_Goto(UIMANAGER_EMENU_Home);

   // m_s32AutodisarmTimeoutMin = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AutoDisarmTimeout);

    while (true)
    {
        xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
        MAINAPP_SCmd sCmd = m_sCmd;
        m_sCmd.eCmd = MAINAPP_ECMD_None;
        xSemaphoreGive(m_xSemaphoreHandle);

        // Command from another thread
        if (sCmd.eCmd != MAINAPP_ECMD_None)
        {
            switch (sCmd.eCmd)
            {
                case MAINAPP_ECMD_CheckConnections:
                    // Cannot check connection while armed
                    StartCheckConnections();
                    break;
                case MAINAPP_ECMD_Fire:
                    StartFire(sCmd.uArg.sFire);

                    // Reset armed timeout ..
                    //if (m_sState.bIsArmed)
                    //    m_sState.ttArmedTicks = xTaskGetTickCount();
                    break;
                default:
                    break;
            }
        }

        // Check for disarming condition
        const bool bIsMasterSwitchON = HARDWAREGPIO_ReadMasterPowerSense();

        if (!m_sState.bIsArmed && bIsMasterSwitchON)
        {
            m_sState.bIsArmed = true;
            ESP_LOGI(TAG, "Master switch is armed");
            m_sState.eGeneralState = MAINAPP_EGENERALSTATE_Armed;

            m_bIsRefreshOLED = true;
        }
        else if (m_sState.bIsArmed && !bIsMasterSwitchON)
        {
            m_sState.bIsArmed = false;
            ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
            m_sState.eGeneralState = MAINAPP_EGENERALSTATE_DisarmedMasterSwitchOff;

            m_bIsRefreshOLED = true;
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - ttSanityTicks) > pdMS_TO_TICKS(m_sState.bIsArmed ? 50 : 500))
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

        // Update LEDs
        if ( (xTaskGetTickCount() - ttUpdateOLEDTick) > pdMS_TO_TICKS(1000) )
        {
            if (HARDWAREGPIO_IsEncoderSwitchON())
                ESP_LOGI(TAG, "IsEncoderSwitchON");

            m_bIsRefreshOLED = true;
            ttUpdateOLEDTick = xTaskGetTickCount();
        }

        if (m_bIsRefreshOLED)
        {
            m_bIsRefreshOLED = false;
            UpdateOLED();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static bool StartCheckConnections()
{
    if (m_xHandle != NULL)
    {
        ESP_LOGE(TAG, "Already doing a job");
        goto ERROR;
    }

    if (m_sState.bIsArmed)
    {
        ESP_LOGE(TAG, "Cannot check connection when the system is armed");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_CheckingConnectionError;
        goto ERROR;
    }

    ESP_LOGI(TAG, "Checking connections ...");

    /* Create the task, storing the handle. */
    const BaseType_t xReturned = xTaskCreate(
        CheckConnectionsTask,   /* Function that implements the task. */
        "CheckConnections",     /* Text name for the task. */
        4096,                   /* Stack size in words, not bytes. */
        ( void * )NULL,           /* Parameter passed into the task. */
        tskIDLE_PRIORITY+10,    /* Priority at which the task is created. */
        &m_xHandle );           /* Used to pass out the created task's handle. */

    assert(xReturned == pdPASS);
    return true;
    ERROR:
    return false;
}

static void CheckConnectionsTask(void* pParam)
{
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_CheckingConnection;

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HARDWAREGPIO_ClearRelayBus();

    uint32_t u32LastAreaIndex = 0;

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MAINAPP_SRelay* pSRelay = &m_sOutputs[i];

        const uint32_t u32AreaIndex = i / HWCONFIG_OUTPUTBUS_COUNT;

        pSRelay->isFired = false;
        pSRelay->isConnected = false;

        // Activate the relay ...
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);
        // Give it some time to detect
        // go to the next one if the return current is detected or wait maximum 40ms
        int ticksMax = pdMS_TO_TICKS(80);
        vTaskDelay(pdMS_TO_TICKS(60));
        do
        {
            pSRelay->isConnected = HARDWAREGPIO_ReadConnectionSense();
            vTaskDelay(pdMS_TO_TICKS(1));
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
    }

    ESP_LOGI(TAG, "Check connection completed");
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_CheckingConnectionOK;
    HARDWAREGPIO_ClearRelayBus();
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    m_xHandle = NULL;
    vTaskDelete(NULL);
}

static bool StartFire(MAINAPP_SFire sFire)
{
    if (m_xHandle != NULL)
    {
        ESP_LOGE(TAG, "Already doing a job");
        goto ERROR;
    }

    if (sFire.u32OutputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringUnknownError;
        goto ERROR;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    if (!m_sState.bIsArmed)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringMasterSwitchWrongStateError;
        goto ERROR;
    }

    ESP_LOGI(TAG, "Fire command issued for output index: %"PRIu32, sFire.u32OutputIndex);

    MAINAPP_SFire* pCopyFire = (MAINAPP_SFire*)malloc(sizeof(MAINAPP_SFire));
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
    ERROR:
    return false;
}

static void FireTask(void* pParam)
{
    const MAINAPP_SFire* pFireParam = (const MAINAPP_SFire*)pParam;
    const uint32_t u32OutputIndex = pFireParam->u32OutputIndex;

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_Firing;

    ESP_LOGI(TAG, "Firing in progress: %"PRIu32, u32OutputIndex);

    MAINAPP_SRelay* pSRelay = &m_sOutputs[u32OutputIndex];

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

    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringOK;
    ESP_LOGI(TAG, "Firing is done");
    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    free(pFireParam);
    m_xHandle = NULL;
    vTaskDelete(NULL);
}

void MAINAPP_ExecCheckConnections()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MAINAPP_SCmd sCmd = { .eCmd = MAINAPP_ECMD_CheckConnections };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MAINAPP_ExecFire(uint32_t u32OutputIndex)
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MAINAPP_SCmd sCmd = { .eCmd = MAINAPP_ECMD_Fire, .uArg = { .sFire = { .u32OutputIndex = u32OutputIndex } } };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

static void UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh)
{
    MAINAPP_SRelay* pSRelay = &m_sOutputs[u32OutputIndex];

    MAINAPP_EOUTPUTSTATE eOutputState = MAINAPP_GetOutputState(pSRelay);
    if (eOutputState == MAINAPP_EOUTPUTSTATE_Enabled)
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 0, 200, 0);
    else if (eOutputState == MAINAPP_EOUTPUTSTATE_Fired) // White for fired
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 100, 100, 0);
    else if (eOutputState == MAINAPP_EOUTPUTSTATE_Connected) // YELLOW for connected
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 200, 200, 0);
    else // minimal white illuminiation
        HARDWAREGPIO_SetOutputRelayStatusColor(u32OutputIndex, 10, 10, 10);

    if (bForceRefresh)
        HARDWAREGPIO_RefreshLEDStrip();
}

MAINAPP_SRelay MAINAPP_GetRelayState(uint32_t u32OutputIndex)
{
    return m_sOutputs[u32OutputIndex];
}

MAINAPP_EOUTPUTSTATE MAINAPP_GetOutputState(const MAINAPP_SRelay* pSRelay)
{
    if (pSRelay->isEN)
        return MAINAPP_EOUTPUTSTATE_Enabled;
    if (pSRelay->isFired) // White for fired
        return MAINAPP_EOUTPUTSTATE_Fired;
    if (pSRelay->isConnected) // YELLOW for connected
        return MAINAPP_EOUTPUTSTATE_Connected;
    return MAINAPP_EOUTPUTSTATE_Idle;
}

bool MAINAPP_IsArmed()
{
    return m_sState.bIsArmed;
}

MAINAPP_EGENERALSTATE MAINAPP_GetGeneralState()
{
    return m_sState.eGeneralState;
}

static void UpdateOLED()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    if (m_sState.bIsArmed)
    {
        int32_t s32EncoderCount = HARDWAREGPIO_GetEncoderCount();

        sprintf(szText, "ARMED AND\nDANGEROUS\nPower: %"PRIi32" %%",
            s32EncoderCount);
    }
    else
    {
        char szSoftAPSSID[32] = {0,};

        esp_netif_ip_info_t wifiIpSta = {0};
        MAIN_GetWiFiSTAIP(&wifiIpSta);

        esp_netif_ip_info_t wifiIpAP = {0};
        MAIN_GetWiFiSoftAPIP(&wifiIpAP);

        MAIN_GetWifiAPSSID(szSoftAPSSID);
        sprintf(szText, "%s\n"IPSTR"\n"IPSTR,
            szSoftAPSSID,
            IP2STR(&wifiIpAP.ip),
            IP2STR(&wifiIpSta.ip));
    }

    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
