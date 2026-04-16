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
    m_ttLastChangeTicks = 0;
    m_isWifiStationActivated = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive);
    DrawScreen();
}

void UIHome::OnExit()
{

}

void UIHome::OnTick()
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(1000) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
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
        esp_netif_ip_info_t wifiIpSta = {0};
        MAIN_GetWiFiSTAIP(&wifiIpSta);
        sprintf(text, "%s\n" IPSTR,
            ssid,
            IP2STR(&wifiIpSta.ip));
    }
    else
    {
        esp_netif_ip_info_t wifiIpAP = {0};
        MAIN_GetWiFiSoftAPIP(&wifiIpAP);
        sprintf(text, "%s\n" IPSTR "\nUser: %" PRId32,
            ssid,
            IP2STR(&wifiIpAP.ip),
            MAIN_GetSAPUserCount());
    }

    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, text);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}
