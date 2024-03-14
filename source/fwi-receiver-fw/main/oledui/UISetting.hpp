#ifndef _UISETTING_H_
#define _UISETTING_H_

#include "UICore.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void UISETTING_Enter();

void UISETTING_Exit();

void UISETTING_Tick();

void UISETTING_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UISETTING_INITLIFECYCLE { .FnEnter = UISETTING_Enter, .FnExit = UISETTING_Exit, .FnTick = UISETTING_Tick, .FnEncoderMove = UISETTING_EncoderMove }

#ifdef __cplusplus
}
#endif

#endif