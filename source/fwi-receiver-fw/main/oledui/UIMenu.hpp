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

        Count,
    };

    typedef struct
    {
        const char* szName;
    } SMenu;

    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:

    bool m_bIsNeedRefresh = false;
    int32_t m_s32MenuItemIndex = 0;
    const SMenu m_sMenuItems[(int)MenuItem::Count] =
    {
        [(int)MenuItem::Exit]            = { .szName = "Exit" },
        [(int)MenuItem::Settings]        = { .szName = "Settings" },
        [(int)MenuItem::TestConn]        = { .szName = "Test conn." },
        [(int)MenuItem::LiveCheckContinuity] = { .szName = "Chk. conn." },
        [(int)MenuItem::Reboot]          = { .szName = "Reboot" },
    };
    static_assert((int)MenuItem::Count == ( sizeof(m_sMenuItems)/sizeof(m_sMenuItems[0])), "Menu items doesn't match");

    int32_t m_s32EncoderTicks = 0;
};
