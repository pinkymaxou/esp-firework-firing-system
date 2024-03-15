#ifndef _MAINAPP_H_
#define _MAINAPP_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "HWConfig.h"

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

class MainApp
{
    public:
    typedef struct
    {
        uint32_t u32OutputIndex;
    } MAINAPP_SFire;

    typedef union
    {
        MAINAPP_SFire sFire;
    } MAINAPP_UArg;

    typedef struct
    {
        MAINAPP_ECMD eCmd;
        MAINAPP_UArg uArg;
    } MAINAPP_SCmd;

    struct SRelay
    {
        uint32_t u32Index;
        // Status
        bool isConnected;
        bool isFired;

        bool isEN;
    };

    typedef struct
    {
        bool bIsArmed;
        // TickType_t ttArmedTicks;

        MAINAPP_EGENERALSTATE eGeneralState;
        double dProgressOfOne;
    } SState;

    MainApp() = default;

    void Init();

    void Run();

    void ExecFire(uint32_t u32OutputIndex);

    void ExecCheckConnections();

    void ExecFullOutputCalibration();

    SRelay GetRelayState(uint32_t u32OutputIndex);

    MAINAPP_EOUTPUTSTATE GetOutputState(const SRelay* pSRelay);

    MAINAPP_EGENERALSTATE GetGeneralState();

    bool IsArmed();

    double GetProgress();

    private:

    // Firing related
    bool StartCheckConnections();
    static void CheckConnectionsTask(void* pParam);

    bool StartFire(MAINAPP_SFire sFire);
    static void FireTask(void* pParam);

    bool StartFullOutputCalibrationTask();
    static void FullOutputCalibrationTask(void* pParam);

    void UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh);

    void CheckUserInput();

    private:
    SRelay m_sOutputs[HWCONFIG_OUTPUT_COUNT];
    SState m_sState = { .bIsArmed = false/*, .ttArmedTicks = 0*/, .dProgressOfOne = 0.0d };

    // Input commands
    MAINAPP_SCmd m_sCmd = { .eCmd = MAINAPP_ECMD_None };

    // Semaphore
    StaticSemaphore_t m_xSemaphoreCreateMutex;
    SemaphoreHandle_t m_xSemaphoreHandle;

    // Launching related tasks
    TaskHandle_t m_xHandle = NULL;
};

extern MainApp g_app;

#endif