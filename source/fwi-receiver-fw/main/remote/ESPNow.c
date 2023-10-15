#include <stdio.h>
#include <string.h>
#include "ESPNow.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvsjson.h"
#include "Settings.h"

#define TAG "ESPNow"

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);

static uint8_t m_u8WiFiChannel = 1;

static uint8_t m_BroadcastMACs[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void ESPNOW_Init()
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    m_u8WiFiChannel = (uint8_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WiFiChannel);

    /* Set primary master key. */
    uint8_t pmks[16];
    size_t length = sizeof(pmks);
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNOW_PMK, (char*)pmks, &length);
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
    memcpy(peer->peer_addr, m_BroadcastMACs, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    char hexDataString[128+1] = {0};
    if (len < 64)
        ToHexString(hexDataString, data, len);

    ESP_LOGI(TAG, "Receiving data: ' %s ', len: %d", (const char*)hexDataString, len);
}

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sprintf(dstHexString + (i * 2), "%02X", data[i]);
}
