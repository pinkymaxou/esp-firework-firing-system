#include <stdio.h>
#include <string.h>
#include "ESPNow.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvsjson.h"
#include "Settings.h"
#include "HWConfig.h"
#include "MainApp.h"
#include "fwi-output.h"
#include "fwi-remote-comm.h"
#include "FWIHelper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#define TAG "ESPNow"

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

static bool SendFrame(ESPNOW_STxFrame* ptxFrame);

static uint8_t m_u8WiFiChannel = 1;

static uint8_t m_u8BroadcastMACs[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static QueueHandle_t m_qReceiveESPNow = NULL;

void ESPNOW_Init()
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    m_u8WiFiChannel = (uint8_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WiFiChannel);

    m_qReceiveESPNow = xQueueCreate(10, sizeof(ESPNOW_SRxFrame));

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
    memcpy(peer->peer_addr, m_u8BroadcastMACs, ESP_NOW_ETH_ALEN);
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

static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    if (esp_now_info == NULL || data == NULL || data_len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (data_len >= 2)
    {
        uint16_t u16FrameID;
        memcpy(&u16FrameID, data, sizeof(uint16_t));
        ESPNOW_SRxFrame sRXFrame;

        bool bIsRXValid = false;
        sRXFrame.eFrameID = (FWIREMOTECOMM_EFRAMEID)u16FrameID;
        switch(sRXFrame.eFrameID)
        {
            case FWIREMOTECOMM_EFRAMEID_C2SFire:
            {
                if (data_len < 2 + sizeof(FWIREMOTECOMM_C2SFire))
                {
                    ESP_LOGE(TAG, "Cannot decode C2SFire");
                    break;
                }
                memcpy(&sRXFrame.URx.c2sFire, data+2, sizeof(FWIREMOTECOMM_C2SFire));
                bIsRXValid = true;
                break;
            }
            case FWIREMOTECOMM_EFRAMEID_C2SGetStatus:
            {
                if (data_len < 2 + sizeof(FWIREMOTECOMM_C2SGetStatus))
                {
                    ESP_LOGE(TAG, "Cannot decode C2SFire");
                    break;
                }
                memcpy(&sRXFrame.URx.c2sGetStatus, data+2, sizeof(FWIREMOTECOMM_C2SGetStatus));
                bIsRXValid = true;
                break;
            }
            default:
            {
                char hexDataString[256+1] = {0};
                if (data_len <= 128)
                    FWIHELPER_ToHexString(hexDataString, data, data_len);
                ESP_LOGW(TAG, "Receiving data [unknown]: ' %s ', len: %d", (const char*)hexDataString, data_len);
                break;
            }
        }

        if (bIsRXValid)
        {
            if (xQueueSend(m_qReceiveESPNow, &sRXFrame, 0) != pdPASS)
            {
                ESP_LOGW(TAG, "Queue overflow");
            }
        }
    }
}

void ESPNOW_RunTick()
{
    ESPNOW_SRxFrame sRXFrame;
    while (xQueueReceive(m_qReceiveESPNow, &sRXFrame, pdMS_TO_TICKS(0)) == pdPASS)
    {
        switch( sRXFrame.eFrameID)
        {
            case FWIREMOTECOMM_EFRAMEID_C2SFire:
            {
                const FWIREMOTECOMM_C2SFire* pRX = &sRXFrame.URx.c2sFire;
                ESPNOW_STxFrame sTxFrame;

                sTxFrame.eFrameID = FWIREMOTECOMM_EFRAMEID_S2CFireResp;
                sTxFrame.URx.s2cFireResp.u32TransactionID = pRX->u32TransactionID;

                MAINAPP_ExecFire(pRX->u8OutputIndex);
                ESP_LOGI(TAG, "Fire command received, transactionid: %" PRIu32 ", out: %" PRIu32, pRX->u32TransactionID, (uint32_t)pRX->u8OutputIndex);

                SendFrame(&sTxFrame);
                break;
            }
            case FWIREMOTECOMM_EFRAMEID_C2SGetStatus:
            {
                //ESP_LOGI(TAG, "Get status received");

                const FWIREMOTECOMM_C2SGetStatus* pRX = &sRXFrame.URx.c2sGetStatus;

                ESPNOW_STxFrame sTxFrame;

                sTxFrame.eFrameID = FWIREMOTECOMM_EFRAMEID_S2CGetStatusResp;

                sTxFrame.URx.s2cGetStatusResp.u32TransactionID = pRX->u32TransactionID;
                sTxFrame.URx.s2cGetStatusResp.u8GeneralState = (uint8_t)MAINAPP_GetGeneralState();

                for(int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
                {
                    MAINAPP_SRelay sRelay = MAINAPP_GetRelayState(i);
                    const FWIOUTPUT_EOUTPUTSTATE eOutputState = MAINAPP_GetOutputState(&sRelay);
                    sTxFrame.URx.s2cGetStatusResp.u8OutState[i] = (int)eOutputState;
                }
                SendFrame(&sTxFrame);
                break;
            }
            default:
                break;
        }
    }
}

static bool SendFrame(ESPNOW_STxFrame* ptxFrame)
{
    uint8_t buffers[256];
    const uint16_t u16 = (uint16_t)ptxFrame->eFrameID;

    int32_t s32Count = 0;
    memcpy(buffers, &u16, sizeof(uint16_t));
    s32Count += sizeof(uint16_t);

    switch( ptxFrame->eFrameID)
    {
        case FWIREMOTECOMM_EFRAMEID_S2CFireResp:
        {
            memcpy(buffers + s32Count, &ptxFrame->URx.s2cFireResp, sizeof(FWIREMOTECOMM_S2CFireResp));
            s32Count += sizeof(FWIREMOTECOMM_S2CFireResp);
            break;
        }
        case FWIREMOTECOMM_EFRAMEID_S2CGetStatusResp:
        {
            memcpy(buffers + s32Count, &ptxFrame->URx.s2cGetStatusResp, sizeof(FWIREMOTECOMM_S2CGetStatusResp));
            s32Count += sizeof(FWIREMOTECOMM_S2CGetStatusResp);
            break;
        }
        default:
            return false;
    }

    //ESP_LOGI(TAG, "==> frameID: %d", (int)ptxFrame->eFrameID);
    esp_now_send(m_u8BroadcastMACs, buffers, s32Count);
    return true;
}
