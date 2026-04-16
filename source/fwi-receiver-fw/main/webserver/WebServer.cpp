#include "WebServer.hpp"
#include "WSWebSocket.hpp"
#include "esp_log.h"
#include "assets/EmbeddedFiles.h"
#include "esp_ota_ops.h"
#include <stdio.h>
#include <sys/param.h>
#include <string.h>

#define TAG "WebServer"

#define HTTP_BUFFER_SIZE (1024 * 6)

#define DEFAULT_URI "/index.html"

static uint8_t m_buffer[HTTP_BUFFER_SIZE];

static esp_err_t fileGetHandler(httpd_req_t* req);
static esp_err_t otaUploadHandler(httpd_req_t* req);
static esp_err_t setContentType(httpd_req_t* req, const char* filename);
static const EF_SFile* findFile(const char* filename);

static const httpd_uri_t ROUTE_WS =
{
    .uri          = "/ws",
    .method       = HTTP_GET,
    .handler      = WSWebSocket::handler,
    .user_ctx     = NULL,
    .is_websocket = true
};

static const httpd_uri_t ROUTE_OTA =
{
    .uri      = "/ota/upload",
    .method   = HTTP_POST,
    .handler  = otaUploadHandler,
    .user_ctx = NULL
};

static const httpd_uri_t ROUTE_FILES =
{
    .uri      = "/*",
    .method   = HTTP_GET,
    .handler  = fileGetHandler,
    .user_ctx = NULL
};

void WebServer::init()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_open_sockets = 13;

    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    if (ESP_OK == httpd_start(&server, &config))
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ROUTE_WS);
        httpd_register_uri_handler(server, &ROUTE_OTA);
        httpd_register_uri_handler(server, &ROUTE_FILES);
    }
}

static esp_err_t fileGetHandler(httpd_req_t* req)
{
    if (0 == strcmp(req->uri, "/"))
    {
        ESP_LOGW(TAG, "Redirect %s -> %s", req->uri, DEFAULT_URI);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_status(req, "301 Moved Permanently");
        httpd_resp_set_hdr(req, "Location", DEFAULT_URI);
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Serving: %s", req->uri);

    const EF_SFile* file = findFile(req->uri + 1);
    if (NULL == file)
    {
        ESP_LOGE(TAG, "File not found: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    setContentType(req, file->strFilename);

    if (EF_ISFILECOMPRESSED(file->eFlags))
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    uint32_t offset = 0;
    while (offset < file->u32Length)
    {
        const uint32_t chunk = MIN(file->u32Length - offset, HTTP_BUFFER_SIZE);
        if (ESP_OK != httpd_resp_send_chunk(req, (char*)(file->pu8StartAddr + offset), chunk))
        {
            ESP_LOGE(TAG, "Send failed for: %s", req->uri);
            httpd_resp_sendstr_chunk(req, NULL);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
        offset += chunk;
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t otaUploadHandler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "OTA upload started");

    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

    if (NULL == update_partition)
    {
        ESP_LOGE(TAG, "No OTA partition available");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Running at 0x%08x, writing to subtype %d at 0x%08x",
        (int)running->address, (int)update_partition->subtype, (int)update_partition->address);

    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        goto ERROR;
    }

    {
        int n = httpd_req_recv(req, (char*)m_buffer, HTTP_BUFFER_SIZE);
        int total = 0;

        while (n > 0)
        {
            err = esp_ota_write(update_handle, m_buffer, n);
            if (ESP_OK != err)
            {
                esp_ota_abort(update_handle);
                goto ERROR;
            }
            total += n;
            ESP_LOGD(TAG, "Written %d bytes", total);
            n = httpd_req_recv(req, (char*)m_buffer, HTTP_BUFFER_SIZE);
        }
    }

    err = esp_ota_end(update_handle);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        goto ERROR;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        goto ERROR;
    }

    ESP_LOGI(TAG, "OTA complete, restarting");
    esp_restart();
    return ESP_OK;

ERROR:
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA failed");
    return ESP_FAIL;
}

#define IS_FILE_EXT(filename, ext) \
    (0 == strcasecmp(&(filename)[strlen(filename) - sizeof(ext) + 1], ext))

static esp_err_t setContentType(httpd_req_t* req, const char* filename)
{
    if      (IS_FILE_EXT(filename, ".html") || IS_FILE_EXT(filename, ".htm"))
        return httpd_resp_set_type(req, "text/html");
    else if (IS_FILE_EXT(filename, ".css"))
        return httpd_resp_set_type(req, "text/css");
    else if (IS_FILE_EXT(filename, ".js"))
        return httpd_resp_set_type(req, "text/javascript");
    else if (IS_FILE_EXT(filename, ".json"))
        return httpd_resp_set_type(req, "application/json");
    else if (IS_FILE_EXT(filename, ".ico"))
        return httpd_resp_set_type(req, "image/x-icon");
    else if (IS_FILE_EXT(filename, ".jpeg") || IS_FILE_EXT(filename, ".jpg"))
        return httpd_resp_set_type(req, "image/jpeg");
    else if (IS_FILE_EXT(filename, ".svg"))
        return httpd_resp_set_type(req, "image/svg+xml");
    else if (IS_FILE_EXT(filename, ".ttf"))
        return httpd_resp_set_type(req, "application/x-font-truetype");
    else if (IS_FILE_EXT(filename, ".woff"))
        return httpd_resp_set_type(req, "application/font-woff");
    else if (IS_FILE_EXT(filename, ".pdf"))
        return httpd_resp_set_type(req, "application/pdf");
    return httpd_resp_set_type(req, "text/plain");
}

static const EF_SFile* findFile(const char* filename)
{
    for (int i = 0; i < EF_EFILE_COUNT; i++)
    {
        if (0 == strcmp(EF_g_sFiles[i].strFilename, filename))
            return &EF_g_sFiles[i];
    }
    return NULL;
}
