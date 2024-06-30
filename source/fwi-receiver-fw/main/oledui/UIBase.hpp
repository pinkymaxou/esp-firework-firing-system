#pragma once

#include <cstdint>
#include "../HWGPIO.hpp"

class UIBase
{
    public:
    enum BTEvent
    {
        Click,
        EncoderClick
    };

    virtual void OnEnter(void) { };
    virtual void OnExit(void) { };

    virtual void OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount) { };

    virtual void OnTick(void) { };

    virtual void DrawScreen(void) { };
};