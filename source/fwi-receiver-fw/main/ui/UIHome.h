#ifndef _UIHOME_H_
#define _UIHOME_H_

#include "UICore.h"

void UIHOME_Enter();

void UIHOME_Exit();

void UIHOME_Tick();

void UIHOME_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UIHOME_INITLIFECYCLE { .FnEnter = UIHOME_Enter, .FnExit = UIHOME_Exit, .FnEncoderMove = UIHOME_EncoderMove, .FnTick = UIHOME_Tick }

#endif