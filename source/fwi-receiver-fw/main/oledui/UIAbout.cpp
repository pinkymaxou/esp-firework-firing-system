#include "UIAbout.hpp"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UIAbout::onEnter()
{
    drawScreen();
}

void UIAbout::onExit()
{

}

void UIAbout::onTick()
{

}

void UIAbout::onEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIAbout::drawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
    char text[65];

    display->clearDisplay();
    sprintf(text, "coucou");
    display->drawString( 15, 4, text);

    display->updateDisplay();
    #endif
}
