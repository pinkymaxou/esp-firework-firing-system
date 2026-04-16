#include "UIHome.hpp"
#include "../main.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "UIManager.hpp"
#include "../Settings.hpp"

#define TAG "UIHOME"

void UIHome::OnEnter()
{
    m_isPublicIP = false;
    m_last_change_ticks = 0;
    m_isWifiStationActivated = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive);
    DrawScreen();
}

void UIHome::OnExit()
{

}

void UIHome::OnTick()
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(1000) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        DrawScreen();
        m_isPublicIP = !m_isPublicIP;
    }
}

void UIHome::OnEncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    if (UIBase::BTEvent::Click == btn_event)
    {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIHome::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    char text[129] = {0,};
    char ssid[32] = {0,};

    MAIN_GetWifiAPSSID(ssid);

    if (m_isPublicIP && m_isWifiStationActivated)
    {
        esp_netif_ip_info_t wifi_ip_sta = {0};
        MAIN_GetWiFiSTAIP(&wifi_ip_sta);
        sprintf(text, "%s\n" IPSTR,
            ssid,
            IP2STR(&wifi_ip_sta.ip));
    }
    else
    {
        esp_netif_ip_info_t wifi_ip_ap = {0};
        MAIN_GetWiFiSoftAPIP(&wifi_ip_ap);
        sprintf(text, "%s\n" IPSTR "\nUser: %" PRId32,
            ssid,
            IP2STR(&wifi_ip_ap.ip),
            MAIN_GetSAPUserCount());
    }

    SSD1306_handle* display = HWGPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(display);
    SSD1306_DrawString(display, 0, 0, text);
    SSD1306_UpdateDisplay(display);
    #endif
}
