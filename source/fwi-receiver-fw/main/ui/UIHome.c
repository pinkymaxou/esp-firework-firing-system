#include "UIHome.h"
    #include "../main.h"

void UIHOME_Enter()
{
    SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();

    char szText[128+1] = {0,};
    char szSoftAPSSID[32] = {0,};

    esp_netif_ip_info_t wifiIpSta = {0};
    MAIN_GetWiFiSTAIP(&wifiIpSta);

    esp_netif_ip_info_t wifiIpAP = {0};
    MAIN_GetWiFiSoftAPIP(&wifiIpAP);

    MAIN_GetWifiAPSSID(szSoftAPSSID);
    sprintf(szText, "%s\n"IPSTR"\n"IPSTR,
        szSoftAPSSID,
        IP2STR(&wifiIpAP.ip),
        IP2STR(&wifiIpSta.ip));

    SSD1306_ClearDisplay(pss1306Handle);
    SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
    SSD1306_UpdateDisplay(pss1306Handle);
}

void UIHOME_Exit()
{

}

void UIHOME_Tick()
{

}

void UIHOME_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{

}
