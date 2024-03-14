#ifndef _MAINAPP_H_
#define _MAINAPP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MAINAPP_EOUTPUTSTATE_Idle = 0,
    MAINAPP_EOUTPUTSTATE_Enabled = 1,
    MAINAPP_EOUTPUTSTATE_Fired = 2,
    MAINAPP_EOUTPUTSTATE_Connected = 3,
} MAINAPP_EOUTPUTSTATE;

typedef enum
{
    MAINAPP_EGENERALSTATE_Idle = 0,

    MAINAPP_EGENERALSTATE_FiringMasterSwitchWrongStateError = 1,
    MAINAPP_EGENERALSTATE_FiringUnknownError = 2,
    MAINAPP_EGENERALSTATE_Firing = 3,
    MAINAPP_EGENERALSTATE_FiringOK = 4,

    MAINAPP_EGENERALSTATE_Armed = 7,

    MAINAPP_EGENERALSTATE_CheckingConnection = 8,
    MAINAPP_EGENERALSTATE_CheckingConnectionOK = 9,
    MAINAPP_EGENERALSTATE_CheckingConnectionError = 10,

    MAINAPP_EGENERALSTATE_DisarmedMasterSwitchOff = 12,
} MAINAPP_EGENERALSTATE;

typedef enum
{
    MAINAPP_ECMD_None = 0,
    MAINAPP_ECMD_Fire,
    MAINAPP_ECMD_CheckConnections,
    MAINAPP_ECMD_OutputCalib,
} MAINAPP_ECMD;

typedef struct
{
    uint32_t u32OutputIndex;
} MAINAPP_SFire;

typedef union
{
    MAINAPP_SFire sFire;
} MAINAPP_UArg;

typedef struct MainApp
{
    MAINAPP_ECMD eCmd;
    MAINAPP_UArg uArg;
} MAINAPP_SCmd;

typedef struct
{
    uint32_t u32Index;
    // Status
    bool isConnected;
    bool isFired;

    bool isEN;
} MAINAPP_SRelay;

void MAINAPP_Init();

void MAINAPP_Run();

void MAINAPP_ExecFire(uint32_t u32OutputIndex);

void MAINAPP_ExecCheckConnections();

void MAINAPP_ExecFullOutputCalibration();

MAINAPP_SRelay MAINAPP_GetRelayState(uint32_t u32OutputIndex);

MAINAPP_EOUTPUTSTATE MAINAPP_GetOutputState(const MAINAPP_SRelay* pSRelay);

MAINAPP_EGENERALSTATE MAINAPP_GetGeneralState();

bool MAINAPP_IsArmed();

double MAINAPP_GetProgress();

#ifdef __cplusplus
}
#endif

#endif