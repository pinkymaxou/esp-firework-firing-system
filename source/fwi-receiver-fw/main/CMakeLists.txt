idf_component_register(
    SRCS "Settings.c"
    "webserver/WSWebSocket.c" "webserver/WebServer.c"
    "HardwareGPIO.cpp" "main.c" "MainApp.cpp"
    "assets/EmbeddedFiles.c"
    "assets/BitmapPotato.c"
    "oledui/UIMenu.c" "oledui/UIHome.c" "oledui/UIArmed.c" "oledui/UIErrorPleaseDisarm.c" "oledui/UISetting.c" "oledui/UIManager.cpp" "oledui/UITestConn.c"
    INCLUDE_DIRS "."
    REQUIRES lwip log json esp_common esp_rom esp_hw_support esp_timer esp_app_format nvs_flash esp_wifi driver app_update led_strip esp_http_server nvsjson SSD1306)
