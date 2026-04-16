#include "UITestConn.hpp"
#include "assets/BitmapPotato.h"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UILiveCheckContinuity::onEnter()
{
    g_app.ExecLiveCheckContinuity();
    drawScreen();
}

void UILiveCheckContinuity::onExit()
{
    g_app.ExecCancel();
}

void UILiveCheckContinuity::onTick()
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(200) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        drawScreen();
    }
}

void UILiveCheckContinuity::onEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UILiveCheckContinuity::drawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
    char text[65];

    display->clearDisplay();
    sprintf(text, "Test single\n#1: %s",
        g_app.GetContinuityTest() ? "YES" : "NO");
    display->drawString( 15, 4, text);

    display->updateDisplay();
    #endif
}
