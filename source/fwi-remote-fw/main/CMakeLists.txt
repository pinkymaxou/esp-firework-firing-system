idf_component_register(SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES fwi-remote-comm log json esp_common esp_rom esp_hw_support esp_timer esp_app_format esp_wifi esp_netif nvs_flash driver)
