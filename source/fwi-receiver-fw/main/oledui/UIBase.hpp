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

    virtual void OnEnter(void)
    {
    }

    virtual void OnExit(void)
    {
    }

    virtual void OnEncoderMove(BTEvent btn_event, int32_t click_count)
    {
    }

    virtual void OnTick(void)
    {
    }

    virtual void DrawScreen(void)
    {
    }
};
