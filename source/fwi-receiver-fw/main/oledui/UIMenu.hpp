#pragma once

#include "UIBase.hpp"

class UIMenu : UIBase
{
    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount) override;

    void OnTick(void) override;
};
