#ifndef _UIARMED_H_
#define _UIARMED_H_

#include "UICore.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void UIARMED_Enter();

void UIARMED_Exit();

void UIARMED_Tick();

void UIARMED_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UIARMED_INITLIFECYCLE { .FnEnter = UIARMED_Enter, .FnExit = UIARMED_Exit, .FnTick = UIARMED_Tick, .FnEncoderMove = UIARMED_EncoderMove }

#ifdef __cplusplus
}
#endif

#endif