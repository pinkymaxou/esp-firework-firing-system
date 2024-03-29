#include "UIMenu.hpp"
#include "UIManager.hpp"
#include "../MainApp.hpp"
#include "esp_log.h"
#include "esp_system.h"

#define TAG "UIMenu"

typedef enum
{
    UIMENU_EMENUITEM_Exit = 0,
    UIMENU_EMENUITEM_Settings,
    UIMENU_EMENUITEM_TestConn,
    UIMENU_EMENUITEM_Reboot,

    UIMENU_EMENUITEM_Count,
} UIMENU_EMENUITEM;

typedef struct
{
    const char* szName;
} UIMENU_SMenu;

static bool m_bIsNeedRefresh = false;
static int32_t m_s32MenuItemIndex = 0;
static const UIMENU_SMenu m_sMenuItems[] =
{
    [UIMENU_EMENUITEM_Exit]         = { .szName = "Exit" },
    [UIMENU_EMENUITEM_Settings]     = { .szName = "Settings" },
    [UIMENU_EMENUITEM_TestConn]     = { .szName = "Test conn." },
    [UIMENU_EMENUITEM_Reboot]       = { .szName = "Reboot" },
};
static_assert((int)UIMENU_EMENUITEM_Count == ( sizeof(m_sMenuItems)/sizeof(m_sMenuItems[0])), "Menu items doesn't match");

static int32_t m_s32EncoderTicks = 0;

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
                if (m_s32MenuItemIndex + 1 < (int32_t)UIMENU_EMENUITEM_Count)
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
            switch((UIMENU_EMENUITEM)m_s32MenuItemIndex)
            {
                case UIMENU_EMENUITEM_Exit:
                    g_uiMgr.Goto(UIManager::EMenu::Home);
                    break;
                case UIMENU_EMENUITEM_Settings:
                    g_uiMgr.Goto(UIManager::EMenu::Setting);
                    break;
                case UIMENU_EMENUITEM_TestConn:
                    g_app.ExecCheckConnections();
                    g_uiMgr.Goto(UIManager::EMenu::TestConn);
                    break;
                case UIMENU_EMENUITEM_Reboot:
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

    for(int i = 0; i < UIMENU_EMENUITEM_Count; i++)
    {
        const UIMENU_SMenu* psMenuItem = &m_sMenuItems[i];

        SSD1306_SetTextColor(pss1306Handle, (i != m_s32MenuItemIndex));
        SSD1306_DrawString(pss1306Handle, 15, 15*i, psMenuItem->szName);
    }

    // Restore ...
    SSD1306_SetTextColor(pss1306Handle, true);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
