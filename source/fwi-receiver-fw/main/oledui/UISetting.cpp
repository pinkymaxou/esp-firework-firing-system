#include <inttypes.h>
#include "UISetting.hpp"
#include "UIManager.hpp"
#include "../Settings.hpp"

void UISetting::OnEnter()
{
    m_firing_pwm_perc_value = (int32_t)NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringPWMPercent);
    m_firing_hold_time_ms = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringHoldTimeMS);
    m_is_wifi_station_activated = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive);

    m_setting_item = Item::PWM;

    DrawScreen();
}

void UISetting::OnExit()
{
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringPWMPercent, false, m_firing_pwm_perc_value);
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringHoldTimeMS, false, m_firing_hold_time_ms);
    NVSJSON_SetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive, false, m_is_wifi_station_activated);
    NVSJSON_Save(&g_settingHandle);
}

void UISetting::OnTick()
{

}

void UISetting::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    switch(m_setting_item)
    {
        case Item::PWM:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_setting_item = Item::HoldTimeMS;
                DrawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firing_pwm_perc_value + click_count;
                if (value >= SETTINGS_FIRINGPWMPERCENT_MIN && value <= SETTINGS_FIRINGPWMPERCENT_MAX)
                {
                    m_firing_pwm_perc_value = value;
                    DrawScreen();
                }
            }
            break;
        }
        case Item::HoldTimeMS:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_setting_item = Item::WiFiStation;
                DrawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firing_hold_time_ms + click_count*50;
                if (value >= SETTINGS_FIRINGHOLDTIMEMS_MIN && value <= SETTINGS_FIRINGHOLDTIMEMS_MAX)
                {
                    m_firing_hold_time_ms = value;
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
                m_bool_counter += click_count;

                int32_t new_value = m_is_wifi_station_activated;
                if (m_bool_counter < -2)
                {
                    new_value = 0;
                }
                else if (m_bool_counter > 2)
                {
                    new_value = 1;
                }

                if (new_value != m_is_wifi_station_activated)
                {
                    m_bool_counter = 0;
                    m_is_wifi_station_activated = new_value;
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
    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    char text[129] = {0,};

    if (Item::PWM == m_setting_item)
    {
        sprintf(text, "SETTINGS\nPWM\n%" PRId32 "%%",
            m_firing_pwm_perc_value);
    }
    else if (Item::HoldTimeMS == m_setting_item)
    {
        sprintf(text, "SETTINGS\nHold time\n%" PRId32 " ms",
            m_firing_hold_time_ms);
    }
    else if (Item::WiFiStation == m_setting_item)
    {
        sprintf(text, "SETTINGS\nWi-Fi STA\n%s",
            (m_is_wifi_station_activated ? "YES" : "NO"));
    }
    SSD1306_ClearDisplay(display);
    SSD1306_DrawString(display, 0, 0, text);
    SSD1306_UpdateDisplay(display);
    #endif
}
