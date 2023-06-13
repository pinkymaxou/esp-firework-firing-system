#include "HardwareGPIO.h"
#include "HWConfig.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Settings.h"
#include "led_strip.h"

#define TAG "HardwareGPIO"

static led_strip_handle_t led_strip;
#if HWCONFIG_OLED_ISPRESENT != 0
static SSD1306_handle m_ssd1306;
#endif

static gpio_num_t m_busPins[HWCONFIG_OUTPUTBUS_COUNT] =
{
    HWCONFIG_RELAY_BUS_1_PIN, HWCONFIG_RELAY_BUS_2_PIN, HWCONFIG_RELAY_BUS_3_PIN, HWCONFIG_RELAY_BUS_4_PIN,
    HWCONFIG_RELAY_BUS_5_PIN, HWCONFIG_RELAY_BUS_6_PIN, HWCONFIG_RELAY_BUS_7_PIN, HWCONFIG_RELAY_BUS_8_PIN,

    HWCONFIG_RELAY_BUS_9_PIN, HWCONFIG_RELAY_BUS_10_PIN, HWCONFIG_RELAY_BUS_11_PIN, HWCONFIG_RELAY_BUS_12_PIN,
    HWCONFIG_RELAY_BUS_13_PIN, HWCONFIG_RELAY_BUS_14_PIN, HWCONFIG_RELAY_BUS_15_PIN, HWCONFIG_RELAY_BUS_16_PIN
};

static gpio_num_t m_busAreaPins[HWCONFIG_OUTPUTAREA_COUNT] =
{
    HWCONFIG_RELAY_MOSA1,
    HWCONFIG_RELAY_MOSA2,
    HWCONFIG_RELAY_MOSA3
};

void HARDWAREGPIO_Init()
{
    // Sanity LEDs
    gpio_set_direction(HWCONFIG_STATUSLED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(HWCONFIG_SANITY2_PIN, GPIO_MODE_OUTPUT);

    // Initialize relay BUS
    for(int i = 0; i < HWCONFIG_OUTPUTBUS_COUNT; i++)
    {
        const gpio_num_t gpio = m_busPins[i];
        gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio, true);
    }

    for(int i = 0; i < HWCONFIG_OUTPUTAREA_COUNT; i++)
    {
        const gpio_num_t gpio = m_busAreaPins[i];
        gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio, true);
    }

    // Relay pin
    gpio_set_direction(HWCONFIG_MASTERPWRRELAY_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWCONFIG_MASTERPWRRELAY_EN, false);

    HARDWAREGPIO_WriteMasterPowerRelay(false);

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

    HARDWAREGPIO_ClearRelayBus();

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = HWCONFIG_STATUSLED_PIN,
        .max_leds = 1+HWCONFIG_OUTPUT_COUNT, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);

	const int i2c_master_port = HWCONFIG_I2C_MASTER_NUM;
    i2c_config_t conf = {0};
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
    const char* szBooting = "Booting ...";
    SSD1306_DrawString(&m_ssd1306, 0, 0, szBooting, strlen(szBooting));
    SSD1306_UpdateDisplay(&m_ssd1306);
    #endif
}

void HARDWAREGPIO_SetSanityLED(bool isEnabled)
{
    gpio_set_level(HWCONFIG_SANITY2_PIN, !isEnabled);

    /* If the addressable LED is enabled */
    if (isEnabled)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 255, 255, 255);
    }
    else
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    }
}

void HARDWAREGPIO_SetOutputRelayStatusColor(uint32_t u32OutputIndex, uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(led_strip, 1+u32OutputIndex, r, g, b);
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
        gpio_set_level(m_busPins[i], true);

    // Mechanical relay, give them some time to be sure they are turned off.
    vTaskDelay(pdMS_TO_TICKS(25));
}

void HARDWAREGPIO_WriteSingleRelay(uint32_t u32OutputIndex, bool bValue)
{
    HARDWAREGPIO_ClearRelayBus();
    if (HWCONFIG_OUTPUT_COUNT >= 48)
        return;

    // Activate the right area
    const uint32_t u32AreaIndex = u32OutputIndex/HWCONFIG_OUTPUTBUS_COUNT;
    const gpio_num_t gpioArea = m_busAreaPins[u32AreaIndex];
    gpio_set_level(gpioArea, true);

    gpio_set_level(m_busPins[u32OutputIndex], !bValue);
}

void HARDWAREGPIO_WriteMasterPowerRelay(bool bValue)
{
    gpio_set_level(HWCONFIG_MASTERPWRRELAY_EN, bValue);
    // Mechanical relay, give it some time to turn off.
    if (!bValue)
        vTaskDelay(pdMS_TO_TICKS(250));
}

bool HARDWAREGPIO_ReadMasterPowerSense()
{
    return gpio_get_level(HWCONFIG_MASTERPWRSENSE_IN) == false;
}

bool HARDWAREGPIO_ReadConnectionSense()
{
    return gpio_get_level(HWCONFIG_CONNSENSE_IN) == false;
}

#if HWCONFIG_OLED_ISPRESENT != 0
SSD1306_handle* GPIO_GetSSD1306Handle()
{
    return &m_ssd1306;
}
#endif