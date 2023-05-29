#ifndef _MAINAPP_H_
#define _MAINAPP_H_

typedef enum
{
    MAINAPP_ECMD_None = 0,

    MAINAPP_ECMD_Disarm,
    MAINAPP_ECMD_Arm,
    MAINAPP_ECMD_Fire,

    MAINAPP_ECMD_CheckConnections,
} MAINAPP_ECMD;

typedef enum
{
    MAINAPP_EOUTPUTSTATE_Idle = 0,
    MAINAPP_EOUTPUTSTATE_Enabled = 1,
    MAINAPP_EOUTPUTSTATE_Fired = 2,
    MAINAPP_EOUTPUTSTATE_Connected = 3,
} MAINAPP_EOUTPUTSTATE;

typedef union
{
    struct
    {
        uint8_t u8Dummy;
    } sDisarm;
    struct
    {
        uint8_t u8Dummy;
    } sArm;
    struct
    {
        uint32_t u32OutputIndex;
    } sFire;
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

void MAINAPP_ExecArm();

void MAINAPP_ExecDisarm();

void MAINAPP_ExecFire(uint32_t u32OutputIndex);

void MAINAPP_ExecCheckConnections();

MAINAPP_SRelay MAINAPP_GetRelayState(uint32_t u32OutputIndex);

MAINAPP_EOUTPUTSTATE MAINAPP_GetOutputState(const MAINAPP_SRelay* pSRelay);

#endif