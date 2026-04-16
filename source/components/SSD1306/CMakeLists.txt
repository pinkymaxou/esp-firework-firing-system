idf_component_register(SRCS "SSD1306.c"
    INCLUDE_DIRS .
    REQUIRES esp_driver_i2c esp_driver_gpio)