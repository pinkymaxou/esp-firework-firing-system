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

    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:

    Item m_eSettingItem = Item::PWM;
    int32_t m_firingPWMPercValue = 5;
    int32_t m_firingHoldTimeMS = 750;
    int32_t m_isWifiStationActivated = false;
    int32_t m_boolCounter = 0;
};
