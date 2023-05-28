#ifndef _MAINAPP_H_
#define _MAINAPP_H_

typedef enum
{
    MAINAPP_ECMD_None = 0,

    MAINAPP_ECMD_Disarm,
    MAINAPP_ECMD_Arm,
    MAINAPP_ECMD_Fire,
} MAINAPP_ECMD;

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

void MAINAPP_Init();

void MAINAPP_Run();

#endif