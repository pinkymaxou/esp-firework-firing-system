#include <inttypes.h>
#include "UISetting.hpp"
#include "UIManager.hpp"
#include "../Settings.hpp"

typedef enum
{
    ESETTING_ITEM_PWM = 0,
    ESETTING_ITEM_HoldTimeMS,

    ESETTING_ITEM_Count
} ESETTING_ITEM;

static void DrawScreen();

static ESETTING_ITEM m_eSettingItem = ESETTING_ITEM_PWM;
static int32_t m_s32FiringPWMPercValue = 5;
static int32_t m_s32FiringHoldTimeMS = 750;

void UISetting::OnEnter()
{
    m_s32FiringPWMPercValue = (int32_t)(NVSJSON_GetValueDouble(&g_sSettingHandle, SETTINGS_EENTRY_FiringPWMPercent) * (int32_t)100);
    m_s32FiringHoldTimeMS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);

    m_eSettingItem = ESETTING_ITEM_PWM;

    DrawScreen();
}

void UISetting::OnExit()
{
    NVSJSON_SetValueDouble(&g_sSettingHandle, SETTINGS_EENTRY_FiringPWMPercent, false, (double)m_s32FiringPWMPercValue/100.0d);
    NVSJSON_SetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_FiringHoldTimeMS, false, m_s32FiringHoldTimeMS);
    NVSJSON_Save(&g_sSettingHandle);
}

void UISetting::OnTick()
{

}

void UISetting::OnEncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    switch(m_eSettingItem)
    {
        case ESETTING_ITEM_PWM:
        {
            if (eBtnEvent == UIBase::BTEvent::Click)
            {
                m_eSettingItem = ESETTING_ITEM_HoldTimeMS;
                DrawScreen();
            }
            else if (eBtnEvent == UIBase::BTEvent::EncoderClick)
            {
                const int32_t s32Value = m_s32FiringPWMPercValue + s32ClickCount;
                if (s32Value >= 5 && s32Value <= 100)
                {
                    m_s32FiringPWMPercValue = s32Value;
                    DrawScreen();
                }
            }
            break;
        }
        case ESETTING_ITEM_HoldTimeMS:
        {
            if (eBtnEvent == UIBase::BTEvent::Click)
                g_uiMgr.Goto(UIManager::EMenu::Menu);
            else if (eBtnEvent == UIBase::BTEvent::EncoderClick)
            {
                const int32_t s32Value = m_s32FiringHoldTimeMS + s32ClickCount*50;
                if (s32Value >= 50 && s32Value <= 5000)
                {
                    m_s32FiringHoldTimeMS = s32Value;
                    DrawScreen();
                }
            }
            break;
        }
        default:
        {
            g_uiMgr.Goto(UIManager::EMenu::Home);
            break;
        }
    }
}

static void DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char szText[128+1] = {0,};

    if (m_eSettingItem == ESETTING_ITEM_PWM)
    {
        sprintf(szText, "SETTINGS\nPWM\n%"PRId32"%%",
            m_s32FiringPWMPercValue);
    }
    else if (m_eSettingItem == ESETTING_ITEM_HoldTimeMS)
    {
        sprintf(szText, "SETTINGS\nHold time\n%"PRId32" ms",
            m_s32FiringHoldTimeMS);
    }
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}