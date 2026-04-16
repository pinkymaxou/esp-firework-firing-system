#pragma once

#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "esp_wifi.h"

namespace Main
{
    void getWiFiSTAIP(esp_netif_ip_info_t* ip);

    void getWiFiSoftAPIP(esp_netif_ip_info_t* ip);

    void getWifiAPSSID(char ssid[32]);

    int32_t getSAPUserCount();
}
