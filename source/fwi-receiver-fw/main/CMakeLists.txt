idf_component_register(
    SRCS "Settings.c" "webserver/WSWebSocket.c" "webserver/WebServer.c" "HardwareGPIO.c" "main.c" "MainApp.c" "assets/EmbeddedFiles.c"
         "ui/UIHome.c" "ui/UIManager.c"
    INCLUDE_DIRS "."
    REQUIRES lwip log json esp_common esp_rom esp_hw_support esp_timer esp_app_format nvs_flash esp_wifi driver app_update led_strip esp_http_server nvsjson SSD1306)
