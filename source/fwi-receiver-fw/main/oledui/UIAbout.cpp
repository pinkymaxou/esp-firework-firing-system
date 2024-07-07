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

void UIAbout::OnEncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UIBase::BTEvent::Click)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIAbout::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[64+1];

    SSD1306_ClearDisplay(pss1306Handle);
    sprintf(szText, "coucou");
    SSD1306_DrawString(pss1306Handle, 15, 4, szText);

    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}