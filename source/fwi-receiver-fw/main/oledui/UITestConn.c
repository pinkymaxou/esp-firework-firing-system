#include "UITestConn.h"
#include "assets/BitmapPotato.h"
#include "UIManager.hpp"
#include "MainApp.hpp"

static TickType_t m_ttLastChangeTicks = 0;

static void DrawScreen();

void UITESTCONN_Enter()
{
    DrawScreen();
}

void UITESTCONN_Exit()
{

}

void UITESTCONN_Tick()
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(500) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
        DrawScreen();
    }
}

void UITESTCONN_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UICORE_EBTNEVENT_Click)
    {
        UIMANAGER_Goto(UIMANAGER_EMENU_Menu);
    }
}

static void DrawScreen()
{
    // All ignitor slots status
    uint32_t u32ConnCount = 0;

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        const MAINAPP_SRelay sRelay = MAINAPP_GetRelayState(i);
        if (sRelay.isConnected)
            u32ConnCount++;
    }

    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[64+1];

    SSD1306_ClearDisplay(pss1306Handle);
    sprintf(szText, "Testing. conn\n%" PRIu32 " / %" PRIu32, u32ConnCount, (uint32_t)HWCONFIG_OUTPUT_COUNT);
    SSD1306_DrawString(pss1306Handle, 15, 4, szText);

    const int32_t s32Width = 128 - 16*2;
    const double ofOne = MAINAPP_GetProgress();
    if (ofOne > 0.0d && ofOne < 1.0d)
    {
        const int32_t s32BarWidth = (int32_t)(ofOne*s32Width);
        SSD1306_FillRect(pss1306Handle, 16, 64 - 16, s32BarWidth, 16, true);
        SSD1306_DrawRect(pss1306Handle, 16, 64 - 16, s32Width, 16, true);
    }
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}