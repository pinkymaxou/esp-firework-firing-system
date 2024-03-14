#ifndef _UICORE_H_
#define _UICORE_H_

#include <stdint.h>
#include "../HardwareGPIO.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UICORE_EBTNEVENT_Click,
    UICORE_EBTNEVENT_EncoderClick
} UICORE_EBTNEVENT;

typedef void (*fnEnter)(void);
typedef void (*fnExit)(void);
typedef void (*fnEncoderMove)(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);
typedef void (*fnTick)(void);

typedef struct
{
    fnEnter FnEnter;
    fnExit FnExit;
    fnTick FnTick;
    fnEncoderMove FnEncoderMove;
} UICORE_SLifeCycle;

#ifdef __cplusplus
}
#endif

#endif