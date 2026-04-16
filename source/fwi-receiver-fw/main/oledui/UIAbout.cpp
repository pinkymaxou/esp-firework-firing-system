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
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char text[65];

    SSD1306_ClearDisplay(pss1306Handle);
    sprintf(text, "coucou");
    SSD1306_DrawString(pss1306Handle, 15, 4, text);

    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
