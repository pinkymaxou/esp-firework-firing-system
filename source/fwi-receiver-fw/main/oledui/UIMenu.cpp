#include <algorithm>
#include "UIMenu.hpp"
#include "UIManager.hpp"
#include "../MainApp.hpp"
#include "esp_log.h"
#include "esp_system.h"

#define TAG "UIMenu"

void UIMenu::OnEnter()
{
    m_menuItemIndex = 0;
    m_isNeedRefresh = true;
}

void UIMenu::OnExit()
{

}

void UIMenu::OnTick()
{
    if (m_isNeedRefresh)
    {
        m_isNeedRefresh = false;
        DrawScreen();
    }
}

void UIMenu::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    switch (btn_event)
    {
        case UIBase::BTEvent::EncoderClick:
        {
            m_encoderTicks += click_count;

            if (m_encoderTicks <= -2)
            {
                if (m_menuItemIndex > 0)
                {
                    m_menuItemIndex--;
                    m_isNeedRefresh = true;
                }
                m_encoderTicks = 0;
            }
            else if (m_encoderTicks >= 2)
            {
                if (m_menuItemIndex + 1 < (int32_t)MenuItem::Count)
                {
                    m_menuItemIndex++;
                    m_isNeedRefresh = true;
                }
                m_encoderTicks = 0;
            }
            break;
        }
        case UIBase::BTEvent::Click:
        {
            switch((MenuItem)m_menuItemIndex)
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
                // case MenuItem::About:
                //     g_uiMgr.Goto(UIManager::EMenu::About);
                //     break;
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

    int32_t start_menu_index = 0;
    if (m_menuItemIndex > MAX_DISPLAY_ITEM_COUNT - 1)
    {
        start_menu_index = ((m_menuItemIndex + 1) - MAX_DISPLAY_ITEM_COUNT);
    }

    int32_t draw_menu_index = 0;
    for(int32_t i = start_menu_index; i < (int32_t)MenuItem::Count && draw_menu_index < MAX_DISPLAY_ITEM_COUNT; i++)
    {
        const SMenu* psMenuItem = &m_sMenuItems[i];

        const bool isSelected = (i == m_menuItemIndex);
        if (isSelected)
        {
            SSD1306_FillRect(pss1306Handle, 0, draw_menu_index*15+3, 127, 16, true);
        }
        SSD1306_SetTextColor(pss1306Handle, !isSelected);
        SSD1306_DrawString(pss1306Handle, 15, 15*draw_menu_index, psMenuItem->name);

        draw_menu_index++;
    }

    // Restore ...
    SSD1306_SetTextColor(pss1306Handle, true);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
