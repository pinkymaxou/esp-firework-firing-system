#ifndef _SETTING_H_
#define _SETTING_H_

#include "nvsjson.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SETTINGS_EENTRY_ESPNOW_PMK,

    SETTINGS_EENTRY_WiFiChannel,
    SETTINGS_EENTRY_WAPPass,

    SETTINGS_EENTRY_WSTAIsActive,
    SETTINGS_EENTRY_WSTASSID,
    SETTINGS_EENTRY_WSTAPass,

    SETTINGS_EENTRY_FiringHoldTimeMS,
    SETTINGS_EENTRY_FiringPWMPercent,

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

void SETTINGS_Init();

#define SETTINGS_FIRINGHOLDTIMEMS_MIN (50)
#define SETTINGS_FIRINGHOLDTIMEMS_MAX (5000)

#define SETTINGS_FIRINGPWMPERCENT_MIN (5)
#define SETTINGS_FIRINGPWMPERCENT_MAX (100)

extern NVSJSON_SHandle g_sSettingHandle;

#ifdef __cplusplus
}
#endif

#endif