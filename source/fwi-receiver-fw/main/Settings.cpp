#include "Settings.hpp"
#include <string.h>

static bool validateWifiPassword(const NVSJSON_SSettingEntry* setting_entry, const char* value);

NVSJSON_SHandle g_settingHandle;

static const NVSJSON_SSettingEntry m_configEntries[SETTINGS_EENTRY_Count] =
{
    // MIN MAX DEFAULT
    [SETTINGS_EENTRY_WiFiChannel]  = NVSJSON_INITINT32_RNG("WiFi.Chan",     "Wi-Fi channel, cannot connect to AP with different channel", 3, 1, 11, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    // WiFi Station related
    [SETTINGS_EENTRY_WAPPass]      = NVSJSON_INITSTRING_VAL("WAP.Pass",     "WiFi password [empty, or > 8 characters]", "", validateWifiPassword, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    // WiFi Station related
    [SETTINGS_EENTRY_WSTAIsActive] = NVSJSON_INITINT32_RNG("WSTA.IsActive", "Wifi is active", 0, 0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTASSID]     = NVSJSON_INITSTRING("WSTA.SSID",    "WiFi (SSID)", "", NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTAPass]     = NVSJSON_INITSTRING_VAL("WSTA.Pass",    "WiFi password [empty, or > 8 characters]", "", validateWifiPassword, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    [SETTINGS_EENTRY_FiringHoldTimeMS]  = NVSJSON_INITINT32_RNG("FireHoldMS", "How long to hold power (MS)", 750, SETTINGS_FIRINGHOLDTIMEMS_MIN, SETTINGS_FIRINGHOLDTIMEMS_MAX, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_FiringPWMPercent]  = NVSJSON_INITINT32_RNG("FirePWM", "Master power PWM percent", 25, SETTINGS_FIRINGPWMPERCENT_MIN, SETTINGS_FIRINGPWMPERCENT_MAX, NVSJSON_EFLAGS_NeedsReboot),
};
static_assert(SETTINGS_EENTRY_Count == (sizeof(m_configEntries)/sizeof(m_configEntries[0])), "Config entries doesn't match the enum");

static NVSJSON_SConfig m_config = { .partitionName = "NVS", .settingEntries = m_configEntries, .settingEntryCount = (uint32_t)SETTINGS_EENTRY_Count };

void SETTINGS_Init()
{
    NVSJSON_Init(&g_settingHandle, &m_config);
    NVSJSON_Load(&g_settingHandle);
}

static bool validateWifiPassword(const NVSJSON_SSettingEntry* setting_entry, const char* value)
{
    const size_t n = strlen(value);
    return n > 8 || 0 == n;
}
