#include "UIHome.h"
#include "../main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "UIManager.h"

#define TAG "UIHOME"

static TickType_t m_ttLastChangeTicks = 0;
static bool m_bIsPublicIP = false;

static void DrawScreen();

void UIHOME_Enter()
{
    m_bIsPublicIP = false;
    m_ttLastChangeTicks = 0;
    DrawScreen();
}

void UIHOME_Exit()
{

}

void UIHOME_Tick()
{
    if ( (xTaskGetTickCount() - m_ttLastChangeTicks) > pdMS_TO_TICKS(1000) )
    {
        m_ttLastChangeTicks = xTaskGetTickCount();
        DrawScreen();
        m_bIsPublicIP = !m_bIsPublicIP;
    }
}

void UIHOME_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{
    if (eBtnEvent == UICORE_EBTNEVENT_Click)
        UIMANAGER_Goto(UIMANAGER_EMENU_Setting);
}

static void DrawScreen()
{
    char szText[128+1] = {0,};
    char szSoftAPSSID[32] = {0,};

    MAIN_GetWifiAPSSID(szSoftAPSSID);

    if (m_bIsPublicIP)
    {
        esp_netif_ip_info_t wifiIpSta = {0};
        MAIN_GetWiFiSTAIP(&wifiIpSta);

        sprintf(szText, "%s\n"IPSTR,
            szSoftAPSSID,
            IP2STR(&wifiIpSta.ip));
    }
    else
    {
        esp_netif_ip_info_t wifiIpAP = {0};
        MAIN_GetWiFiSoftAPIP(&wifiIpAP);

        sprintf(szText, "%s\n"IPSTR,
            szSoftAPSSID,
            IP2STR(&wifiIpAP.ip));
    }

    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
    SSD1306_UpdateDisplay(pss1306Handle);
}