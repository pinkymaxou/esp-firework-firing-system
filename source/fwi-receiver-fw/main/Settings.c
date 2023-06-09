#include "Settings.h"
#include <string.h>

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);

NVSJSON_SHandle g_sSettingHandle;

static const NVSJSON_SSettingEntry g_sConfigEntries[SETTINGS_EENTRY_Count] =
{
    // MIN MAX DEFAULT
    [SETTINGS_EENTRY_ESPNOW_PMK]   = NVSJSON_INITSTRING_RNG("espn.pmk",    "Primary master key for ESP-NOW", "pmk1234567890123", NVSJSON_EFLAGS_NeedsReboot),

    [SETTINGS_EENTRY_WiFiChannel]  = NVSJSON_INITINT32_RNG("WiFi.Chan",     "Wi-Fi channel, cannot connect to AP with different channel", 3, 1, 11, NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot),

    // WiFi Station related
    [SETTINGS_EENTRY_WAPPass]      = NVSJSON_INITSTRING_VAL("WAP.Pass", "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    // WiFi Station related
    [SETTINGS_EENTRY_WSTAIsActive] = NVSJSON_INITINT32_RNG("WSTA.IsActive", "Wifi is active", 0, 0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTASSID]     = NVSJSON_INITSTRING_RNG("WSTA.SSID",    "WiFi (SSID)", "", NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTAPass]     = NVSJSON_INITSTRING_VAL("WSTA.Pass",    "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    [SETTINGS_EENTRY_AutoDisarmTimeout] = NVSJSON_INITINT32_RNG("AutoDisarmT", "Automatically disarm after X min of inactivity", 3, 1, 20, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_FiringHoldTimeMS]  = NVSJSON_INITINT32_RNG("FireHoldMS", "How long to hold power (MS)", 5000, 50, 5000, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_FiringPWMPercent]  = NVSJSON_INITDOUBLE_RNG("FirePWM", "Master power PWM percent", 0.95d, 0.05d, 1.0d, NVSJSON_EFLAGS_NeedsReboot),
};

void SETTINGS_Init()
{
    NVSJSON_Init(&g_sSettingHandle, g_sConfigEntries, SETTINGS_EENTRY_Count);
}

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue)
{
    const size_t n = strlen(szValue);
    return n > 8 || n == 0;
}