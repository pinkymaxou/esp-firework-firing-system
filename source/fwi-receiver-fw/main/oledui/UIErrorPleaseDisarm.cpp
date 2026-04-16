#include "UIErrorPleaseDisarm.hpp"
#include "assets/BitmapPotato.h"
#include <string.h>

void UIErrorPleaseDisarm::onEnter()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
    char text[129] = {0,};

    sprintf(text, "PLEASE\nDISARM");

    display->clearDisplay();
    memcpy(display->m_buffer, g_alert_data, g_alert_data_len);
    display->drawString( 55, 0, text);
    display->updateDisplay();
    #endif
}

void UIErrorPleaseDisarm::onExit()
{

}

void UIErrorPleaseDisarm::onTick()
{

}

void UIErrorPleaseDisarm::onEncoderMove(BTEvent btn_event, int32_t click_count)
{

}
