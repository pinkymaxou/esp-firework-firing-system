#pragma once

#include "UIBase.hpp"

class UILiveCheckContinuity : UIBase
{
    void OnEnter(void) override;
    void OnExit(void) override;

    void OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount) override;

    void OnTick(void) override;

    void DrawScreen(void) override;

    private:
    TickType_t m_ttLastChangeTicks = 0;
};
