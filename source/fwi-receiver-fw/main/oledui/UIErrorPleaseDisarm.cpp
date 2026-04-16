#include "UIErrorPleaseDisarm.hpp"
#include "assets/BitmapPotato.h"

void UIErrorPleaseDisarm::OnEnter()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char text[129] = {0,};

    sprintf(text, "PLEASE\nDISARM");

    SSD1306_ClearDisplay(pss1306Handle);
    memcpy(pss1306Handle->buffer, m_u8AlertDatas, m_u32AlertDataLen);
    SSD1306_DrawString(pss1306Handle, 55, 0, text);
    SSD1306_UpdateDisplay(pss1306Handle);
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
