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

}

void UILiveCheckContinuity::OnTick()
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(500) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
        DrawScreen();
    }
}

void UILiveCheckContinuity::OnEncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UIBase::BTEvent::Click)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UILiveCheckContinuity::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[64+1];

    SSD1306_ClearDisplay(pss1306Handle);
    sprintf(szText, "Test continuity\noutput #1");
    SSD1306_DrawString(pss1306Handle, 15, 4, szText);

    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}