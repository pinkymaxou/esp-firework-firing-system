#ifndef _SETTING_H_
#define _SETTING_H_

#include "nvsjson.h"

typedef enum
{
    SETTINGS_EENTRY_ESPNOW_PMK,

    SETTINGS_EENTRY_WiFiChannel,
    SETTINGS_EENTRY_WAPPass,

    SETTINGS_EENTRY_WSTAIsActive,
    SETTINGS_EENTRY_WSTASSID,
    SETTINGS_EENTRY_WSTAPass,

    SETTINGS_EENTRY_AutoDisarmTimeout,
    SETTINGS_EENTRY_FiringHoldTimeMS,

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

void SETTINGS_Init();

extern NVSJSON_SHandle g_sSettingHandle;

#endif