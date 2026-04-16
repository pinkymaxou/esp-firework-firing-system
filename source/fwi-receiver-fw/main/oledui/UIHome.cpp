#include "UIHome.hpp"
#include "../main.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "UIManager.hpp"
#include "../Settings.hpp"

#define TAG "UIHOME"

void UIHome::onEnter()
{
    m_is_public_ip = false;
    m_last_change_ticks = 0;
    m_is_wifi_station_activated = NVSJSON_GetValueInt32(&Settings::g_handle, Settings::WSTAIsActive);
    drawScreen();
}

void UIHome::onExit()
{

}

void UIHome::onTick()
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(1000) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        drawScreen();
        m_is_public_ip = !m_is_public_ip;
    }
}

void UIHome::onEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIHome::drawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    char text[129] = {0,};
    char ssid[32] = {0,};

    Main::getWifiAPSSID(ssid);

    if (m_is_public_ip && m_is_wifi_station_activated)
    {
        esp_netif_ip_info_t wifi_ip_sta = {0};
        Main::getWiFiSTAIP(&wifi_ip_sta);
        sprintf(text, "%s\n" IPSTR,
            ssid,
            IP2STR(&wifi_ip_sta.ip));
    }
    else
    {
        esp_netif_ip_info_t wifi_ip_ap = {0};
        Main::getWiFiSoftAPIP(&wifi_ip_ap);
        sprintf(text, "%s\n" IPSTR "\nUser: %" PRId32,
            ssid,
            IP2STR(&wifi_ip_ap.ip),
            Main::getSAPUserCount());
    }

    SSD1306* display = HWGPIO::getSSD1306Handle();
    display->clearDisplay();
    display->drawString( 0, 0, text);
    display->updateDisplay();
    #endif
}
