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

    void onEnter(void) override;
    void onExit(void) override;

    void onEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void onTick(void) override;

    void drawScreen(void) override;

    private:

    bool m_is_need_refresh = false;
    int32_t m_menu_item_index = 0;
    const SMenu m_menu_items[(int)MenuItem::Count] =
    {
        [(int)MenuItem::Exit]            = { .name = "Exit" },
        [(int)MenuItem::Settings]        = { .name = "Settings" },
        [(int)MenuItem::TestConn]        = { .name = "Test conn." },
        [(int)MenuItem::LiveCheckContinuity] = { .name = "Test single" },
        [(int)MenuItem::Reboot]          = { .name = "Reboot" },
        //[(int)MenuItem::About]           = { .name = "About" },
    };
    static_assert((int)MenuItem::Count == (sizeof(m_menu_items)/sizeof(m_menu_items[0])), "Menu items doesn't match");

    int32_t m_encoder_ticks = 0;
};
