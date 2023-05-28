#include "HardwareGPIO.h"
#include "HWConfig.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Settings.h"
#include "led_strip.h"

#define TAG "HardwareGPIO"

static led_strip_handle_t led_strip;

void HARDWAREGPIO_Init()
{
    // Sanity LEDs
    gpio_set_direction(HWCONFIG_SANITY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(HWCONFIG_SANITY2_PIN, GPIO_MODE_OUTPUT);

    #if HWCONFIG_TESTMODE == 0
    // Initialize relay BUS
    gpio_set_direction(HWCONFIG_RELAY_BUS_1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_1_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_2_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_3_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_3_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_4_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_4_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_5_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_5_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_6_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_6_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_7_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_7_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_8_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_8_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_9_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_9_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_10_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_10_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_11_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_12_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_12_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_13_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_13_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_14_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_14_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_15_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_15_PIN, true);
    gpio_set_direction(HWCONFIG_RELAY_BUS_16_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_BUS_16_PIN, true);

    gpio_set_direction(HWCONFIG_RELAY_MOSA1, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_MOSA1, true);
    gpio_set_direction(HWCONFIG_RELAY_MOSA2, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_MOSA2, true);
    gpio_set_direction(HWCONFIG_RELAY_MOSA3, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_RELAY_MOSA3, true);

    // Relay pin
    gpio_set_direction(HWCONFIG_MASTERPWRRELAY_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_MASTERPWRRELAY_EN, false);

    gpio_set_direction(HWCONFIG_MASTERPWRSENSE_IN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_MASTERPWRSENSE_IN);
    gpio_set_direction(HWCONFIG_CONNSENSE_IN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_CONNSENSE_IN);

    // User input
    gpio_set_direction(HWCONFIG_ENCODERA_IN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_ENCODERA_IN);
    gpio_set_direction(HWCONFIG_ENCODERB_IN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_ENCODERB_IN);
    gpio_set_direction(HWCONFIG_ENCODERSW, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_ENCODERSW);
    #endif
    
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = HWCONFIG_SANITY_PIN,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void HARDWAREGPIO_SetSanityLED(bool isEnabled)
{
    /* If the addressable LED is enabled */
    if (isEnabled)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 255, 255, 255);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    }
    else
    {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}