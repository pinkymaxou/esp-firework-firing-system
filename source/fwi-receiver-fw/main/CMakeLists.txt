idf_component_register(SRCS "Settings.c" "WebServer.c" "HardwareGPIO.c" "main.c" "assets/EmbeddedFiles.c"
                    INCLUDE_DIRS "."
                    REQUIRES lwip log json esp_common esp_hw_support esp_timer esp_app_format nvs_flash esp_wifi driver app_update led_strip esp_http_server nvsjson)
