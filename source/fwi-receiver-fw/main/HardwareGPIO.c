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
    gpio_set_direction(HWCONFIG_SANITY_PIN, GPIO_MODE_OUTPUT);

    // Relay pin
    // gpio_set_direction(HWCONFIG_OUTRELAY_PIN, GPIO_MODE_OUTPUT);
    // gpio_set_level(HWCONFIG_OUTRELAY_PIN, false);

    // Sanity pin
    // gpio_set_direction(HWCONFIG_OUTSANITY_PIN, GPIO_MODE_OUTPUT);
    /// gpio_set_level(HWCONFIG_OUTSANITY_PIN, false);

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