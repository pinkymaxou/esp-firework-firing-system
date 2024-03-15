#ifndef _MAINAPP_H_
#define _MAINAPP_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "HWConfig.h"

class MainApp
{
    public:

    enum class EOutputState
    {
        Idle = 0,
        Enabled = 1,
        Fired = 2,
        Connected = 3,
    };

    enum class EGeneralState
    {
        Idle = 0,

        FiringMasterSwitchWrongStateError = 1,
        FiringUnknownError = 2,
        Firing = 3,
        FiringOK = 4,

        Armed = 7,

        CheckingConnection = 8,
        CheckingConnectionOK = 9,
        CheckingConnectionError = 10,

        DisarmedMasterSwitchOff = 12,
    };

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

        MainApp::EGeneralState eGeneralState;
        double dProgressOfOne;
    } SState;

    MainApp() = default;

    void Init();

    void Run();

    void ExecFire(uint32_t u32OutputIndex);

    void ExecCheckConnections();

    void ExecFullOutputCalibration();

    SRelay GetRelayState(uint32_t u32OutputIndex);

    MainApp::EOutputState GetOutputState(const SRelay* pSRelay);

    MainApp::EGeneralState GetGeneralState();

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