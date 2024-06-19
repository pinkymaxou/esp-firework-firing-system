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

    void OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:

    Item m_eSettingItem = Item::PWM;
    int32_t m_s32FiringPWMPercValue = 5;
    int32_t m_s32FiringHoldTimeMS = 750;
    int32_t m_s32IsWifiStationActivated;
};
