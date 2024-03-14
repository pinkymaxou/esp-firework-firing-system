#ifndef _UIMANAGER_H_
#define _UIMANAGER_H_

#include <stdint.h>
#include <stddef.h>
#include "UICore.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UIMANAGER_EMENU_None = -1,

    UIMANAGER_EMENU_Home = 0,
    UIMANAGER_EMENU_ArmedReady,

    UIMANAGER_EMENU_ErrorPleaseDisarm,
    UIMANAGER_EMENU_Menu,
    UIMANAGER_EMENU_Setting,
    UIMANAGER_EMENU_TestConn,

    UIMANAGER_EMENU_Count
} UIMANAGER_EMENU;

void UIMANAGER_Goto(UIMANAGER_EMENU eMenu);

void UIMANAGER_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

void UIMANAGER_RunTick();

#ifdef __cplusplus
}
#endif

#endif