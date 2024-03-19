#include "HardwareGPIO.hpp"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "Settings.hpp"
#include "assets/BitmapPotato.h"
#include "led_strip.h"
#include <assert.h>

#define TAG "HardwareGPIO"

static led_strip_handle_t led_strip;
#if HWCONFIG_OLED_ISPRESENT != 0
static SSD1306_handle m_ssd1306;
#endif
static int32_t m_s32EncoderTicks = 0;

static gpio_num_t m_busPins[HWCONFIG_OUTPUTBUS_COUNT] =
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

static gpio_num_t m_busAreaPins[HWCONFIG_OUTPUTAREA_COUNT] =
{
    HWCONFIG_RELAY_MOSA1,
    HWCONFIG_RELAY_MOSA2,
    // HWCONFIG_RELAY_MOSA3
};

static void IRAM_ATTR gpio_isr_handler(void* arg) ;

void HARDWAREGPIO_Init()
{
    // Sanity LEDs
    gpio_reset_pin(HWCONFIG_LEDWS2812B_PIN);
    gpio_set_direction(HWCONFIG_LEDWS2812B_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(HWCONFIG_SANITY2_PIN);
    gpio_set_direction(HWCONFIG_SANITY2_PIN, GPIO_MODE_OUTPUT);

    // Initialize relay BUS
    for(int i = 0; i < HWCONFIG_OUTPUTBUS_COUNT; i++)
    {
        const gpio_num_t gpio = m_busPins[i];
        gpio_reset_pin(gpio);
        gpio_set_direction(gpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpio, GPIO_FLOATING);
    }

    for(int i = 0; i < HWCONFIG_OUTPUTAREA_COUNT; i++)
    {
        const gpio_num_t gpio = m_busAreaPins[i];
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
    HARDWAREGPIO_ClearRelayBus();

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = HWCONFIG_LEDWS2812B_PIN,
        .max_leds = HWCONFIG_OUTPUT_COUNT+1, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);

	const i2c_port_t i2c_master_port = HWCONFIG_I2C_MASTER_NUM;
    i2c_config_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = HWCONFIG_I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = HWCONFIG_I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = HWCONFIG_I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
	ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, HWCONFIG_I2C_MASTER_RX_BUF_DISABLE, HWCONFIG_I2C_MASTER_TX_BUF_DISABLE, 0));

    #if HWCONFIG_OLED_ISPRESENT != 0
    static SSD1306_config cfgSSD1306 = SSD1306_CONFIG_DEFAULT_128x64;
	//cfgSSD1306.pinReset = (gpio_num_t)CONFIG_I2C_MASTER_RESET;
    SSD1306_Init(&m_ssd1306, i2c_master_port, &cfgSSD1306);
    SSD1306_ClearDisplay(&m_ssd1306);
    memcpy(m_ssd1306.u8Buffer, m_u8LogoDatas, m_u32LogoDataLen);
    assert(m_u32LogoDataLen == m_ssd1306.u32BufferLen); // , "Bitmap and buffer doesn't match"
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

    #if HWCONFIG_ENCODER_ISPRESENT != 0
    // Conf A
    gpio_config_t io_confA;
    io_confA.intr_type = GPIO_INTR_ANYEDGE; // Configure interrupt on positive edge
    // Replace XX with the GPIO pin number
    io_confA.pin_bit_mask = (1ULL << HWCONFIG_ENCODERA_IN) | (1ULL << HWCONFIG_ENCODERB_IN) | (1ULL << HWCONFIG_ENCODERSW);
    io_confA.mode = GPIO_MODE_INPUT;
    io_confA.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_confA);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(HWCONFIG_ENCODERA_IN, gpio_isr_handler, NULL);
    gpio_isr_handler_add(HWCONFIG_ENCODERB_IN, gpio_isr_handler, NULL);
    #endif
}

static bool m_lastEncA = false;
static void gpio_isr_handler(void* arg)
{
    bool encA = gpio_get_level(HWCONFIG_ENCODERA_IN) == false;
    bool encB = gpio_get_level(HWCONFIG_ENCODERB_IN) == false;

    if (encA != m_lastEncA) {
        if (encA != encB) {
            m_s32EncoderTicks--;
        }
        else {
            m_s32EncoderTicks++;
        }
        m_lastEncA = encA;
    }
}

void HARDWAREGPIO_SetSanityLED(bool isEnabled, bool isArmed)
{
    // Ground driven
    const uint32_t u32Value = isEnabled ? (isArmed ? 0 : (4095-500)) : 4095;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_SANITY2_CHANNEL, u32Value));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_SANITY2_CHANNEL));

    if (isArmed)
        led_strip_set_pixel(led_strip, 0, isEnabled ? 200 : 0, 0, 0);
    else
        led_strip_set_pixel(led_strip, 0, 0, isEnabled ? 200 : 0, 0);
    HARDWAREGPIO_RefreshLEDStrip();
}

void HARDWAREGPIO_SetOutputRelayStatusColor(uint32_t u32OutputIndex, uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(led_strip, u32OutputIndex+1, r, g, b);
}

void HARDWAREGPIO_RefreshLEDStrip()
{
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
}

void HARDWAREGPIO_ClearRelayBus()
{
    // Stop all relay boards
    for(int i = 0; i < HWCONFIG_OUTPUTAREA_COUNT; i++)
        gpio_set_level(m_busAreaPins[i], false);

    // Clear the bus
    for(int i = 0; i < HWCONFIG_OUTPUTBUS_COUNT; i++)
    {
        const gpio_num_t gpioRelay = m_busPins[i];
        gpio_set_direction(gpioRelay, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpioRelay, GPIO_FLOATING);
    }

    // Mechanical relay, give them some time to be sure they are turned off.
    vTaskDelay(pdMS_TO_TICKS(25));
}

void HARDWAREGPIO_WriteSingleRelay(uint32_t u32OutputIndex, bool bValue)
{
    if (u32OutputIndex >= HWCONFIG_OUTPUT_COUNT)
        return;

    HARDWAREGPIO_ClearRelayBus();

    // Activate the right area
    const uint32_t u32AreaIndex = u32OutputIndex / HWCONFIG_OUTPUTBUS_COUNT;
    const gpio_num_t gpioArea = m_busAreaPins[u32AreaIndex];
    gpio_set_level(gpioArea, bValue);

    const uint32_t u32BusPinIndex = u32OutputIndex % HWCONFIG_OUTPUTBUS_COUNT;
    const gpio_num_t gpioRelay = m_busPins[u32BusPinIndex];

    if (bValue)
    {
        gpio_set_direction(gpioRelay, GPIO_MODE_OUTPUT);
        gpio_set_level(gpioRelay, false);
    }
    else
    {
        gpio_set_direction(gpioRelay, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpioRelay, GPIO_FLOATING);
    }
}

void HARDWAREGPIO_WriteMasterPowerRelay(bool bValue)
{
    const double dPercent = NVSJSON_GetValueDouble(&g_sSettingHandle, SETTINGS_EENTRY_FiringPWMPercent);

    const uint32_t u32Value = bValue ? (4095 * dPercent) : 0;

    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_MASTERPWR_CHANNEL, u32Value));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, HWCONFIG_MASTERPWR_CHANNEL));
}

bool HARDWAREGPIO_ReadMasterPowerSense()
{
    return gpio_get_level(HWCONFIG_MASTERPWRSENSE_IN) == false;
}

bool HARDWAREGPIO_ReadConnectionSense()
{
    return gpio_get_level(HWCONFIG_CONNSENSE_IN) == false;
}

bool HARDWAREGPIO_IsEncoderSwitchON()
{
    return gpio_get_level(HWCONFIG_ENCODERSW) == false;
}

uint32_t HARDWAREGPIO_GetRelayArea(uint32_t u32OutputIndex)
{
    return u32OutputIndex / HWCONFIG_OUTPUTBUS_COUNT;
}

int32_t HARDWAREGPIO_GetEncoderCount()
{
    const int32_t s32Ticks = m_s32EncoderTicks;
    m_s32EncoderTicks = 0;
    return s32Ticks;
}

#if HWCONFIG_OLED_ISPRESENT != 0
SSD1306_handle* GPIO_GetSSD1306Handle()
{
    return &m_ssd1306;
}
#endif