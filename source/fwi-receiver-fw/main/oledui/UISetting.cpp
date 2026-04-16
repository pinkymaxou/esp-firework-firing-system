#include <inttypes.h>
#include "UISetting.hpp"
#include "UIManager.hpp"
#include "../Settings.hpp"

void UISetting::onEnter()
{
    m_firing_pwm_perc_value = (int32_t)NVSJSON_GetValueInt32(&Settings::g_handle, Settings::FiringPWMPercent);
    m_firing_hold_time_ms = NVSJSON_GetValueInt32(&Settings::g_handle, Settings::FiringHoldTimeMS);
    m_is_wifi_station_activated = NVSJSON_GetValueInt32(&Settings::g_handle, Settings::WSTAIsActive);

    m_setting_item = Item::PWM;

    drawScreen();
}

void UISetting::onExit()
{
    NVSJSON_SetValueInt32(&Settings::g_handle, Settings::FiringPWMPercent, false, m_firing_pwm_perc_value);
    NVSJSON_SetValueInt32(&Settings::g_handle, Settings::FiringHoldTimeMS, false, m_firing_hold_time_ms);
    NVSJSON_SetValueInt32(&Settings::g_handle, Settings::WSTAIsActive, false, m_is_wifi_station_activated);
    NVSJSON_Save(&Settings::g_handle);
}

void UISetting::onTick()
{

}

void UISetting::onEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    switch(m_setting_item)
    {
        case Item::PWM:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_setting_item = Item::HoldTimeMS;
                drawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firing_pwm_perc_value + click_count;
                if (value >= Settings::FIRINGPWMPERCENT_MIN && value <= Settings::FIRINGPWMPERCENT_MAX)
                {
                    m_firing_pwm_perc_value = value;
                    drawScreen();
                }
            }
            break;
        }
        case Item::HoldTimeMS:
        {
            if (UIBase::BTEvent::Click == btn_event)
            {
                m_setting_item = Item::WiFiStation;
                drawScreen();
            }
            else if (UIBase::BTEvent::EncoderClick == btn_event)
            {
                const int32_t value = m_firing_hold_time_ms + click_count*50;
                if (value >= Settings::FIRINGHOLDTIMEMS_MIN && value <= Settings::FIRINGHOLDTIMEMS_MAX)
                {
                    m_firing_hold_time_ms = value;
                    drawScreen();
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
                    drawScreen();
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

void UISetting::drawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
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
    display->clearDisplay();
    display->drawString( 0, 0, text);
    display->updateDisplay();
    #endif
}
