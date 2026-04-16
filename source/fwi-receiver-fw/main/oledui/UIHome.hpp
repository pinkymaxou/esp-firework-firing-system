#pragma once

#include "UIBase.hpp"

class UIHome : UIBase
{
    void onEnter(void) override;
    void onExit(void) override;

    void onEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void onTick(void) override;

    void drawScreen(void) override;

    private:
    TickType_t m_last_change_ticks = 0;
    bool m_is_public_ip = false;
    bool m_is_wifi_station_activated = false;
};
