#include "UIArmed.h"

void UIARMED_Enter()
{
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    int32_t s32EncoderCount = HARDWAREGPIO_GetEncoderCount();

    sprintf(szText, "ARMED AND\nDANGEROUS\nPower: %"PRIi32" %%",
        s32EncoderCount);

    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
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
