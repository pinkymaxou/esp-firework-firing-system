#include <inttypes.h>
#include "UISetting.hpp"
#include "UIManager.hpp"
#include "../Settings.hpp"

void UISetting::OnEnter()
{
    m_firingPWMPercValue = (int32_t)NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringPWMPercent);
    m_firingHoldTimeMS = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    m_isWifiStationActivated = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive);

    m_eSettingItem = Item::PWM;

    DrawScreen();
}

void UISetting::OnExit()
{
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringPWMPercent, false, m_firingPWMPercValue);
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringHoldTimeMS, false, m_firingHoldTimeMS);
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive, false, m_isWifiStationActivated);
    NVSJSON_Save(&g_settingHandle);
}

void UISetting::OnTick()
{

}

void UISetting::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    switch(m_eSettingItem)
    {
        case Item::PWM:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_eSettingItem = Item::HoldTimeMS;
                DrawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firingPWMPercValue + click_count;
                if (value >= SETTINGS_FIRINGPWMPERCENT_MIN && value <= SETTINGS_FIRINGPWMPERCENT_MAX)
                {
                    m_firingPWMPercValue = value;
                    DrawScreen();
                }
            }
            break;
        }
        case Item::HoldTimeMS:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_eSettingItem = Item::WiFiStation;
                DrawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firingHoldTimeMS + click_count*50;
                if (value >= SETTINGS_FIRINGHOLDTIMEMS_MIN && value <= SETTINGS_FIRINGHOLDTIMEMS_MAX)
                {
                    m_firingHoldTimeMS = value;
                    DrawScreen();
                }
            }
            break;
        }
        case Item::WiFiStation:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                g_uiMgr.Goto(UIManager::EMenu::Menu);
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                m_boolCounter += click_count;

                int32_t new_value = m_isWifiStationActivated;
                if (m_boolCounter < -2)
                {
                    new_value = 0;
                }
                else if (m_boolCounter > 2)
                {
                    new_value = 1;
                }

                if (new_value != m_isWifiStationActivated)
                {
                    m_boolCounter = 0;
                    m_isWifiStationActivated = new_value;
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

void UISetting::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    char text[129] = {0,};

    if (Item::PWM == m_eSettingItem)
    {
        sprintf(text, "SETTINGS\nPWM\n%" PRId32 "%%",
            m_firingPWMPercValue);
    }
    else if (Item::HoldTimeMS == m_eSettingItem)
    {
        sprintf(text, "SETTINGS\nHold time\n%" PRId32 " ms",
            m_firingHoldTimeMS);
    }
    else if (Item::WiFiStation == m_eSettingItem)
    {
        sprintf(text, "SETTINGS\nWi-Fi STA\n%s",
            (m_isWifiStationActivated ? "YES" : "NO"));
    }
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, text);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
