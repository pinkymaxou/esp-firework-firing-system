#ifndef _UIMENU_H_
#define _UIMENU_H_

#include "UICore.h"

void UIMENU_Enter();

void UIMENU_Exit();

void UIMENU_Tick();

void UIMENU_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UIMENU_INITLIFECYCLE { .FnEnter = UIMENU_Enter, .FnExit = UIMENU_Exit, .FnEncoderMove = UIMENU_EncoderMove, .FnTick = UIMENU_Tick }

#endif