#include "UIArmed.h"

void UIARMED_Enter()
{
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    sprintf(szText, "ARMED AND\nDANGEROUS");

    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText);
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
