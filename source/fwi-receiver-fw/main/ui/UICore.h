#ifndef _UICORE_H_
#define _UICORE_H_

#include <stdint.h>

typedef enum
{
    UICORE_EBTNEVENT_Click,
    UICORE_EBTNEVENT_EncoderClick
} UICORE_EBTNEVENT;

typedef void (*fnEnter)(void);
typedef void (*fnExit)(void);
typedef void (*fnEncoderMove)(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount);

typedef struct
{
    fnEnter FnEnter;
    fnExit FnExit;
    fnEncoderMove FnEncoderMove;
} UICORE_SLifeCycle;

#endif