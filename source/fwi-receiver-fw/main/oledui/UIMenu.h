#ifndef _UIMENU_H_
#define _UIMENU_H_

#include "UICore.h"

#ifdef __cplusplus
extern "C" {
#endif

void UIMENU_Enter();

void UIMENU_Exit();

void UIMENU_Tick();

void UIMENU_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UIMENU_INITLIFECYCLE { .FnEnter = UIMENU_Enter, .FnExit = UIMENU_Exit, .FnTick = UIMENU_Tick, .FnEncoderMove = UIMENU_EncoderMove }

#ifdef __cplusplus
}
#endif

#endif