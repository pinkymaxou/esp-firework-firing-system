#include "UIErrorPleaseDisarm.hpp"
#include "assets/BitmapPotato.h"

void UIErrorPleaseDisarm::OnEnter()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    char text[129] = {0,};

    sprintf(text, "PLEASE\nDISARM");

    SSD1306_ClearDisplay(display);
    memcpy(display->buffer, m_u8AlertDatas, m_u32AlertDataLen);
    SSD1306_DrawString(display, 55, 0, text);
    SSD1306_UpdateDisplay(display);
    #endif
}

void UIErrorPleaseDisarm::OnExit()
{

}

void UIErrorPleaseDisarm::OnTick()
{

}

void UIErrorPleaseDisarm::OnEncoderMove(BTEvent btn_event, int32_t click_count)
{

}
