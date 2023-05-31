#ifndef _MAIN_H_
#define _MAIN_H_

#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "esp_wifi.h"

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip);

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip);

void MAIN_GetWifiAPSSID(char szSoftAPSSID[32]);

#endif