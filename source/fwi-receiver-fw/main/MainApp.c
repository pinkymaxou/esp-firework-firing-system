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

#define TAG "MainApp"

typedef struct
{
    uint32_t u32Index;
    // Status
    bool isConnected;
    bool isFired;

    bool isEN;
} SRelay;

typedef struct
{
    bool bIsArmed;
    TickType_t ttArmedTicks;
} SState;

#define INIT_RELAY(_gpio) { .gpio = _gpio, .isConnected = false, .isFired = false }

static SRelay m_sOutputs[HWCONFIG_OUTPUT_COUNT];
static SState m_sState = { .bIsArmed = false, .ttArmedTicks = 0 };

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

void MAINAPP_Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->u32Index = i;
        pSRelay->isConnected = false;
        pSRelay->isFired = false;
        pSRelay->isEN = false;
    }

    HARDWAREGPIO_WriteMasterPowerRelay(false);
    HARDWAREGPIO_ClearRelayBus();
}

void MAINAPP_Run()
{
    bool bSanityOn = false;
    TickType_t ttSanityTicks = 0;

    int32_t s32AutodisarmTimeoutMin = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AutoDisarmTimeout);

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
            const bool bIsTimeout = ttDiffArmed > pdMS_TO_TICKS(s32AutodisarmTimeoutMin*60*1000);
            const bool bIsNoMasterPower = !HARDWAREGPIO_ReadMasterPowerSense();

            if (bIsTimeout)
            {
                ESP_LOGI(TAG, "Automatic disarming, timeout");
                DisarmSystem();
            }
            else if (!bIsNoMasterPower)
            {
                ESP_LOGI(TAG, "Automatic disarming, master power switch as been deactivated");
                DisarmSystem();
            }
        }

        // Sanity blink ...
        if ( (xTaskGetTickCount() - ttSanityTicks) > pdMS_TO_TICKS(m_sState.bIsArmed ? 50 : 250))
        {
            ttSanityTicks = xTaskGetTickCount();
            HARDWAREGPIO_SetSanityLED(bSanityOn);
            bSanityOn = !bSanityOn;
        }

        /* Refresh the strip to send data */
        // Update LEDs
        for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
            UpdateLED(i);

        HARDWAREGPIO_RefreshLEDStrip();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void CheckConnections()
{
    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Clear relay bus
    HARDWAREGPIO_ClearRelayBus();

    // Scan the bus to find connected
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->isConnected = false;

        // Activate the relay ...
        HARDWAREGPIO_WriteSingleRelay(pSRelay->u32Index, true);
        const bool bConnSense = HARDWAREGPIO_ReadConnectionSense();
        vTaskDelay(pdMS_TO_TICKS(25));  // Give it some time to detect
        pSRelay->isConnected = bConnSense;
    }
}

static void ArmSystem()
{
    m_sState.bIsArmed = false;

    // Ensure power is availble
    if (!HARDWAREGPIO_ReadMasterPowerSense())
    {
        // Master power is not active
        ESP_LOGE(TAG, "Unable to arm the system, no power!");
        // TODO: Display the error somewhere ...
        return;
    }

    // Check every connected outputs
    CheckConnections();

    // Reset fired counter
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->isFired = false;
    }

    m_sState.ttArmedTicks = xTaskGetTickCount();
    m_sState.bIsArmed = true;
    ESP_LOGI(TAG, "System is now armed and dangereous!");
    return;
}

static void DisarmSystem()
{
    if (m_sState.bIsArmed)
        ESP_LOGI(TAG, "Disarming system");

    m_sState.bIsArmed = false;
}

static void Fire(uint32_t u32OutputIndex)
{
    if (u32OutputIndex >= HWCONFIG_OUTPUT_COUNT)
    {
        ESP_LOGE(TAG, "Output index is invalid !");
        return;
    }

    // If it's in dry-run mode, power shouldn't be present
    // if it's armed, power should be present.
    const bool bIsReady =
        (m_sState.bIsArmed && HARDWAREGPIO_ReadMasterPowerSense()) ||
        (!m_sState.bIsArmed && !HARDWAREGPIO_ReadMasterPowerSense());

    if (!bIsReady)
    {
        // TODO: Report the error;
        ESP_LOGE(TAG, "Cannot fire, not ready !");
        return;
    }

    // If it's armed and ready, enable the master relay
    if (m_sState.bIsArmed)
        HARDWAREGPIO_WriteMasterPowerRelay(true);

    SRelay* pSRelay = &m_sOutputs[u32OutputIndex];

    ESP_LOGI(TAG, "Firing on output index: %d", (int)u32OutputIndex);
    int32_t s32FireHoldTimeMS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    pSRelay->isEN = true;
    UpdateLED(u32OutputIndex, true);
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, true);
    vTaskDelay(pdMS_TO_TICKS(s32FireHoldTimeMS));
    HARDWAREGPIO_WriteSingleRelay(u32OutputIndex, false);
    pSRelay->isEN = false;
    pSRelay->isFired = true;
    UpdateLED(u32OutputIndex, true);

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);
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
    SRelay* pSRelay = &m_sOutputs[u32OutputIndex];
    if (pSRelay->isEN)
        HARDWAREGPIO_SetOutputRelayStatusColor(i, 0, 200, 0);
    else if (pSRelay->isFired) // White for fired
        HARDWAREGPIO_SetOutputRelayStatusColor(i, 100, 100, 0);
    else if (pSRelay->isConnected) // YELLOW for connected
        HARDWAREGPIO_SetOutputRelayStatusColor(i, 200, 200, 0);
    else // minimal white illuminiation
        HARDWAREGPIO_SetOutputRelayStatusColor(i, 10, 10, 10);

    if (bForceRefresh)
        HARDWAREGPIO_RefreshLEDStrip();
}