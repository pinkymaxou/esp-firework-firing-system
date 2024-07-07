#pragma once

#include <stdint.h>
#include <stddef.h>
#include "UIBase.hpp"
#include "UIHome.hpp"
#include "UIArmed.hpp"
#include "UISetting.hpp"
#include "UIErrorPleaseDisarm.hpp"
#include "UILiveCheckContinuity.hpp"
#include "UIMenu.hpp"
#include "UITestConn.hpp"
#include "UIAbout.hpp"

class UIManager
{
    public:
    enum class EMenu
    {
        None = -1,

        Home = 0,
        ArmedReady,

        ErrorPleaseDisarm,
        Menu,
        Setting,
        TestConn,
        LiveCheckContinuity,
        About,

        Count
    };

    void Goto(EMenu eMenu);

    void EncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount);

    void RunTick();

    private:
    UIBase* GetMenuLC(EMenu eMenu);

    EMenu m_eCurrentMenu = EMenu::None;

    // UI
    UIHome m_sHome;
    UIArmed m_sArmed;
    UIErrorPleaseDisarm m_sErrorPleaseDisarm;
    UIMenu m_sMenu;
    UISetting m_sSetting;
    UITestConn m_sTestConn;
    UILiveCheckContinuity m_sLiveCheckContinuity;
    UIAbout m_sAbout;

    UIBase* m_psUIHomes[(int)UIManager::EMenu::Count] =
    {
        [(int)UIManager::EMenu::Home] = (UIBase*)&m_sHome,
        [(int)UIManager::EMenu::ArmedReady] = (UIBase*)&m_sArmed,
        [(int)UIManager::EMenu::ErrorPleaseDisarm] = (UIBase*)&m_sErrorPleaseDisarm,
        [(int)UIManager::EMenu::Menu] = (UIBase*)&m_sMenu,
        [(int)UIManager::EMenu::Setting] = (UIBase*)&m_sSetting,
        [(int)UIManager::EMenu::TestConn] = (UIBase*)&m_sTestConn,
        [(int)UIManager::EMenu::LiveCheckContinuity] = (UIBase*)&m_sLiveCheckContinuity,
        [(int)UIManager::EMenu::About] = (UIBase*)&m_sAbout,
    };
    //static_assert(UIManager::EMenu::Count == (sizeof(m_psUIHomes)/sizeof(m_psUIHomes[0])), "Life cycle doesn't fit");

};
extern UIManager g_uiMgr;
