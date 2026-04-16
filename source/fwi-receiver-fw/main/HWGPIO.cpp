#include "HWGPIO.hpp"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "Settings.hpp"
#include "assets/BitmapPotato.h"
#include "led_strip.h"
#include <assert.h>

#define TAG "HardwareGPIO"

static led_strip_handle_t m_led_strip;
#if HWCONFIG_OLED_ISPRESENT != 0
static SSD1306_handle m_ssd1306;
#endif
static int32_t m_encoder_ticks = 0;

static gpio_num_t m_bus_pins[HWCONFIG_OUTPUTBUS_COUNT] =
{
    /* 0*/HWCONFIG_RELAY_BUS_1_PIN,
    /* 1*/HWCONFIG_RELAY_BUS_2_PIN,
    /* 2*/HWCONFIG_RELAY_BUS_3_PIN,
    /* 3*/HWCONFIG_RELAY_BUS_4_PIN,
    /* 4*/HWCONFIG_RELAY_BUS_5_PIN,
    /* 5*/HWCONFIG_RELAY_BUS_6_PIN,
    /* 6*/HWCONFIG_RELAY_BUS_7_PIN,
    /* 7*/HWCONFIG_RELAY_BUS_8_PIN,

    /* 8*/HWCONFIG_RELAY_BUS_9_PIN,
    /* 9*/HWCONFIG_RELAY_BUS_10_PIN,
    /*10*/HWCONFIG_RELAY_BUS_11_PIN,
    /*11*/HWCONFIG_RELAY_BUS_12_PIN,
    /*12*/HWCONFIG_RELAY_BUS_13_PIN,
    /*13*/HWCONFIG_RELAY_BUS_14_PIN,
    /*14*/HWCONFIG_RELAY_BUS_15_PIN,
    /*15*/HWCONFIG_RELAY_BUS_16_PIN
};

static gpio_num_t m_bus_area_pins[HWCONFIG_OUTPUTAREA_COUNT] =
{
    HWCONFIG_RELAY_MOSA1,
    HWCONFIG_RELAY_MOSA2,
    // HWCONFIG_RELAY_MOSA3
};

static void IRAM_ATTR gpioIsrHandler(void* arg) ;

void HWGPIO_Init()
{
    // Sanity LEDs
    gpio_reset_pin(HWCONFIG_LEDWS2812B_PIN);
    gpio_set_direction(HWCONFIG_LEDWS2812B_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(HWCONFIG_SANITY2_PIN);
    gpio_set_direction(HWCONFIG_SANITY2_PIN, GPIO_MODE_OUTPUT);

    // Initialize relay BUS
    for(int i = 0; i < HWCONFIG_OUTPUTBUS_COUNT; i++)
    {
        const gpio_num_t gpio = m_bus_pins[i];
        gpio_reset_pin(gpio);
        gpio_set_direction(gpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpio, GPIO_FLOATING);
    }

    for(int i = 0; i < HWCONFIG_OUTPUTAREA_COUNT; i++)
    {
        const gpio_num_t gpio = m_bus_area_pins[i];
        gpio_reset_pin(gpio);
        gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio, false);
    }

    // Relay pin
    gpio_reset_pin(HWCONFIG_MASTERPWR_EN);
    gpio_set_direction(HWCONFIG_MASTERPWR_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_MASTERPWR_EN, false);

    gpio_reset_pin(HWCONFIG_MASTERPWRSENSE_IN);
    gpio_set_direction(HWCONFIG_MASTERPWRSENSE_IN, GPIO_MODE_INPUT);
    gpio_pullup_en(HWCONFIG_MASTERPWRSENSE_IN);

    gpio_reset_pin(HWCONFIG_CONNSENSE_IN);
    gpio_set_direction(HWCONFIG_CONNSENSE_IN, GPIO_MODE_INPUT);

    // User input
    HWGPIO_ClearRelayBus();

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = HWCONFIG_LEDWS2812B_PIN,
        .max_leds = HWCONFIG_OUTPUT_COUNT+1, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &m_led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(m_led_strip);

    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = HWCONFIG_I2C_MASTER_NUM,
        .sda_io_num = HWCONFIG_I2C_SDA,
        .scl_io_num = HWCONFIG_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));

    #if HWCONFIG_OLED_ISPRESENT != 0
    i2c_device_config_t oled_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = HWCONFIG_OLED_I2CADDR,
        .scl_speed_hz = HWCONFIG_I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t oled_dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &oled_dev_config, &oled_dev_handle));

    static SSD1306_config cfg_ssd1306 = SSD1306_CONFIG_DEFAULT_128x64;
	//cfg_ssd1306.pinReset = (gpio_num_t)CONFIG_I2C_MASTER_RESET;
    SSD1306_Init(&m_ssd1306, oled_dev_handle, &cfg_ssd1306);
    SSD1306_ClearDisplay(&m_ssd1306);
    memcpy(m_ssd1306.buffer, m_u8LogoDatas, m_u32LogoDataLen);
    assert(m_u32LogoDataLen == m_ssd1306.bufferLen); // , "Bitmap and buffer doesn't match"
    SSD1306_UpdateDisplay(&m_ssd1306);
    #endif

    // PWM for master power
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_12_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 200,  // Set output frequency at 200 Hz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ESP_LOGI(TAG, "initialize LEDC");

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t masterpwrm_channel = {
        .gpio_num       = HWCONFIG_MASTERPWR_EN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = HWCONFIG_MASTERPWR_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&masterpwrm_channel));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t sanityled_channel = {
        .gpio_num       = HWCONFIG_SANITY2_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = HWCONFIG_SANITY2_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&sanityled_channel));

    // Install LEDC fade service for smooth sanity LED transitions
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    #if HWCONFIG_ENCODER_ISPRESENT != 0
    // Conf A
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE; // Configure interrupt on positive edge
    // Replace XX with the GPIO pin number
    io_conf.pin_bit_mask = (1ULL << HWCONFIG_ENCODERA_IN) | (1ULL << HWCONFIG_ENCODERB_IN) | (1ULL << HWCONFIG_ENCODERSW);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(HWCONFIG_ENCODERA_IN, gpioIsrHandler, NULL);
    gpio_isr_handler_add(HWCONFIG_ENCODERB_IN, gpioIsrHandler, NULL);
    #endif
}

static bool m_last_enc_a = false;
static void gpioIsrHandler(void* arg)
{
    const bool enc_a = gpio_get_level(HWCONFIG_ENCODERA_IN) == false;
    const bool enc_b = gpio_get_level(HWCONFIG_ENCODERB_IN) == false;

    if (enc_a != m_last_enc_a)
    {
        if (enc_a != enc_b)
        {
            m_encoder_ticks--;
        }
        else
        {
            m_encoder_ticks++;
        }
        m_last_enc_a = enc_a;
    }
}

void HWGPIO_SetSanityLED(bool is_enabled, bool is_armed)
{
    // Ground driven: lower duty = brighter
    const uint32_t value = is_enabled ? (is_armed ? 0 : (4095 - 500)) : 4095;
    const uint32_t fade_ms = is_armed ? 200 : 450;

    ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, HWCONFIG_SANITY2_CHANNEL, value, (int)fade_ms));
    ESP_ERROR_CHECK(ledc_fade_start(LEDC_LOW_SPEED_MODE, HWCONFIG_SANITY2_CHANNEL, LEDC_FADE_NO_WAIT));

    if (is_armed)
        led_strip_set_pixel(m_led_strip, 0, is_enabled ? 200 : 0, 0, 0);
    else
        led_strip_set_pixel(m_led_strip, 0, 0, is_enabled ? 200 : 0, 0);
    HWGPIO_RefreshLEDStrip();
}

void HWGPIO_SetOutputRelayStatusColor(uint32_t output_index, uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(m_led_strip, output_index+1, r, g, b);
}

void HWGPIO_RefreshLEDStrip()
{
    /* Refresh the strip to send data */
    led_strip_refresh(m_led_strip);
}

void HWGPIO_ClearRelayBus()
{
    // Stop all relay boards
    for(int i = 0; i < HWCONFIG_OUTPUTAREA_COUNT; i++)
        gpio_set_level(m_bus_area_pins[i], false);

    // Clear the bus
    for(int i = 0; i < HWCONFIG_OUTPUTBUS_COUNT; i++)
    {
        const gpio_num_t gpio_relay = m_bus_pins[i];
        gpio_set_direction(gpio_relay, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpio_relay, GPIO_FLOATING);
    }

    // Mechanical relay, give them some time to be sure they are turned off.
    vTaskDelay(pdMS_TO_TICKS(25));
}

void HWGPIO_WriteSingleRelay(uint32_t output_index, bool value)
{
    if (output_index >= HWCONFIG_OUTPUT_COUNT)
        return;

    HWGPIO_ClearRelayBus();

    // Activate the right area
    const uint32_t area_index = output_index / HWCONFIG_OUTPUTBUS_COUNT;
    const gpio_num_t gpio_area = m_bus_area_pins[area_index];
    gpio_set_level(gpio_area, value);

    const uint32_t bus_pin_index = output_index % HWCONFIG_OUTPUTBUS_COUNT;
    const gpio_num_t gpio_relay = m_bus_pins[bus_pin_index];

    if (value)
    {
        gpio_set_direction(gpio_relay, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_relay, false);
    }
    else
    {
        gpio_set_direction(gpio_relay, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpio_relay, GPIO_FLOATING);
    }
}

void HWGPIO_WriteMasterPowerRelay(bool value)
{
    const double percent = NVSJSON_GetValueInt32(&g_settingHandle, SETTINGS_EENTRY_FiringPWMPercent)/100.0;

    const uint32_t duty = value ? (4095 * percent) : 0;

    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_MASTERPWR_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_MASTERPWR_CHANNEL));
}

bool HWGPIO_ReadMasterPowerSense()
{
    return gpio_get_level(HWCONFIG_MASTERPWRSENSE_IN) == false;
}

bool HWGPIO_ReadConnectionSense()
{
    return gpio_get_level(HWCONFIG_CONNSENSE_IN) == false;
}

bool HWGPIO_IsEncoderSwitchON()
{
    return gpio_get_level(HWCONFIG_ENCODERSW) == false;
}

uint32_t HWGPIO_GetRelayArea(uint32_t output_index)
{
    return output_index / HWCONFIG_OUTPUTBUS_COUNT;
}

int32_t HWGPIO_GetEncoderCount()
{
    const int32_t ticks = m_encoder_ticks;
    m_encoder_ticks = 0;
    return ticks;
}

#if HWCONFIG_OLED_ISPRESENT != 0
SSD1306_handle* HWGPIO_GetSSD1306Handle()
{
    return &m_ssd1306;
}
#endif
