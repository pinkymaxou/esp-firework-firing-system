#include "UIMenu.hpp"
#include "UIManager.hpp"
#include "../MainApp.hpp"
#include "esp_log.h"
#include "esp_system.h"

#define TAG "UIMenu"

void UIMenu::OnEnter()
{
    m_s32MenuItemIndex = 0;
    m_bIsNeedRefresh = true;
}

void UIMenu::OnExit()
{

}

void UIMenu::OnTick()
{
    if (m_bIsNeedRefresh)
    {
        m_bIsNeedRefresh = false;
        DrawScreen();
    }
}

void UIMenu::OnEncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    switch (eBtnEvent)
    {
        case UIBase::BTEvent::EncoderClick:
        {
            m_s32EncoderTicks += s32ClickCount;

            if (m_s32EncoderTicks <= -2)
            {
                if (m_s32MenuItemIndex > 0)
                {
                    m_s32MenuItemIndex--;
                    m_bIsNeedRefresh = true;
                }
                m_s32EncoderTicks = 0;
            }
            else if (m_s32EncoderTicks >= 2)
            {
                if (m_s32MenuItemIndex + 1 < (int32_t)MenuItem::Count)
                {
                    m_s32MenuItemIndex++;
                    m_bIsNeedRefresh = true;
                }
                m_s32EncoderTicks = 0;
            }
            break;
        }
        case UIBase::BTEvent::Click:
        {
            switch((MenuItem)m_s32MenuItemIndex)
            {
                case MenuItem::Exit:
                    g_uiMgr.Goto(UIManager::EMenu::Home);
                    break;
                case MenuItem::Settings:
                    g_uiMgr.Goto(UIManager::EMenu::Setting);
                    break;
                case MenuItem::TestConn:
                    g_app.ExecCheckConnections();
                    g_uiMgr.Goto(UIManager::EMenu::TestConn);
                    break;
                case MenuItem::Reboot:
                    ESP_LOGI(TAG, "Rebooting ...");
                    esp_restart();
                    // MAINAPP_ExecFullOutputCalibration();
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

void UIMenu::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(pss1306Handle);

    // Cursor
    SSD1306_FillRect(pss1306Handle, 0, m_s32MenuItemIndex*15+3, 127, 16, true);

    for(int i = 0; i < (int)MenuItem::Count; i++)
    {
        const SMenu* psMenuItem = &m_sMenuItems[i];

        SSD1306_SetTextColor(pss1306Handle, (i != m_s32MenuItemIndex));
        SSD1306_DrawString(pss1306Handle, 15, 15*i, psMenuItem->szName);
    }

    // Restore ...
    SSD1306_SetTextColor(pss1306Handle, true);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
