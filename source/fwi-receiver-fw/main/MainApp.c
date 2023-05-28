#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "MainApp.h"
#include "HWConfig.h"
#include "HardwareGPIO.h"

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

static void CheckConnections();
static void ArmSystem();
static void DisarmSystem();

void MAINAPP_Init()
{
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

    while (true)
    {
        // Check for disarming condition
        if (m_sState.bIsArmed)
        {
            const TickType_t ttDiffArmed = (xTaskGetTickCount() - m_sState.ttArmedTicks);

            // Automatic disarm after 15 minutes
            const bool bIsTimeout = ttDiffArmed > pdMS_TO_TICKS(3*300*1000);
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
        HARDWAREGPIO_WriteSingleRelay(i, true);
        const bool bConnSense = HARDWAREGPIO_ReadConnectionSense();
        vTaskDelay(pdMS_TO_TICKS(25));  // Give it some time to detect
        pSRelay->isConnected = bConnSense;
    }
}

static void ArmSystem()
{
    m_sState.bIsArmed = false;
    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);

    // Ensure power is availble
    if (!HARDWAREGPIO_ReadMasterPowerSense())
    {
        // Master power is not active
        ESP_LOGE(TAG, "Unable to arm the system, no power!");
        // TODO: Display the error somewhere ...
        return;
    }

    CheckConnections();

    // Reset fired counter
    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        SRelay* pSRelay = &m_sOutputs[i];
        pSRelay->isFired = false;
    }

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(true);
    m_sState.bIsArmed = true;
    ESP_LOGI(TAG, "System is now armed and dangereous!");
    return;
}

static void DisarmSystem()
{
    if (m_sState.bIsArmed)
        ESP_LOGI(TAG, "Disarming system");

    // Master power relay shouln'd be active during check
    HARDWAREGPIO_WriteMasterPowerRelay(false);
    m_sState.bIsArmed = false;
}