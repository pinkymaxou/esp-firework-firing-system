#pragma once

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "gfxfont.h"
#include <stdint.h>

#define SSD1306_BLACK     0
#define SSD1306_WHITE     1
#define SSD1306_INVERSE   2

#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON        0xA5
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_COMSCANINC          0xC0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_SETLOWCOLUMN        0x00
#define SSD1306_SETHIGHCOLUMN       0x10
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_EXTERNALVCC         0x01
#define SSD1306_SWITCHCAPVCC        0x02

class SSD1306
{
public:
    struct Config
    {
        uint8_t i2c_address;
        gpio_num_t pin_reset;
    };

    static constexpr Config CONFIG_DEFAULT_128x64 = { .i2c_address = 0x3C, .pin_reset = (gpio_num_t)-1 };

    uint8_t* m_buffer = nullptr;
    uint32_t m_buffer_len = 0;

    bool init(i2c_master_dev_handle_t i2c_dev, const Config& config);
    void uninit();

    void setPixel(uint16_t x, uint16_t y);
    void clearPixel(uint16_t x, uint16_t y);
    void clearDisplay();
    void displayState(bool is_active);
    void invertDisplay();
    void normalDisplay();
    int drawChar(uint16_t x, uint16_t y, unsigned char c);
    void setTextColor(bool text_color);
    void drawString(uint16_t x, uint16_t y, const char* str);
    void fillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color);
    void drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color);
    void updateDisplay();

private:
    enum class ControlByte : uint8_t
    {
        Command = 0x00,
        Data    = 0x40,
    };

    bool sendCommand1(uint8_t value);
    bool sendCommand(const uint8_t* value, int n);
    bool sendData(const uint8_t* value, int n);
    bool writeI2C(ControlByte control_byte, const uint8_t* value, int length);
    bool isXYValid(uint16_t x, uint16_t y);

    i2c_master_dev_handle_t m_i2c_dev = nullptr;
    Config m_config = {};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    const GFXfont* m_font = nullptr;
    uint32_t m_baseline_y = 0;
    bool m_text_color = true;
    bool m_is_init = false;
};
