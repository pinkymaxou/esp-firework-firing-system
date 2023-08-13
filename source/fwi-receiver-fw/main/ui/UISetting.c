#include <inttypes.h>
#include "UISetting.h"
#include "UIManager.h"

static void DrawScreen();

static int32_t m_s32Value = 25;

void UISETTING_Enter()
{
    DrawScreen();
}

void UISETTING_Exit()
{

}

void UISETTING_Tick()
{

}

void UISETTING_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UICORE_EBTNEVENT_Click)
        UIMANAGER_Goto(UIMANAGER_EMENU_Home);
    else if (eBtnEvent == UICORE_EBTNEVENT_EncoderClick)
    {
        const int32_t s32Value = m_s32Value + s32ClickCount;
        if (s32Value >= 5 && s32Value <= 100)
        {
            m_s32Value = s32Value;
            DrawScreen();
        }
    }
}

static void DrawScreen()
{
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    sprintf(szText, "PWM\n%"PRId32"%%",
        m_s32Value);
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
    SSD1306_UpdateDisplay(pss1306Handle);
}