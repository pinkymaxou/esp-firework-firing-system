#ifndef _UIMANAGER_H_
#define _UIMANAGER_H_

#include <stdint.h>
#include <stddef.h>
#include "UICore.h"
#include "UIHome.h"
#include "UIArmed.h"
#include "UIErrorPleaseDisarm.h"

typedef enum
{
    UIMANAGER_EMENU_Home = 0,
    UIMANAGER_EMENU_ArmedReady,

    UIMANAGER_EMENU_ErrorPleaseDisarm,
    //UIMANAGER_EMENU_Setting,

    UIMANAGER_EMENU_Count
} UIMANAGER_EMENU;

void UIMANAGER_Goto(UIMANAGER_EMENU eMenu);

void UIMANAGER_RunTick();

#endif