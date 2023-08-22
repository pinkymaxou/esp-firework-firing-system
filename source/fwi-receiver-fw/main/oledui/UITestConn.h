#ifndef _UITESTCONN_H_
#define _UITESTCONN_H_

#include "UICore.h"

void UITESTCONN_Enter();

void UITESTCONN_Exit();

void UITESTCONN_Tick();

void UITESTCONN_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

#define UITESTCONN_INITLIFECYCLE { .FnEnter = UITESTCONN_Enter, .FnExit = UITESTCONN_Exit, .FnEncoderMove = UITESTCONN_EncoderMove, .FnTick = UITESTCONN_Tick }

#endif