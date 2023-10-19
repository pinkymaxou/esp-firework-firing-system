#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_crc.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi_default.h"
#include "FWIHelper.h"
#include "fwi-remote-comm.h"
#include "FWConfig.h"

#define TAG "fwi-remote-fw"

static void wifi_init();
static void ESPNOW_Init();

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

static esp_netif_t* m_pWifiSoftAP = NULL;
static uint8_t m_u8WiFiChannel = 3;
static wifi_config_t m_WifiConfigAP = {0};

static uint8_t m_u8BroadcastMACs[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    m_pWifiSoftAP = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );

    m_WifiConfigAP.ap.authmode = WIFI_AUTH_OPEN;
    m_WifiConfigAP.ap.pmf_cfg.required = true;
    memset(m_WifiConfigAP.ap.ssid, 0, sizeof(m_WifiConfigAP.ap.ssid));
    const char* szWiFiSSID = "FWI-Remote";
    m_WifiConfigAP.ap.ssid_len = strlen(szWiFiSSID);
    memcpy(m_WifiConfigAP.ap.ssid, szWiFiSSID, m_WifiConfigAP.ap.ssid_len);
    m_WifiConfigAP.ap.channel = m_u8WiFiChannel;
    m_WifiConfigAP.ap.max_connection = 5;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &m_WifiConfigAP));

    ESP_ERROR_CHECK(esp_wifi_start());
}

static void ESPNOW_Init()
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    /* Set primary master key. */
    uint8_t pmks[16] = FWCONFIG_ESPNOW_PMK;
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)pmks) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = m_u8WiFiChannel;
    peer->ifidx = ESP_IF_WIFI_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, m_u8BroadcastMACs, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    wifi_init();
    ESPNOW_Init();

    uint32_t u32TransactionID = 1;

    while(1)
    {
        const uint8_t u8Buffers[128];

        // Send a "get status"
        int32_t s32Count = 0;
        memcpy(u8Buffers, &g_FWIREMOTECOMM_u32Magic, sizeof(uint32_t));
        s32Count += sizeof(uint32_t);

        uint16_t u16 = (uint16_t)FWIREMOTECOMM_EFRAMEID_C2SGetStatus;
        memcpy(u8Buffers + s32Count, &u16, sizeof(uint16_t));
        s32Count += sizeof(uint16_t);

        FWIREMOTECOMM_C2SGetStatus c2sGetStatus;
        c2sGetStatus.u32TransactionID = ++u32TransactionID;
        memcpy(u8Buffers + s32Count, &c2sGetStatus, sizeof(FWIREMOTECOMM_C2SGetStatus));
        s32Count += sizeof(FWIREMOTECOMM_S2CGetStatusResp);

        esp_now_send(m_u8BroadcastMACs, u8Buffers, s32Count);
        vTaskDelay(10);
    }
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{

}

static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    if (esp_now_info == NULL || data == NULL || data_len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    char hexDataString[256+1] = {0};
    if (data_len < 128)
        FWIHELPER_ToHexString(hexDataString, data, data_len);

    ESP_LOGI(TAG, "Receiving data: ' %s ', len: %d", (const char*)hexDataString, data_len);
}
