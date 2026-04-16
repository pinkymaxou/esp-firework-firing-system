#include "UITestConn.hpp"
#include "assets/BitmapPotato.h"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UITestConn::OnEnter()
{
    g_app.ExecCheckConnections();
    DrawScreen();
}

void UITestConn::OnExit()
{
    g_app.ExecCancel();
}

void UITestConn::OnTick()
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(500) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
        DrawScreen();
    }
}

void UITestConn::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UITestConn::DrawScreen()
{
    // All ignitor slots status
    uint32_t conn_count = 0;

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        const MainApp::SRelay sRelay = g_app.GetRelayState(i);
        if (sRelay.isConnected)
            conn_count++;
    }

    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char text[65];

    SSD1306_ClearDisplay(pss1306Handle);
    sprintf(text, "Testing. conn\n%" PRIu32 " / %" PRIu32, conn_count, (uint32_t)HWCONFIG_OUTPUT_COUNT);
    SSD1306_DrawString(pss1306Handle, 15, 4, text);

    const int32_t width = 128 - 16*2;
    const double of_one = g_app.GetProgress();
    if (of_one > 0.0d && of_one < 1.0d)
    {
        const int32_t bar_width = (int32_t)(of_one*width);
        SSD1306_FillRect(pss1306Handle, 16, 64 - 16, bar_width, 16, true);
        SSD1306_DrawRect(pss1306Handle, 16, 64 - 16, width, 16, true);
    }
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
