#include "UIMenu.h"
#include "UIManager.h"
#include "../MainApp.h"
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

static void DrawScreen();

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

void UIMENU_Enter()
{
    m_s32MenuItemIndex = 0;
    m_bIsNeedRefresh = true;
}

void UIMENU_Exit()
{

}

void UIMENU_Tick()
{
    if (m_bIsNeedRefresh)
    {
        m_bIsNeedRefresh = false;
        DrawScreen();
    }
}

void UIMENU_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{
    switch (eBtnEvent)
    {
        case UICORE_EBTNEVENT_EncoderClick:
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
        case UICORE_EBTNEVENT_Click:
        {
            switch((UIMENU_EMENUITEM)m_s32MenuItemIndex)
            {
                case UIMENU_EMENUITEM_Exit:
                    UIMANAGER_Goto(UIMANAGER_EMENU_Home);
                    break;
                case UIMENU_EMENUITEM_Settings:
                    UIMANAGER_Goto(UIMANAGER_EMENU_Setting);
                    break;
                case UIMENU_EMENUITEM_TestConn:
                    MAINAPP_ExecCheckConnections();
                    UIMANAGER_Goto(UIMANAGER_EMENU_TestConn);
                    break;
                case UIMENU_EMENUITEM_Reboot:
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

static void DrawScreen()
{
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
}
