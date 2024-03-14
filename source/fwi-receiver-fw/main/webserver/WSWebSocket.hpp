#ifndef _WSWEBSOCKET_H_
#define _WSWEBSOCKET_H_

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t WSWEBSOCKET_Handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif