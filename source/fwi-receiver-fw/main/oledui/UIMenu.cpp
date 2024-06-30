#include <algorithm>
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
                    g_uiMgr.Goto(UIManager::EMenu::TestConn);
                    break;
                case MenuItem::LiveCheckContinuity:
                    g_uiMgr.Goto(UIManager::EMenu::LiveCheckContinuity);
                    break;
                case MenuItem::Reboot:
                    ESP_LOGI(TAG, "Rebooting ...");
                    esp_restart();
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
    const int32_t MAX_DISPLAY_ITEM_COUNT = 4;

    int32_t s32StartMenuIndex = 0;
    if (m_s32MenuItemIndex > MAX_DISPLAY_ITEM_COUNT - 1) {
        s32StartMenuIndex = ((m_s32MenuItemIndex + 1) - MAX_DISPLAY_ITEM_COUNT);
    }

    int32_t s32DrawMenuIndex = 0;
    for(int32_t i = s32StartMenuIndex; i < (int32_t)MenuItem::Count && s32DrawMenuIndex < MAX_DISPLAY_ITEM_COUNT; i++)
    {
        const SMenu* psMenuItem = &m_sMenuItems[i];

        const bool isSelected = (i == m_s32MenuItemIndex);
        if (isSelected) {
            SSD1306_FillRect(pss1306Handle, 0, s32DrawMenuIndex*15+3, 127, 16, true);
        }
        SSD1306_SetTextColor(pss1306Handle, !isSelected);
        SSD1306_DrawString(pss1306Handle, 15, 15*s32DrawMenuIndex, psMenuItem->szName);

        s32DrawMenuIndex++;
    }

    // Restore ...
    SSD1306_SetTextColor(pss1306Handle, true);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
