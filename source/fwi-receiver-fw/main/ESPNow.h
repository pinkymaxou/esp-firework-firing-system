#ifndef _ESPNOW_H_
#define _ESPNOW_H_

#include <stdbool.h>
#include "fwi-output.h"
#include "fwi-remote-comm.h"

typedef struct
{
    FWIREMOTECOMM_EFRAMEID eFrameID;
    union
    {
        FWIREMOTECOMM_C2SGetStatus c2sGetStatus;
        FWIREMOTECOMM_C2SFire c2sFire;
    } URx;
} __attribute__((aligned)) ESPNOW_SRxFrame;

typedef struct
{
    FWIREMOTECOMM_EFRAMEID eFrameID;
    union
    {
        FWIREMOTECOMM_S2CGetStatusResp s2cGetStatusResp;
        FWIREMOTECOMM_S2CFireResp s2cFireResp;
    } URx;
} __attribute__((aligned)) ESPNOW_STxFrame;

void ESPNOW_Init();

void ESPNOW_RunTick();

#endif