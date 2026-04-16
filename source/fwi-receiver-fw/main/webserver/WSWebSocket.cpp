#include "WSWebSocket.hpp"
#include "Settings.hpp"
#include "main.hpp"
#include "MainApp.hpp"
#include "HWConfig.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_app_desc.h"
#include "esp_netif.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lwip/ip4_addr.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

#define TAG "WSWebSocket"

#define WS_RECV_BUFFER_SIZE (4096)


static char* buildStatusJSON();
static char* buildSettingsJSON();
static char* buildSysInfoJSON();
static esp_err_t sendTextFrame(httpd_req_t* req, const char* text);
static esp_err_t handleCommand(httpd_req_t* req, const cJSON* root);
static void toHexString(char* dst, const uint8_t* data, uint8_t len);
static const char* getChipName(esp_chip_model_t model);

esp_err_t WSWebSocket::handler(httpd_req_t* req)
{
    if (HTTP_GET == req->method)
    {
        ESP_LOGI(TAG, "Handshake done, new connection opened");
        return ESP_OK;
    }

    uint8_t* buf = (uint8_t*)malloc(WS_RECV_BUFFER_SIZE);
    if (NULL == buf)
        return ESP_ERR_NO_MEM;

    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.payload = buf;
    frame.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, WS_RECV_BUFFER_SIZE);
    if (ESP_OK != ret)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed: %d", ret);
        free(buf);
        return ret;
    }

    if (HTTPD_WS_TYPE_TEXT == frame.type)
    {
        buf[frame.len] = '\0';
        cJSON* root = cJSON_Parse((const char*)buf);
        if (NULL != root)
        {
            ret = handleCommand(req, root);
            cJSON_Delete(root);
        }
        else
        {
            ESP_LOGE(TAG, "Invalid JSON received");
            ret = ESP_FAIL;
        }
    }

    free(buf);
    return ret;
}

static esp_err_t sendTextFrame(httpd_req_t* req, const char* text)
{
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.payload = (uint8_t*)text;
    frame.len = strlen(text);
    frame.type = HTTPD_WS_TYPE_TEXT;
    return httpd_ws_send_frame(req, &frame);
}

static esp_err_t handleCommand(httpd_req_t* req, const cJSON* root)
{
    const cJSON* cmd = cJSON_GetObjectItem(root, "cmd");
    if (NULL == cmd || !cJSON_IsString(cmd))
    {
        ESP_LOGE(TAG, "Missing or invalid cmd field");
        return ESP_FAIL;
    }

    const char* cmd_str = cmd->valuestring;

    if (0 == strcmp(cmd_str, "getstatus"))
    {
        char* json = buildStatusJSON();
        if (NULL == json)
            return ESP_FAIL;
        esp_err_t ret = sendTextFrame(req, json);
        free(json);
        return ret;
    }
    else if (0 == strcmp(cmd_str, "getsettings"))
    {
        char* json = buildSettingsJSON();
        if (NULL == json)
            return ESP_FAIL;
        esp_err_t ret = sendTextFrame(req, json);
        free(json);
        return ret;
    }
    else if (0 == strcmp(cmd_str, "getsysinfo"))
    {
        char* json = buildSysInfoJSON();
        if (NULL == json)
            return ESP_FAIL;
        esp_err_t ret = sendTextFrame(req, json);
        free(json);
        return ret;
    }
    else if (0 == strcmp(cmd_str, "fire"))
    {
        const cJSON* index = cJSON_GetObjectItem(root, "index");
        if (NULL == index || !cJSON_IsNumber(index))
        {
            ESP_LOGE(TAG, "fire: missing index parameter");
            return ESP_FAIL;
        }
        g_app.ExecFire(index->valueint);
        return ESP_OK;
    }
    else if (0 == strcmp(cmd_str, "checkconnections"))
    {
        g_app.ExecTestConnections();
        return ESP_OK;
    }
    else if (0 == strcmp(cmd_str, "setsettings"))
    {
        const cJSON* entries = cJSON_GetObjectItem(root, "entries");

        cJSON* settings = cJSON_CreateObject();
        if (NULL == settings)
            return ESP_FAIL;

        if (NULL != entries)
            cJSON_AddItemToObject(settings, "entries", cJSON_Duplicate(entries, true));

        char* json_str = cJSON_PrintUnformatted(settings);
        cJSON_Delete(settings);

        if (NULL == json_str)
            return ESP_FAIL;

        bool ok = NVSJSON_ImportJSON(&Settings::g_handle, json_str);
        free(json_str);
        return ok ? ESP_OK : ESP_FAIL;
    }
    else if (0 == strcmp(cmd_str, "reboot"))
    {
        esp_restart();
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Unknown command: %s", cmd_str);
    return ESP_FAIL;
}

static char* buildStatusJSON()
{
    cJSON* root = cJSON_CreateObject();
    if (NULL == root)
        return NULL;

    cJSON_AddItemToObject(root, "type", cJSON_CreateString("status"));

    cJSON* status = cJSON_CreateObject();
    static int req_index = 0;
    cJSON_AddItemToObject(status, "req", cJSON_CreateNumber(++req_index));
    cJSON_AddItemToObject(status, "is_armed", cJSON_CreateBool(g_app.IsArmed()));
    cJSON_AddItemToObject(status, "general_state", cJSON_CreateNumber((int)g_app.GetGeneralState()));
    cJSON_AddItemToObject(status, "uptime_s", cJSON_CreateNumber((int)(esp_timer_get_time() / 1000000LL)));

    cJSON* outputs = cJSON_AddArrayToObject(status, "outputs");
    for (int i = 0; i < HWCONFIG_OUTPUT_COUNT; i++)
    {
        const MainApp::SRelay relay = g_app.GetRelayState(i);
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "ix", cJSON_CreateNumber(i));
        cJSON_AddItemToObject(entry, "s", cJSON_CreateNumber((int)g_app.GetOutputState(&relay)));
        cJSON_AddItemToArray(outputs, entry);
    }

    cJSON_AddItemToObject(root, "status", status);

    char* str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str;
}

static char* buildSettingsJSON()
{
    cJSON* root = cJSON_CreateObject();
    if (NULL == root)
        return NULL;

    cJSON_AddItemToObject(root, "type", cJSON_CreateString("settings"));

    char* exported = NVSJSON_ExportJSON(&Settings::g_handle);
    if (NULL != exported)
    {
        cJSON* parsed = cJSON_Parse(exported);
        free(exported);
        if (NULL != parsed)
        {
            const cJSON* entries = cJSON_GetObjectItem(parsed, "entries");
            if (NULL != entries)
                cJSON_AddItemToObject(root, "entries", cJSON_Duplicate(entries, true));
            cJSON_Delete(parsed);
        }
    }

    char* str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str;
}

static char* buildSysInfoJSON()
{
    char buff[100];

    cJSON* root = cJSON_CreateObject();
    if (NULL == root)
        return NULL;

    cJSON_AddItemToObject(root, "type", cJSON_CreateString("sysinfo"));
    cJSON* infos = cJSON_AddArrayToObject(root, "infos");

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // Chip model
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("Chip"));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(getChipName(chip_info.model)));
        cJSON_AddItemToArray(infos, entry);
    }
    const esp_app_desc_t* app_desc = esp_app_get_description();
    // Firmware version
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("Firmware"));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(app_desc->version));
        cJSON_AddItemToArray(infos, entry);
    }
    // Compile date/time
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("Compile Time"));
        sprintf(buff, "%s %s", app_desc->date, app_desc->time);
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(buff));
        cJSON_AddItemToArray(infos, entry);
    }
    // SHA256
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("SHA256"));
        char sha256[sizeof(app_desc->app_elf_sha256) * 2 + 1] = {0};
        toHexString(sha256, app_desc->app_elf_sha256, sizeof(app_desc->app_elf_sha256));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(sha256));
        cJSON_AddItemToArray(infos, entry);
    }
    // IDF version
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("IDF"));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(app_desc->idf_ver));
        cJSON_AddItemToArray(infos, entry);
    }
    // MAC addresses
    {
        uint8_t macs[6];
        const struct { const char* name; esp_mac_type_t type; } mac_entries[] =
        {
            { "WiFi.STA", ESP_MAC_WIFI_STA     },
            { "WiFi.AP",  ESP_MAC_WIFI_SOFTAP  },
            { "WiFi.BT",  ESP_MAC_BT           },
        };
        for (int i = 0; i < 3; i++)
        {
            cJSON* entry = cJSON_CreateObject();
            cJSON_AddItemToObject(entry, "name", cJSON_CreateString(mac_entries[i].name));
            esp_read_mac(macs, mac_entries[i].type);
            sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
                macs[0], macs[1], macs[2], macs[3], macs[4], macs[5]);
            cJSON_AddItemToObject(entry, "value", cJSON_CreateString(buff));
            cJSON_AddItemToArray(infos, entry);
        }
    }
    // Heap memory
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("Memory"));
        const int free_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        const int total_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);
        sprintf(buff, "%d / %d", free_size, total_size);
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(buff));
        cJSON_AddItemToArray(infos, entry);
    }
    // WiFi STA IP
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("WiFi (STA)"));
        esp_netif_ip_info_t ip_info;
        Main::getWiFiSTAIP(&ip_info);
        sprintf(buff, IPSTR, IP2STR(&ip_info.ip));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(buff));
        cJSON_AddItemToArray(infos, entry);
    }
    // WiFi Soft-AP IP
    {
        cJSON* entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "name", cJSON_CreateString("WiFi (Soft-AP)"));
        esp_netif_ip_info_t ip_info;
        Main::getWiFiSoftAPIP(&ip_info);
        sprintf(buff, IPSTR, IP2STR(&ip_info.ip));
        cJSON_AddItemToObject(entry, "value", cJSON_CreateString(buff));
        cJSON_AddItemToArray(infos, entry);
    }

    char* str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str;
}

static void toHexString(char* dst, const uint8_t* data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
        sprintf(dst + (i * 2), "%02X", data[i]);
}

static const char* getChipName(esp_chip_model_t model)
{
    switch (model)
    {
        case CHIP_ESP32:       return "ESP32";
        case CHIP_ESP32S2:     return "ESP32-S2";
        case CHIP_ESP32C2:     return "ESP32-C2";
        case CHIP_ESP32C3:     return "ESP32-C3";
        case CHIP_ESP32S3:     return "ESP32-S3";
        case CHIP_ESP32H2:     return "ESP32-H2";
        case CHIP_ESP32P4:     return "ESP32P4";
        case CHIP_ESP32C6:     return "ESP32C6";
        case CHIP_POSIX_LINUX: return "POSIX_LINUX";
        default:               return "Unknown";
    }
}
