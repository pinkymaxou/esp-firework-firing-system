#pragma once

#include "UIBase.hpp"

class UISetting : UIBase
{

    enum class Item
    {
        PWM = 0,
        HoldTimeMS,
        WiFiStation,

        Count
    };

    void onEnter(void) override;
    void onExit(void) override;

    void onEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void onTick(void) override;

    void drawScreen(void) override;

    private:

    Item m_setting_item = Item::PWM;
    int32_t m_firing_pwm_perc_value = 5;
    int32_t m_firing_hold_time_ms = 750;
    int32_t m_is_wifi_station_activated = false;
    int32_t m_bool_counter = 0;
};
