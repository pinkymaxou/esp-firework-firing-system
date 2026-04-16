#pragma once

#include "nvsjson.h"

namespace Settings
{
    enum EEntry
    {
        WiFiChannel,
        WAPPass,

        WSTAIsActive,
        WSTASSID,
        WSTAPass,

        FiringHoldTimeMS,
        FiringPWMPercent,

        Count
    };

    void init();

    constexpr int32_t FIRINGHOLDTIMEMS_MIN = 50;
    constexpr int32_t FIRINGHOLDTIMEMS_MAX = 5000;

    constexpr int32_t FIRINGPWMPERCENT_MIN = 5;
    constexpr int32_t FIRINGPWMPERCENT_MAX = 100;

    extern NVSJSON_SHandle g_handle;
}
