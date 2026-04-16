#include "UIArmed.hpp"
#include "assets/BitmapPotato.h"

void UIArmed::OnEnter(void)
{
    //DrawScreen();
}

void UIArmed::OnExit(void)
{

}

void UIArmed::OnEncoderMove(BTEvent btn_event, int32_t click_count)
{

}

void UIArmed::OnTick(void)
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(500) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        DrawScreen();
        m_alternImage = !m_alternImage;
    }
}

void UIArmed::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    //SSD1306_ClearDisplay(display);
    if (m_alternImage)
        memcpy(display->buffer, m_u8AlertDatas, m_u32AlertDataLen);
    else
        memcpy(display->buffer, m_u8FireworkDatas, m_u32FireworkDataLen);
    SSD1306_DrawString(display, 60, 15, "ARMED");
    SSD1306_UpdateDisplay(display);
    #endif
}
