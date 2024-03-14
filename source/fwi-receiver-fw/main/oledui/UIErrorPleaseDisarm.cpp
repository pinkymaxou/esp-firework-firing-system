#include "UIErrorPleaseDisarm.hpp"
#include "assets/BitmapPotato.h"

void UIERRORPLEASEDISARM_Enter()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    sprintf(szText, "PLEASE\nDISARM");

    SSD1306_ClearDisplay(pss1306Handle);
    memcpy(pss1306Handle->u8Buffer, m_u8AlertDatas, m_u32AlertDataLen);
    SSD1306_DrawString(pss1306Handle, 55, 0, szText);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}

void UIERRORPLEASEDISARM_Exit()
{

}

void UIERRORPLEASEDISARM_Tick()
{

}

void UIERRORPLEASEDISARM_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{

}
