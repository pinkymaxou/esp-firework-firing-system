#include "UITestConn.hpp"
#include "assets/BitmapPotato.h"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UITestConn::onEnter()
{
    g_app.ExecTestConnections();
    drawScreen();
}

void UITestConn::onExit()
{
    g_app.ExecCancel();
}

void UITestConn::onTick()
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(500) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        drawScreen();
    }
}

void UITestConn::onEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UITestConn::drawScreen()
{
    // All ignitor slots status
    uint32_t conn_count = 0;

    for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        const MainApp::SRelay relay = g_app.GetRelayState(i);
        if (relay.isConnected)
            conn_count++;
    }

    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
    char text[65];

    display->clearDisplay();
    sprintf(text, "Testing. conn\n%" PRIu32 " / %" PRIu32, conn_count, (uint32_t)HWCONFIG_OUTPUT_COUNT);
    display->drawString( 15, 4, text);

    const int32_t width = 128 - 16*2;
    const double of_one = g_app.GetProgress();
    if (of_one > 0.0d && of_one < 1.0d)
    {
        const int32_t bar_width = (int32_t)(of_one*width);
        display->fillRect( 16, 64 - 16, bar_width, 16, true);
        display->drawRect( 16, 64 - 16, width, 16, true);
    }
    display->updateDisplay();
    #endif
}
