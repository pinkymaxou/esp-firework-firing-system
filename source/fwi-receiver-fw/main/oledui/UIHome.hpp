#pragma once

#include "UIBase.hpp"

class UIHome : UIBase
{
    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:
    TickType_t m_last_change_ticks = 0;
    bool m_isPublicIP = false;
    bool m_isWifiStationActivated = false;
};
