#ifndef _UISETTING_H_
#define _UISETTING_H_

#include "UICore.h"

void UISETTING_Enter();

void UISETTING_Exit();

void UISETTING_Tick();

void UISETTING_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UISETTING_INITLIFECYCLE { .FnEnter = UISETTING_Enter, .FnExit = UISETTING_Exit, .FnEncoderMove = UISETTING_EncoderMove, .FnTick = UISETTING_Tick }

#endif