#pragma once

#include "UIBase.hpp"

class UIArmed : UIBase
{
    void onEnter(void) override;
    void onExit(void) override;

    void onEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void onTick(void) override;

    void drawScreen(void) override;

    private:
    TickType_t m_last_change_ticks = 0;
    bool m_altern_image = false;
};
