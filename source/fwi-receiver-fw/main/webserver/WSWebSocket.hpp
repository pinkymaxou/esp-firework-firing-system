#pragma once

#include <esp_http_server.h>

namespace WSWebSocket
{
    esp_err_t handler(httpd_req_t* req);
}
