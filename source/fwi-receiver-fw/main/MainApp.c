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

#define TAG "MainApp"

typedef struct
{
    bool bIsArmed;
    TickType_t ttArmedTicks;

    MAINAPP_EGENERALSTATE eGeneralState;
} SState;

#define INIT_RELAY(_gpio) { .gpio = _gpio, .isConnected = false, .isFired = false, .eGeneralState = MAINAPP_EGENERALSTATE_Idle }

static MAINAPP_SRelay m_sOutputs[HWCONFIG_OUTPUT_COUNT];
static SState m_sState = { .bIsArmed = false, .ttArmedTicks = 0 };

static int32_t m_s32AutodisarmTimeoutMin = 0;

// Input commands
static MAINAPP_SCmd m_sCmd = { .eCmd = MAINAPP_ECMD_None };

// Semaphore
static StaticSemaphore_t m_xSemaphoreCreateMutex;
static SemaphoreHandle_t m_xSemaphoreHandle;

static void CheckConnections();
static void ArmSystem();
static void DisarmSystem();
static void Fire(uint32_t u32OutputIndex);

static void UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh);
static void UpdateOLED();

void MAINAPP_Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MAINAPP_SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->u32Index = i;
        pSRelay->isConnected = false;
        pSRelay->isFired = false;
        pSRelay->isEN = false;
    }

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    HARDWAREGPIO_ClearRelayBus();

    UpdateOLED();
}

void MAINAPP_Run()
{
    bool bSanityOn = false;
    TickType_t ttSanityTicks = 0;
    TickType_t ttUpdateOLEDTick = 0;

    m_s32AutodisarmTimeoutMin = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AutoDisarmTimeout);

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
                    if (m_sState.bIsArmed)
                    {
                        ESP_LOGI(TAG, "Forcing disarm");
                        DisarmSystem();
                    }
                    CheckConnections();
                    break;
                case MAINAPP_ECMD_Arm:
                    ESP_LOGI(TAG, "Arm command issued");
                    ArmSystem();
                    break;
                case MAINAPP_ECMD_Disarm:
                    ESP_LOGI(TAG, "Disarm command issued");
                    DisarmSystem();
                    break;
                case MAINAPP_ECMD_Fire:
                    ESP_LOGI(TAG, "Fire command issued for output index: %d", (int)sCmd.uArg.sFire.u32OutputIndex);
                    Fire(sCmd.uArg.sFire.u32OutputIndex);

                    // Reset armed timeout ..
                    if (m_sState.bIsArmed)
                        m_sState.ttArmedTicks = xTaskGetTickCount();
                    break;
                default:
                    break;
            }
        }

        // Check for disarming condition
        if (m_sState.bIsArmed)
        {
            const TickType_t ttDiffArmed = (xTaskGetTickCount() - m_sState.ttArmedTicks);

            // Automatic disarm after 15 minutes
            const bool bIsTimeout = ttDiffArmed > pdMS_TO_TICKS(m_s32AutodisarmTimeoutMin*60*1000);
            const bool bIsNoMasterPower = !HARDWAREGPIO_ReadMasterPowerSense();

            if (bIsTimeout)
            {
                ESP_LOGI(TAG, "Automatic disarming, timeout");
                DisarmSystem();
                m_sState.eGeneralState = MAINAPP_EGENERALSTATE_DisarmedAutomaticTimeout;
            }
            else if (bIsNoMasterPower)
            {
                ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
                DisarmSystem();
                m_sState.eGeneralState = MAINAPP_EGENERALSTATE_DisarmedMasterSwitchOff;
            }
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - ttSanityTicks) > pdMS_TO_TICKS(m_sState.bIsArmed ? 50 : 500))
        {
            ttSanityTicks = xTaskGetTickCount();
            HARDWAREGPIO_SetSanityLED(bSanityOn);

            bSanityOn = !bSanityOn;
        }

        // Update LEDs
        /* Refresh the strip to send data */
        // Update LEDs
        for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
            UpdateLED(i, false);

        HARDWAREGPIO_RefreshLEDStrip();

        // Update LEDs
        if ( (xTaskGetTickCount() - ttUpdateOLEDTick) > pdMS_TO_TICKS(250) )
        {
            UpdateOLED();
            ttUpdateOLEDTick = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void CheckConnections()
{
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_CheckingConnection;

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HARDWAREGPIO_ClearRelayBus();

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MAINAPP_SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->isConnected = false;

        // Activate the relay ...
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);
        const bool bConnSense = HARDWAREGPIO_ReadConnectionSense();
        vTaskDelay(pdMS_TO_TICKS(100));  // Give it some time to detect
        pSRelay->isConnected = bConnSense;
    }
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_CheckingConnectionOK;
}

static void ArmSystem()
{
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_ArmingSystem;
    m_sState.bIsArmed = false;

    // Ensure power is availble
    if (!HARDWAREGPIO_ReadMasterPowerSense())
    {
        // Master power is not active
        ESP_LOGE(TAG, "Unable to arm the system, no power!");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_ArmingSystemNoPowerError;
        return;
    }

    // Check every connected outputs
    CheckConnections();

    // Reset fired counter
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        MAINAPP_SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->isFired = false;
    }

    m_sState.ttArmedTicks = xTaskGetTickCount();
    m_sState.bIsArmed = true;
    ESP_LOGI(TAG, "System is now armed and dangereous!");
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_ArmingSystemOK;
    return;
}

static void DisarmSystem()
{
    if (m_sState.bIsArmed)
        ESP_LOGI(TAG, "Disarming system");
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_Disarmed;
    m_sState.bIsArmed = false;
}

static void Fire(uint32_t u32OutputIndex)
{
    HARDWAREGPIO_WriteMasterPowerRelay(false);
    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_Firing;

    if (u32OutputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringUnknownError;
        return;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    const bool bIsReady =
        (m_sState.bIsArmed && HARDWAREGPIO_ReadMasterPowerSense()) ||
        (!m_sState.bIsArmed && !HARDWAREGPIO_ReadMasterPowerSense());

    if (!bIsReady)
    {
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringMasterSwitchWrongStateError;
        return;
    }

    MAINAPP_SRelay* pSRelay = &m_sOutputs[u32OutputIndex];

    pSRelay->isEN = true;

    // If it's armed and ready, enable the master relay
    if (m_sState.bIsArmed)
        HARDWAREGPIO_WriteMasterPowerRelay(true);

    ESP_LOGI(TAG, "Firing on output index: %d", (int)u32OutputIndex);
    int32_t s32FireHoldTimeMS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    UpdateLED(u32OutputIndex, true);
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, true);
    vTaskDelay(pdMS_TO_TICKS(s32FireHoldTimeMS));
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, false);
    UpdateLED(u32OutputIndex, true);

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    pSRelay->isEN = false;
    pSRelay->isFired = true;

    m_sState.eGeneralState = MAINAPP_EGENERALSTATE_FiringOK;
}

void MAINAPP_ExecArm()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MAINAPP_SCmd sCmd = { .eCmd = MAINAPP_ECMD_Arm };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
}

void MAINAPP_ExecDisarm()
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
    const MAINAPP_SCmd sCmd = { .eCmd = MAINAPP_ECMD_Disarm };
    m_sCmd = sCmd;
    xSemaphoreGive(m_xSemaphoreHandle);
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
        const int32_t s32DiffS = ((m_s32AutodisarmTimeoutMin*60*1000) - pdTICKS_TO_MS(xTaskGetTickCount() - m_sState.ttArmedTicks)) / 1000;
        int min = 0;
        int sec = 0;
        if (s32DiffS >= 0)
        {
            min = (int)(s32DiffS / 60);
            sec = (int)(s32DiffS % 60);
        }
        sprintf(szText, "ARMED AND\nDANGEROUS\n%02d:%02d",
            /*0*/min,
            /*1*/sec);
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