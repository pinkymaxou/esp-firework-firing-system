#pragma once

#include "UIBase.hpp"

class UIErrorPleaseDisarm : UIBase
{
    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent btn_event, int32_t click_count) override;

    void OnTick(void) override;
};
