#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "HWGPIO.hpp"
#include "lwip/apps/netbiosns.h"
#include "lwip/ip4_addr.h"
#include "HWConfig.h"
#include "FWConfig.hpp"
#include "webserver/WebServer.hpp"
#include "Settings.hpp"
#include "MainApp.hpp"
#include "main.hpp"

#define TAG "main"

static esp_netif_t* m_wifi_soft_ap;
static esp_netif_t* m_wifi_sta;
static wifi_config_t m_wifi_config_ap = {0};

static uint8_t m_wifi_channel = 1;
static int32_t m_user_count = 0;

static void wifiInit();

static void wifiSoftAPEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifiStationEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

extern "C" {
    void app_main(void);
}

static void wifiSoftAPEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT_AP_STACONNECTED == event_id)
    {
        const wifi_event_ap_staconnected_t* event = (const wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
        m_user_count++;
    }
    else if (WIFI_EVENT_AP_STADISCONNECTED == event_id)
    {
        const wifi_event_ap_stadisconnected_t* event = (const wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
        m_user_count--;
    }
}

static void wifiStationEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base && WIFI_EVENT_STA_START == event_id)
    {
        esp_wifi_connect();
    }
    else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_DISCONNECTED == event_id)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (IP_EVENT == event_base && IP_EVENT_STA_GOT_IP == event_id)
    {
        const ip_event_got_ip_t* event = (const ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void wifiInit()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    const bool is_wifi_sta = (1 == NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WSTAIsActive));
    if (is_wifi_sta)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    }

    // Access point mode
    m_wifi_soft_ap = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(m_wifi_soft_ap);
    esp_netif_set_ip_info(m_wifi_soft_ap, &ip_info);
    esp_netif_dhcps_start(m_wifi_soft_ap);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiSoftAPEventHandler,
                                                        NULL,
                                                        NULL));

    memset(m_wifi_config_ap.ap.ssid, 0, sizeof(m_wifi_config_ap.ap.ssid));
    m_wifi_config_ap.ap.ssid_len = 0;
    m_wifi_config_ap.ap.channel = m_wifi_channel;
    m_wifi_config_ap.ap.max_connection = 5;

    uint8_t mac_addr[6];
    esp_read_mac(mac_addr, ESP_MAC_WIFI_SOFTAP);

    sprintf((char*)m_wifi_config_ap.ap.ssid, FWCONFIG_STAAP_WIFI_SSID, mac_addr[3], mac_addr[4], mac_addr[5]);
    const int n = strlen((const char*)m_wifi_config_ap.ap.ssid);
    m_wifi_config_ap.ap.ssid_len = n;

    size_t sta_pass_length = 64;
    NVSJSON_GetValueString(&g_settingHandle, SETTINGS_EENTRY_WAPPass, (char*)m_wifi_config_ap.ap.password, &sta_pass_length);

    if (0 == strlen((const char*)m_wifi_config_ap.ap.password))
    {
        m_wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        m_wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    ESP_LOGI(TAG, "SoftAP: %s", m_wifi_config_ap.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &m_wifi_config_ap));

    if (is_wifi_sta)
    {
        m_wifi_sta = esp_netif_create_default_wifi_sta();

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifiStationEventHandler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifiStationEventHandler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_config_sta = {
            .sta = {
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };
        wifi_config_sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        size_t sta_ssid_length = 32;
        NVSJSON_GetValueString(&g_settingHandle, SETTINGS_EENTRY_WSTASSID, (char*)wifi_config_sta.sta.ssid, &sta_ssid_length);

        size_t sta_pass_length2 = 64;
        NVSJSON_GetValueString(&g_settingHandle, SETTINGS_EENTRY_WSTAPass, (char*)wifi_config_sta.sta.password, &sta_pass_length2);

        ESP_LOGI(TAG, "STA mode is active, attempt to connect to ssid: %s", wifi_config_sta.sta.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    }

    ESP_ERROR_CHECK( esp_wifi_start());
}

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_wifi_sta, ip);
}

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_wifi_soft_ap, ip);
}

int32_t MAIN_GetSAPUserCount()
{
    return m_user_count;
}

void MAIN_GetWifiAPSSID(char ssid[32])
{
    strncpy(ssid, (char*)m_wifi_config_ap.ap.ssid, 32);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ESP_ERR_NVS_NO_FREE_PAGES == ret || ESP_ERR_NVS_NEW_VERSION_FOUND == ret)
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_LOGI(TAG, "HWGPIO_Init");
    HWGPIO_Init();

    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "SETTINGS_Init");
    SETTINGS_Init();

    m_wifi_channel = (uint8_t)NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_WiFiChannel);

    ESP_LOGI(TAG, "wifi_init");
    wifiInit();

    ESP_LOGI(TAG, "WEBSERVER_Init");
    webServerInit();

    g_app.Init();

    char* all_task = (char*)malloc(4096);
    vTaskList(all_task);
    ESP_LOGI(TAG, "vTaskList: \r\n\r\n%s", all_task);
    free(all_task);

    // Just give sometime to display the logo
    ESP_LOGI(TAG, "Showing logo");
    vTaskDelay(pdMS_TO_TICKS(500));
    // Lock forever
    ESP_LOGI(TAG, "Run");
    g_app.Run();
}
