#include "UIAbout.hpp"
#include "UIManager.hpp"
#include "MainApp.hpp"

void UIAbout::OnEnter()
{
    DrawScreen();
}

void UIAbout::OnExit()
{

}

void UIAbout::OnTick()
{

}

void UIAbout::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIAbout::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    char text[65];

    SSD1306_ClearDisplay(display);
    sprintf(text, "coucou");
    SSD1306_DrawString(display, 15, 4, text);

    SSD1306_UpdateDisplay(display);
    #endif
}
