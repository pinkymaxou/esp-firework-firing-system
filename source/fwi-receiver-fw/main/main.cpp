#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/GPIO.h"
#include "HardwareGPIO.hpp"
#include "lwip/apps/netbiosns.h"
#include "lwip/ip4_addr.h"
#include "HWConfig.h"
#include "FWConfig.hpp"
#include "webserver/WebServer.hpp"
#include "Settings.hpp"
#include "MainApp.hpp"
#include "main.hpp"

#define TAG "main"

static esp_netif_t* m_pWifiSoftAP;
static esp_netif_t* m_pWifiSTA;
static wifi_config_t m_WifiConfigAP = {0};

static uint8_t m_u8WiFiChannel = 1;
static int32_t m_s32UserCount = 0;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void wifi_init();
static esp_err_t espnow_init(void);

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void example_espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);

extern "C" {
    void app_main(void);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    if (esp_now_info->des_addr == NULL || data == NULL || data_len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    char hexDataString[128+1] = {0};
    if (data_len < 64)
        ToHexString(hexDataString, data, data_len);

    ESP_LOGI(TAG, "Receiving data: ' %s ', len: %d", (const char*)hexDataString, data_len);
}

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
        m_s32UserCount++;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
        m_s32UserCount--;
    }
}

static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        /*if (!m_bIsWebServerInit)
        {
            m_bIsWebServerInit = true;
            WEBSERVER_Init();
        }*/
    }
}

static void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    const bool isWiFiSTA = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive) == 1;
    if (isWiFiSTA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    }

    // Access point mode
    m_pWifiSoftAP = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo.gw, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(m_pWifiSoftAP);
    esp_netif_set_ip_info(m_pWifiSoftAP, &ipInfo);
    esp_netif_dhcps_start(m_pWifiSoftAP);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifisoftap_event_handler,
                                                        NULL,
                                                        NULL));

    memset(m_WifiConfigAP.ap.ssid, 0, sizeof(m_WifiConfigAP.ap.ssid));
    m_WifiConfigAP.ap.ssid_len = 0;
    m_WifiConfigAP.ap.channel = m_u8WiFiChannel;
    m_WifiConfigAP.ap.max_connection = 5;

    uint8_t macAddr[6];
    esp_read_mac(macAddr, ESP_MAC_WIFI_SOFTAP);

    sprintf((char*)m_WifiConfigAP.ap.ssid, FWCONFIG_STAAP_WIFI_SSID, macAddr[3], macAddr[4], macAddr[5]);
    int n = strlen((const char*)m_WifiConfigAP.ap.ssid);
    m_WifiConfigAP.ap.ssid_len = n;

    size_t staPassLength = 64;
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WAPPass, (char*)m_WifiConfigAP.ap.password, &staPassLength);

    if (strlen((const char*)m_WifiConfigAP.ap.password) == 0)
    {
        m_WifiConfigAP.ap.authmode = WIFI_AUTH_OPEN;
    }
    else {
        m_WifiConfigAP.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    ESP_LOGI(TAG, "SoftAP: %s", m_WifiConfigAP.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &m_WifiConfigAP));

    if (isWiFiSTA)
    {
        m_pWifiSTA = esp_netif_create_default_wifi_sta();

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_configSTA = {
            .sta = {
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };
        wifi_configSTA.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        size_t staSSIDLength = 32;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTASSID, (char*)wifi_configSTA.sta.ssid, &staSSIDLength);

        size_t staPassLength = 64;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTAPass, (char*)wifi_configSTA.sta.password, &staPassLength);

        ESP_LOGI(TAG, "STA mode is active, attempt to connect to ssid: %s", wifi_configSTA.sta.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configSTA) );
    }

    ESP_ERROR_CHECK( esp_wifi_start());
}

static esp_err_t espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    uint8_t pmks[16];
    size_t length = sizeof(pmks);
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNOW_PMK, (char*)pmks, &length);
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)pmks) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t* peer = (esp_now_peer_info_t*)malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = m_u8WiFiChannel;
    peer->ifidx = WIFI_IF_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSTA, ip);
}

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSoftAP, ip);
}

int32_t MAIN_GetSAPUserCount()
{
    return m_s32UserCount;
}

void MAIN_GetWifiAPSSID(char szSoftAPSSID[32])
{
    strncpy(szSoftAPSSID, (char*)m_WifiConfigAP.ap.ssid, 32);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_LOGI(TAG, "HARDWAREGPIO_Init");
    HARDWAREGPIO_Init();

    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "SETTINGS_Init");
    SETTINGS_Init();

    m_u8WiFiChannel = (uint8_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WiFiChannel);

    ESP_LOGI(TAG, "wifi_init");
    wifi_init();
    espnow_init();

    ESP_LOGI(TAG, "WEBSERVER_Init");
    WEBSERVER_Init();

    g_app.Init();

    char* szAllTask = (char*)malloc(4096);
    vTaskList(szAllTask);
    ESP_LOGI(TAG, "vTaskList: \r\n\r\n%s", szAllTask);
    free(szAllTask);

    // Just give sometime to display the logo
    ESP_LOGI(TAG, "Showing logo");
    vTaskDelay(pdMS_TO_TICKS(500));
    // Lock forever
    ESP_LOGI(TAG, "Run");
    g_app.Run();
}

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sprintf(dstHexString + (i * 2), "%02X", data[i]);
}
