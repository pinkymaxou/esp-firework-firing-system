#include "UIArmed.h"
#include "assets/BitmapPotato.h"

void UIARMED_Enter()
{
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();

    SSD1306_ClearDisplay(pss1306Handle);
    memcpy(pss1306Handle->u8Buffer, m_u8FireworkDatas, m_u32FireworkDataLen);
    SSD1306_DrawString(pss1306Handle, 60, 15, "ARMED");
    SSD1306_UpdateDisplay(pss1306Handle);
}

void UIARMED_Exit()
{

}

void UIARMED_Tick()
{

}

void UIARMED_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{

}
