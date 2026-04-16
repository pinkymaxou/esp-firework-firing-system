#include "SSD1306.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
// Fonts
#include "fonts/FreeMono12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "fonts/Tiny3x3a2pt7b.h"
#include "fonts/FreeSerif9pt7b.h"
#include "fonts/Picopixel.h"

#define TAG "SSD1306"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

bool SSD1306::init(i2c_master_dev_handle_t i2c_dev, const Config& config)
{
    m_i2c_dev = i2c_dev;
    m_config = config;
    m_width = 128;
    m_height = 64;
    m_text_color = true;
    m_buffer_len = (m_height * m_width) / 8;
    m_buffer = (uint8_t*)malloc(sizeof(uint8_t) * m_buffer_len);
    m_is_init = false;

    if (config.pin_reset >= 0)
    {
        gpio_set_direction((gpio_num_t)config.pin_reset, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)config.pin_reset, false);
        vTaskDelay(pdMS_TO_TICKS(50) + 1);
        gpio_set_level((gpio_num_t)config.pin_reset, true);
    }

    if (!sendCommand1(SSD1306_DISPLAYOFF))
        return false;

    sendCommand1(SSD1306_SETDISPLAYCLOCKDIV);
    sendCommand1(0xF0);
    sendCommand1(SSD1306_SETMULTIPLEX);
    sendCommand1(m_height - 1);
    sendCommand1(SSD1306_SETDISPLAYOFFSET);
    sendCommand1(0x00);
    sendCommand1(SSD1306_SETSTARTLINE);
    sendCommand1(SSD1306_CHARGEPUMP);
    sendCommand1(0x14);
    sendCommand1(SSD1306_MEMORYMODE);
    sendCommand1(0x00);
    sendCommand1(SSD1306_SEGREMAP | 0x01);
    sendCommand1(SSD1306_COMSCANDEC);
    sendCommand1(SSD1306_SETCOMPINS);
    sendCommand1(0x12);
    sendCommand1(SSD1306_SETCONTRAST);
    sendCommand1(0xCF);
    sendCommand1(SSD1306_SETPRECHARGE);
    sendCommand1(0xF1);
    sendCommand1(SSD1306_SETVCOMDETECT);
    sendCommand1(0x40);
    sendCommand1(SSD1306_DISPLAYALLON_RESUME);
    sendCommand1(SSD1306_NORMALDISPLAY);
    sendCommand1(0x2e);
    sendCommand1(SSD1306_DISPLAYON);

    m_font = &FreeSerif9pt7b;
    m_baseline_y = 0;
    for (int i = 0; i < (m_font->last - m_font->first); i++)
    {
        if (m_baseline_y < m_font->glyph[i].height)
            m_baseline_y = m_font->glyph[i].height;
    }

    m_is_init = true;
    vTaskDelay(pdMS_TO_TICKS(10) + 1);
    return true;
}

void SSD1306::uninit()
{
    m_is_init = false;
    if (NULL != m_buffer)
        free(m_buffer);
}

void SSD1306::clearDisplay()
{
    if (!m_is_init)
        return;
    memset(m_buffer, 0, m_buffer_len);
}

void SSD1306::updateDisplay()
{
    if (!m_is_init)
        return;
    const uint8_t display_init[] = {
        SSD1306_PAGEADDR,
        0,
        0xFF,
        SSD1306_COLUMNADDR, 0, (uint8_t)(m_width - 1)
    };
    sendCommand(display_init, sizeof(display_init));
    sendData(m_buffer, m_buffer_len);
}

void SSD1306::displayState(bool is_active)
{
    if (!m_is_init)
        return;
    sendCommand1(is_active ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void SSD1306::invertDisplay()
{
    if (!m_is_init)
        return;
    sendCommand1(SSD1306_INVERTDISPLAY);
}

void SSD1306::normalDisplay()
{
    if (!m_is_init)
        return;
    sendCommand1(SSD1306_NORMALDISPLAY);
}

void SSD1306::setPixel(uint16_t x, uint16_t y)
{
    if (!m_is_init || !isXYValid(x, y))
        return;
    m_buffer[x + (y >> 3) * m_width] |= (1 << (y & 7));
}

void SSD1306::clearPixel(uint16_t x, uint16_t y)
{
    if (!m_is_init || !isXYValid(x, y))
        return;
    m_buffer[x + (y >> 3) * m_width] &= ~(1 << (y & 7));
}

int SSD1306::drawChar(uint16_t x, uint16_t y, unsigned char c)
{
    if (!m_is_init)
        return 0;
    if (c < m_font->first || c > m_font->last)
        return 0;

    const int index = c - m_font->first;
    const GFXglyph* glyph = m_font->glyph + index;
    const uint8_t* bitmap = m_font->bitmap + glyph->bitmapOffset;
    const uint8_t bitsize = glyph->width * glyph->height;
    uint8_t byte = 0;

    for (int i = 0; i < bitsize; i++)
    {
        const int x1 = i % glyph->width + x + glyph->xOffset;
        const int y1 = i / glyph->width + y + (m_baseline_y + glyph->yOffset);

        if (i % 8 == 0)
            byte = bitmap[i / 8];

        if (byte & 0x80)
        {
            if (m_text_color)
                setPixel(x1, y1);
            else
                clearPixel(x1, y1);
        }
        byte <<= 1;
    }

    return glyph->xAdvance;
}

void SSD1306::drawString(uint16_t x, uint16_t y, const char* str)
{
    if (!m_is_init)
        return;

    const int len = strlen(str);
    int x1 = x, y1 = y;

    for (int i = 0; i < len; i++)
    {
        const unsigned char c = (unsigned char)str[i];
        if (c == '\r' || c == '\n')
        {
            y1 += m_font->yAdvance;
            x1 = x;
        }
        else
        {
            x1 += drawChar(x1, y1, c);
        }
    }
}

void SSD1306::setTextColor(bool text_color)
{
    m_text_color = text_color;
}

void SSD1306::fillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color)
{
    const uint32_t right = MIN(x + width, m_width - 1);
    const uint32_t bottom = MIN(y + height, m_height - 1);

    for (uint32_t py = y; py < bottom; py++)
    {
        for (uint32_t px = x; px < right; px++)
        {
            if (color)
                setPixel(px, py);
            else
                clearPixel(px, py);
        }
    }
}

void SSD1306::drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color)
{
    const uint32_t right = MIN(x + width, m_width - 1);
    const uint32_t bottom = MIN(y + height, m_height - 1);

    for (uint32_t py = y; py < bottom; py++)
    {
        if (color)
        {
            setPixel(x, py);
            setPixel(right, py);
        }
        else
        {
            clearPixel(x, py);
            clearPixel(right, py);
        }
    }

    for (uint32_t px = x; px < right; px++)
    {
        if (color)
        {
            setPixel(px, y);
            setPixel(px, bottom);
        }
        else
        {
            clearPixel(px, y);
            clearPixel(px, bottom);
        }
    }
}

bool SSD1306::isXYValid(uint16_t x, uint16_t y)
{
    return (x < m_width) && (y < m_height);
}

bool SSD1306::sendCommand1(uint8_t value)
{
    return writeI2C(ControlByte::Command, &value, 1);
}

bool SSD1306::sendCommand(const uint8_t* value, int length)
{
    return writeI2C(ControlByte::Command, value, length);
}

bool SSD1306::sendData(const uint8_t* value, int length)
{
    return writeI2C(ControlByte::Data, value, length);
}

bool SSD1306::writeI2C(ControlByte control_byte, const uint8_t* value, int length)
{
    uint8_t* buf = (uint8_t*)malloc(length + 1);
    if (!buf)
        return false;
    buf[0] = (uint8_t)control_byte;
    memcpy(buf + 1, value, length);
    const esp_err_t ret = i2c_master_transmit(m_i2c_dev, buf, length + 1, 1000);
    free(buf);
    if (ESP_OK != ret)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        return false;
    }
    return true;
}
