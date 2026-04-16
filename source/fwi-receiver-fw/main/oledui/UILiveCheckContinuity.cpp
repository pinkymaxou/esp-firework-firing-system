#include "UITestConn.hpp"
#include "assets/BitmapPotato.h"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UILiveCheckContinuity::OnEnter()
{
    g_app.ExecLiveCheckContinuity();
    DrawScreen();
}

void UILiveCheckContinuity::OnExit()
{
    g_app.ExecCancel();
}

void UILiveCheckContinuity::OnTick()
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(200) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        DrawScreen();
    }
}

void UILiveCheckContinuity::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UILiveCheckContinuity::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    char text[65];

    SSD1306_ClearDisplay(display);
    sprintf(text, "Test single\n#1: %s",
        g_app.GetContinuityTest() ? "YES" : "NO");
    SSD1306_DrawString(display, 15, 4, text);

    SSD1306_UpdateDisplay(display);
    #endif
}
