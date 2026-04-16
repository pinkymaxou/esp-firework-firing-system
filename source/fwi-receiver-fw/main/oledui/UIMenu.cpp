#include <algorithm>
#include "UIMenu.hpp"
#include "UIManager.hpp"
#include "../MainApp.hpp"
#include "esp_log.h"
#include "esp_system.h"

#define TAG "UIMenu"

void UIMenu::OnEnter()
{
    m_menu_item_index = 0;
    m_is_need_refresh = true;
}

void UIMenu::OnExit()
{

}

void UIMenu::OnTick()
{
    if (m_is_need_refresh)
    {
        m_is_need_refresh = false;
        DrawScreen();
    }
}

void UIMenu::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    switch (btn_event)
    {
        case UIBase::BTEvent::EncoderClick:
        {
            m_encoder_ticks += click_count;

            if (m_encoder_ticks <= -2)
            {
                if (m_menu_item_index > 0)
                {
                    m_menu_item_index--;
                    m_is_need_refresh = true;
                }
                m_encoder_ticks = 0;
            }
            else if (m_encoder_ticks >= 2)
            {
                if (m_menu_item_index + 1 < (int32_t)MenuItem::Count)
                {
                    m_menu_item_index++;
                    m_is_need_refresh = true;
                }
                m_encoder_ticks = 0;
            }
            break;
        }
        case UIBase::BTEvent::Click:
        {
            switch((MenuItem)m_menu_item_index)
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
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(display);

    // Cursor
    const int32_t MAX_DISPLAY_ITEM_COUNT = 4;

    int32_t start_menu_index = 0;
    if (m_menu_item_index > MAX_DISPLAY_ITEM_COUNT - 1)
    {
        start_menu_index = ((m_menu_item_index + 1) - MAX_DISPLAY_ITEM_COUNT);
    }

    int32_t draw_menu_index = 0;
    for(int32_t i = start_menu_index; i < (int32_t)MenuItem::Count && draw_menu_index < MAX_DISPLAY_ITEM_COUNT; i++)
    {
        const SMenu* menu_item = &m_menu_items[i];

        const bool is_selected = (i == m_menu_item_index);
        if (is_selected)
        {
            SSD1306_FillRect(display, 0, draw_menu_index*15+3, 127, 16, true);
        }
        SSD1306_SetTextColor(display, !is_selected);
        SSD1306_DrawString(display, 15, 15*draw_menu_index, menu_item->name);

        draw_menu_index++;
    }

    // Restore ...
    SSD1306_SetTextColor(display, true);
    SSD1306_UpdateDisplay(display);
    #endif
}
