#include "HardwareGPIO.h"
#include "HWConfig.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "Settings.h"

#define TAG "HardwareGPIO"

void HARDWAREGPIO_Init()
{
    // Relay pin
    gpio_set_direction(HWCONFIG_OUTRELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_OUTRELAY_PIN, false);
    
    // Sanity pin
    gpio_set_direction(HWCONFIG_OUTSANITY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_OUTSANITY_PIN, false);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_12_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };

    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel[1] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = HWCONFIG_OUTSANITY_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .flags.output_invert = 0
        }
    };

    const int ch = 0;
    ledc_channel_config(&ledc_channel[ch]);    
}

void HARDWAREGPIO_SetSanityLED(float fltPercent)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095 * fltPercent));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}