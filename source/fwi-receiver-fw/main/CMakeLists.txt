idf_component_register(
    SRCS "Settings.cpp"
    "webserver/WSWebSocket.cpp" "webserver/WebServer.cpp"
    "HardwareGPIO.cpp" "main.cpp" "MainApp.cpp"
    "assets/EmbeddedFiles.c"
    "assets/BitmapPotato.c" "ESPNow.cpp"
    "oledui/UIMenu.cpp" "oledui/UIHome.cpp" "oledui/UIArmed.cpp" "oledui/UIErrorPleaseDisarm.cpp" "oledui/UISetting.cpp" "oledui/UIManager.cpp" "oledui/UITestConn.cpp"
    INCLUDE_DIRS "."
    REQUIRES fwi-remote-comm lwip log json esp_common esp_rom esp_hw_support esp_timer esp_app_format nvs_flash esp_wifi driver app_update led_strip esp_http_server nvsjson SSD1306)
