idf_component_register(
    SRCS "Settings.cpp"
    "webserver/WSWebSocket.cpp" "webserver/WebServer.cpp"
    "HWGPIO.cpp" "main.cpp" "MainApp.cpp"
    "assets/EmbeddedFiles.c"
    "assets/BitmapPotato.c"
    "oledui/UIMenu.cpp" "oledui/UIHome.cpp" "oledui/UIArmed.cpp" "oledui/UIErrorPleaseDisarm.cpp" "oledui/UISetting.cpp" "oledui/UIManager.cpp" "oledui/UITestConn.cpp"
    "oledui/UILiveCheckContinuity.cpp" "oledui/UIAbout.cpp"
    INCLUDE_DIRS "."
    REQUIRES lwip log cjson esp_common esp_rom esp_hw_support esp_timer esp_app_format nvs_flash esp_wifi driver esp_driver_i2c esp_driver_gpio esp_driver_ledc app_update led_strip esp_http_server nvsjson SSD1306)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Werror")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-missing-field-initializers")

# Auto-regenerate embedded web assets when www/ files change
if(NOT CMAKE_SCRIPT_MODE_FILE)
    file(GLOB_RECURSE WWW_FILES "${CMAKE_CURRENT_LIST_DIR}/www/*")

    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_LIST_DIR}/assets/EmbeddedFiles.c"
            "${CMAKE_CURRENT_LIST_DIR}/assets/EmbeddedFiles.h"
            "${CMAKE_CURRENT_LIST_DIR}/assets/EmbeddedFiles.bin"
        COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../../tools/embeddedgen.py"
            -i main/www -o main/assets
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
        DEPENDS ${WWW_FILES}
        COMMENT "Generating embedded web assets"
    )

    set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/assets/EmbeddedFiles.c"
        PROPERTIES GENERATED TRUE)
endif()
