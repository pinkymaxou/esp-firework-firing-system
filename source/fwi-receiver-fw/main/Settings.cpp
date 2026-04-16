#include "Settings.hpp"
#include <string.h>

static bool validateWifiPassword(const NVSJSON_SSettingEntry* setting_entry, const char* value);

namespace Settings
{

NVSJSON_SHandle g_handle;

static const NVSJSON_SSettingEntry m_configEntries[Count] =
{
    // MIN MAX DEFAULT
    [WiFiChannel]    = NVSJSON_INITINT32_RNG("WiFi.Chan",     "Wi-Fi channel, cannot connect to AP with different channel", 3, 1, 11, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    // WiFi Station related
    [WAPPass]        = NVSJSON_INITSTRING_VAL("WAP.Pass",     "WiFi password [empty, or > 8 characters]", "", validateWifiPassword, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    // WiFi Station related
    [WSTAIsActive]   = NVSJSON_INITINT32_RNG("WSTA.IsActive", "Wifi is active", 0, 0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [WSTASSID]       = NVSJSON_INITSTRING("WSTA.SSID",    "WiFi (SSID)", "", NVSJSON_EFLAGS_NeedsReboot),
    [WSTAPass]       = NVSJSON_INITSTRING_VAL("WSTA.Pass",    "WiFi password [empty, or > 8 characters]", "", validateWifiPassword, (NVSJSON_EFLAGS)(NVSJSON_EFLAGS_None | NVSJSON_EFLAGS_NeedsReboot)),

    [FiringHoldTimeMS] = NVSJSON_INITINT32_RNG("FireHoldMS", "How long to hold power (MS)", 750, FIRINGHOLDTIMEMS_MIN, FIRINGHOLDTIMEMS_MAX, NVSJSON_EFLAGS_NeedsReboot),
    [FiringPWMPercent] = NVSJSON_INITINT32_RNG("FirePWM", "Master power PWM percent", 25, FIRINGPWMPERCENT_MIN, FIRINGPWMPERCENT_MAX, NVSJSON_EFLAGS_NeedsReboot),
};
static_assert(Count == (sizeof(m_configEntries)/sizeof(m_configEntries[0])), "Config entries doesn't match the enum");

static NVSJSON_SConfig m_config = { .partitionName = "NVS", .settingEntries = m_configEntries, .settingEntryCount = (uint32_t)Count };

void init()
{
    NVSJSON_Init(&g_handle, &m_config);
    NVSJSON_Load(&g_handle);
}

} // namespace Settings

static bool validateWifiPassword(const NVSJSON_SSettingEntry* setting_entry, const char* value)
{
    const size_t n = strlen(value);
    return n > 8 || 0 == n;
}
