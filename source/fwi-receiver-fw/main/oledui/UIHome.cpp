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
    m_bIsPublicIP = false;
    m_ttLastChangeTicks = 0;
    m_s32IsWifiStationActivated = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive);
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
        m_bIsPublicIP = !m_bIsPublicIP;
    }
}

void UIHome::OnEncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UIBase::BTEvent::Click) {
        g_uiMgr.Goto(UIManager::EMenu::Menu);
    }
}

void UIHome::DrawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    char szText[128+1] = {0,};
    char szSoftAPSSID[32] = {0,};

    MAIN_GetWifiAPSSID(szSoftAPSSID);

    if (m_bIsPublicIP && m_s32IsWifiStationActivated)
    {
        esp_netif_ip_info_t wifiIpSta = {0};
        MAIN_GetWiFiSTAIP(&wifiIpSta);
        sprintf(szText, "%s\n" IPSTR,
            szSoftAPSSID,
            IP2STR(&wifiIpSta.ip));
    }
    else
    {
        esp_netif_ip_info_t wifiIpAP = {0};
        MAIN_GetWiFiSoftAPIP(&wifiIpAP);
        sprintf(szText, "%s\n" IPSTR "\nUser: %" PRId32,
            szSoftAPSSID,
            IP2STR(&wifiIpAP.ip),
            MAIN_GetSAPUserCount());
    }

    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText);
    SSD1306_UpdateDisplay(pss1306Handle);
    #endif
}