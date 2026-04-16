#pragma once

#include <cstdint>
#include "../HWGPIO.hpp"

class UIBase
{
    public:
    enum class BTEvent
    {
        Click,
        EncoderClick
    };

    virtual void onEnter(void)
    {
    }

    virtual void onExit(void)
    {
    }

    virtual void onEncoderMove(BTEvent btn_event, int32_t click_count)
    {
    }

    virtual void onTick(void)
    {
    }

    virtual void drawScreen(void)
    {
    }
};
