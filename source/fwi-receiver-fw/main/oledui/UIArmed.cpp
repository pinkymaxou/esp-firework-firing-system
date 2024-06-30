#include "UIArmed.hpp"
#include "assets/BitmapPotato.h"

void UIArmed::OnEnter(void)
{
    //DrawScreen();
}

void UIArmed::OnExit(void)
{

}

void UIArmed::OnEncoderMove(BTEvent eBtnEvent, int32_t s32ClickCount)
{

}

void UIArmed::OnTick(void)
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(500) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
        DrawScreen();
        m_bAlernImage = !m_bAlernImage;
    }
}

void UIArmed::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    //SSD1306_ClearDisplay(pss1306Handle);
    if (m_bAlernImage)
        memcpy(pss1306Handle->u8Buffer, m_u8AlertDatas, m_u32AlertDataLen);
    else
        memcpy(pss1306Handle->u8Buffer, m_u8FireworkDatas, m_u32FireworkDataLen);
    SSD1306_DrawString(pss1306Handle, 60, 15, "ARMED");
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}