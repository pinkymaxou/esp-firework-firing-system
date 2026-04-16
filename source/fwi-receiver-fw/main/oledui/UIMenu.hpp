#pragma once

#include "UIBase.hpp"

class UIMenu : UIBase
{

    enum class MenuItem
    {
        Exit = 0,
        Settings,
        TestConn,
        LiveCheckContinuity,
        Reboot,
        // About,

        Count,
    };

    typedef struct
    {
        const char* name;
    } SMenu;

    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:

    bool m_isNeedRefresh = false;
    int32_t m_menuItemIndex = 0;
    const SMenu m_sMenuItems[(int)MenuItem::Count] =
    {
        [(int)MenuItem::Exit]            = { .name = "Exit" },
        [(int)MenuItem::Settings]        = { .name = "Settings" },
        [(int)MenuItem::TestConn]        = { .name = "Test conn." },
        [(int)MenuItem::LiveCheckContinuity] = { .name = "Chk. conn." },
        [(int)MenuItem::Reboot]          = { .name = "Reboot" },
        //[(int)MenuItem::About]           = { .name = "About" },
    };
    static_assert((int)MenuItem::Count == (sizeof(m_sMenuItems)/sizeof(m_sMenuItems[0])), "Menu items doesn't match");

    int32_t m_encoderTicks = 0;
};
