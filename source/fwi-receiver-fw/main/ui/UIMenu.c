#include "UIMenu.h"
#include "UIManager.h"
#include "../MainApp.h"
#include "esp_log.h"

#define TAG "UIMenu"

typedef enum
{
    UIMENU_EMENUITEM_Exit = 0,
    UIMENU_EMENUITEM_Settings,
    UIMENU_EMENUITEM_TestConn,

    UIMENU_EMENUITEM_Count,
} UIMENU_EMENUITEM;

static void DrawScreen();

static bool m_bIsNeedRefresh = false;
static int32_t m_s32MenuItemIndex = 0;

static int32_t m_s32EncoderTicks = 0;

void UIMENU_Enter()
{
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

    SSD1306_DrawString(pss1306Handle, 14, 14*0, "EXIT");
    SSD1306_DrawString(pss1306Handle, 14, 14*1, "SETTINGS");
    SSD1306_DrawString(pss1306Handle, 14, 14*2, "TEST CONN.");

    // Cursor
    SSD1306_DrawString(pss1306Handle, 0, m_s32MenuItemIndex*14, ">");

    SSD1306_UpdateDisplay(pss1306Handle);
}
