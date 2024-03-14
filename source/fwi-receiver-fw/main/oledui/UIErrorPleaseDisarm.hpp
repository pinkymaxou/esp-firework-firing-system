#ifndef _UIERRORPLEASEDISARM_H_
#define _UIERRORPLEASEDISARM_H_

#include "UICore.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void UIERRORPLEASEDISARM_Enter();

void UIERRORPLEASEDISARM_Exit();

void UIERRORPLEASEDISARM_Tick();

void UIERRORPLEASEDISARM_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UIERRORPLEASEDISARM_INITLIFECYCLE { .FnEnter = UIERRORPLEASEDISARM_Enter, .FnExit = UIERRORPLEASEDISARM_Exit, .FnTick = UIERRORPLEASEDISARM_Tick, .FnEncoderMove = UIERRORPLEASEDISARM_EncoderMove }

#ifdef __cplusplus
}
#endif

#endif