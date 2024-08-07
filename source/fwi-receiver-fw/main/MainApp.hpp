#ifndef _MAINAPP_H_
#define _MAINAPP_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "HWConfig.h"
#include "oledui/UIManager.hpp"

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

        LiveCheckContinuity = 11,

        DisarmedMasterSwitchOff = 12,
    };

    enum class ECmd
    {
        None = 0,
        Fire,
        CheckConnections,
        OutputCalib,
        LiveCheckContinuity,
    };

    struct SFire
    {
        uint32_t u32OutputIndex;
    };

    union UArg
    {
        SFire sFire;
    };

    struct SCmd
    {
        MainApp::ECmd eCmd;
        UArg uArg;
    } ;

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
        bool bIsContinuityCheckOK;

        EGeneralState eGeneralState;
        double dProgressOfOne;
    } SState;

    MainApp() = default;

    void Init();

    void Run();

    void ExecFire(uint32_t u32OutputIndex);

    void ExecCheckConnections();

    void ExecLiveCheckContinuity();

    void ExecFullOutputCalibration();

    void ExecCancel();

    SRelay GetRelayState(uint32_t u32OutputIndex);

    MainApp::EOutputState GetOutputState(const SRelay* pSRelay);

    MainApp::EGeneralState GetGeneralState();

    bool GetContinuityTest();

    bool IsArmed();

    double GetProgress();

    private:

    // Firing related
    bool StartCheckConnections();
    static void CheckConnectionsTask(void* pParam);

    bool StartFire(MainApp::SFire sFire);
    static void FireTask(void* pParam);

    bool StartFullOutputCalibration();
    static void FullOutputCalibrationTask(void* pParam);

    bool StartLiveCheckContinuity();
    static void LiveCheckContinuityTask(void* pParam);

    void UpdateLED(uint32_t u32OutputIndex, bool bForceRefresh);

    void CheckUserInput();

    private:
    SRelay m_sOutputs[HWCONFIG_OUTPUT_COUNT];
    SState m_sState = { .bIsArmed = false/*, .ttArmedTicks = 0*/, .dProgressOfOne = 0.0d };

    // Input commands
    SCmd m_sCmd = { .eCmd = ECmd::None };
    bool m_isOperationCancelled = false;

    // Semaphore
    StaticSemaphore_t m_xSemaphoreCreateMutex;
    SemaphoreHandle_t m_xSemaphoreHandle;

    // Launching related tasks
    TaskHandle_t m_xHandle = NULL;
};

extern MainApp g_app;

#endif