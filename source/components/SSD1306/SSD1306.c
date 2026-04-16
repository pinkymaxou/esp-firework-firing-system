#include "SSD1306.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// Fonts
#include "fonts/FreeMono12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "fonts/Tiny3x3a2pt7b.h"
#include "fonts/FreeSerif9pt7b.h"
#include "fonts/Picopixel.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef enum
{
    ControlByte_Command = 0x00,
    ControlByte_Data = 0x40
} ControlByte;

static bool sendCommand1(SSD1306_handle* pHandle, uint8_t value);
static bool sendCommand(SSD1306_handle* pHandle, const uint8_t* value, int n);

static bool sendData(SSD1306_handle* pHandle, const uint8_t* value, int n);
static bool writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, const uint8_t* value, int length);

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y);

bool SSD1306_Init(SSD1306_handle* pHandle, i2c_master_dev_handle_t i2c_dev, SSD1306_config* pconfig)
{
    pHandle->i2c_dev = i2c_dev;
    pHandle->sConfig = *pconfig;

    pHandle->width = 128;
    pHandle->height = 64;

    pHandle->textColor = true;

    pHandle->bufferLen = (pHandle->height * pHandle->width) / 8;
    pHandle->buffer = malloc(sizeof(uint8_t) * pHandle->bufferLen);

    pHandle->isInit = false;

    // Clear screen
    SSD1306_ClearDisplay(pHandle);

    // Reset
    if (pconfig->pinReset >= 0)
    {
        gpio_set_direction((gpio_num_t)pconfig->pinReset, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)pconfig->pinReset, false);
        vTaskDelay(pdMS_TO_TICKS(50)+1);
        gpio_set_level((gpio_num_t)pconfig->pinReset, true);
    }

    if (!sendCommand1(pHandle, SSD1306_DISPLAYOFF))
    {
        return false;
    }

    sendCommand1(pHandle, SSD1306_SETDISPLAYCLOCKDIV);
    sendCommand1(pHandle, 0xF0); // Increase speed of the display max ~96Hz
    sendCommand1(pHandle, SSD1306_SETMULTIPLEX);
    sendCommand1(pHandle, pHandle->height - 1);
    sendCommand1(pHandle, SSD1306_SETDISPLAYOFFSET);
    sendCommand1(pHandle, 0x00);

    sendCommand1(pHandle, SSD1306_SETSTARTLINE);
    sendCommand1(pHandle, SSD1306_CHARGEPUMP);
    sendCommand1(pHandle, 0x14);
    sendCommand1(pHandle, SSD1306_MEMORYMODE);
    sendCommand1(pHandle, 0x00);
    sendCommand1(pHandle, SSD1306_SEGREMAP|0x01);
    sendCommand1(pHandle, SSD1306_COMSCANDEC);
    sendCommand1(pHandle, SSD1306_SETCOMPINS);
    sendCommand1(pHandle, 0x12);

    sendCommand1(pHandle, SSD1306_SETCONTRAST);
    sendCommand1(pHandle, 0xCF);

    sendCommand1(pHandle, SSD1306_SETPRECHARGE);
    sendCommand1(pHandle, 0xF1);
    sendCommand1(pHandle, SSD1306_SETVCOMDETECT); //0xDB, (additionally needed to lower the contrast)
    sendCommand1(pHandle, 0x40);        //0x40 default, to lower the contrast, put 0
    sendCommand1(pHandle, SSD1306_DISPLAYALLON_RESUME);
    sendCommand1(pHandle, SSD1306_NORMALDISPLAY);
    sendCommand1(pHandle, 0x2e);            // stop scroll
    sendCommand1(pHandle, SSD1306_DISPLAYON);

    // https://rop.nl/truetype2gfx/
    //pHandle->font = &Tiny3x3a2pt7b;
    //pHandle->font = &FreeMono7pt7b;
    //pHandle->font = &Picopixel;
    pHandle->font = &FreeSerif9pt7b;
    // Find the baseline
    pHandle->baselineY = 0;

    for(int i = 0; i < (pHandle->font->last - pHandle->font->first); i++)
    {
        if (pHandle->baselineY < pHandle->font->glyph[i].height)
            pHandle->baselineY = pHandle->font->glyph[i].height;
    }

    pHandle->isInit = true;
    // Sometime to help initialization ..
    vTaskDelay(pdMS_TO_TICKS(10)+1);
    return true;
}

void SSD1306_Uninit(SSD1306_handle* pHandle)
{
    pHandle->isInit = false;
    if (NULL != pHandle->buffer)
    {
        free(pHandle->buffer);
    }
}

void SSD1306_ClearDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->isInit)
        return;
    memset(pHandle->buffer, 0, pHandle->bufferLen);
}

void SSD1306_UpdateDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->isInit)
        return;
    const uint8_t displayInit[] = {
      SSD1306_PAGEADDR,
      0,                      // Page start address
      0xFF,                   // Page end (not really, but works here)
      SSD1306_COLUMNADDR, 0, pHandle->width - 1}; // Column start address

    sendCommand(pHandle, displayInit, sizeof(displayInit));
    sendData(pHandle, pHandle->buffer, pHandle->bufferLen);
}

void SSD1306_DisplayState(SSD1306_handle* pHandle, bool isActive)
{
    if (!pHandle->isInit)
        return;
    if (isActive)
        sendCommand1(pHandle, SSD1306_DISPLAYON);
    else
        sendCommand1(pHandle, SSD1306_DISPLAYOFF);
}

void SSD1306_InvertDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->isInit)
        return;
    sendCommand1(pHandle, SSD1306_INVERTDISPLAY);
}

void SSD1306_NormalDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->isInit)
        return;
    sendCommand1(pHandle, SSD1306_NORMALDISPLAY);
}

void SSD1306_SetPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!pHandle->isInit || !IsXYValid(pHandle, x, y))
        return;
    pHandle->buffer[x + (y >> 3) * pHandle->width] |=  (1 << (y & 7));
}

void SSD1306_ClearPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!pHandle->isInit || !IsXYValid(pHandle, x, y))
        return;
    pHandle->buffer[x + (y >> 3) * pHandle->width] &=  ~(1 << (y & 7));
}

/*void SSD1306_DrawBitmap(SSD1306_handle* pHandle, const uint8_t* bitmapDatas, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{

}
*/
int SSD1306_DrawChar(SSD1306_handle* pHandle, uint16_t x, uint16_t y, unsigned char c)
{
    if (!pHandle->isInit)
        return 0;
    // Character is not supported by the font
    if (c < pHandle->font->first || c > pHandle->font->last)
        return 0;

    const int index = c - pHandle->font->first;

    const GFXglyph* glyph = pHandle->font->glyph + index;

    const uint8_t* pMap = pHandle->font->bitmap + glyph->bitmapOffset;

    const uint8_t bitsize = glyph->width * glyph->height;

    uint8_t byte = 0;

    for(int i = 0; i < bitsize; i++)
    {
        const int x1 = i % glyph->width + x + glyph->xOffset;
        const int y1 = i / glyph->width + y + (pHandle->baselineY + glyph->yOffset);

        if (i % 8 == 0)
            byte = pMap[i / 8];

        if (byte & 0x80)
        {
            if (pHandle->textColor)
                SSD1306_SetPixel(pHandle, x1, y1);
            else
                SSD1306_ClearPixel(pHandle, x1, y1);
        }
        byte <<= 1;
    }

    return glyph->xAdvance;
}

void SSD1306_DrawString(SSD1306_handle* pHandle, uint16_t x, uint16_t y, const char* str)
{
    if (!pHandle->isInit)
        return;

    const int len = strlen(str);
    int x1 = x, y1 = y;

    for(int i = 0; i < len; i++)
    {
        const unsigned char c = (unsigned char)str[i];
        if (c == '\r' || c == '\n')
        {
            y1 += pHandle->font->yAdvance;
            x1 = x;
        }
        else
        {
            int xAdvance = SSD1306_DrawChar(pHandle, x1, y1, c);
            x1 += xAdvance;
        }
    }
}

void SSD1306_SetTextColor(SSD1306_handle* pHandle, bool textColor)
{
    pHandle->textColor = textColor;
}

void SSD1306_FillRect(SSD1306_handle* pHandle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color)
{
    const uint32_t right = MIN(x + width, pHandle->width - 1);
    const uint32_t bottom = MIN(y + height, pHandle->height - 1);

    for(uint32_t py = y; py < bottom; py++)
    {
        for(uint32_t px = x; px < right; px++)
        {
            if (color)
                SSD1306_SetPixel(pHandle, px, py);
            else
                SSD1306_ClearPixel(pHandle, px, py);
        }
    }
}

void SSD1306_DrawRect(SSD1306_handle* pHandle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool color)
{
    const uint32_t right = MIN(x + width, pHandle->width - 1);
    const uint32_t bottom = MIN(y + height, pHandle->height - 1);

    for(uint32_t py = y; py < bottom; py++)
    {
        if (color)
        {
            SSD1306_SetPixel(pHandle, x, py);
            SSD1306_SetPixel(pHandle, right, py);
        }
        else
        {
            SSD1306_ClearPixel(pHandle, x, py);
            SSD1306_ClearPixel(pHandle, right, py);
        }
    }

    for(uint32_t px = x; px < right; px++)
    {
        if (color)
        {
            SSD1306_SetPixel(pHandle, px, y);
            SSD1306_SetPixel(pHandle, px, bottom);
        }
        else
        {
            SSD1306_ClearPixel(pHandle, px, y);
            SSD1306_ClearPixel(pHandle, px, bottom);
        }
    }
}

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    return (x < pHandle->width) && (y < pHandle->height);
}

static bool sendCommand1(SSD1306_handle* pHandle, uint8_t value)
{
    return writeI2C(pHandle, ControlByte_Command, &value, 1);
}

static bool sendCommand(SSD1306_handle* pHandle, const uint8_t* value, int length)
{
    return writeI2C(pHandle, ControlByte_Command, value, length);
}

static bool sendData(SSD1306_handle* pHandle, const uint8_t* value, int length)
{
    return writeI2C(pHandle, ControlByte_Data, value, length);
}

static bool writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, const uint8_t* value, int length)
{
    uint8_t* buf = malloc(length + 1);
    if (!buf) return false;
    buf[0] = (uint8_t)controlByte;
    memcpy(buf + 1, value, length);
    const esp_err_t ret = i2c_master_transmit(pHandle->i2c_dev, buf, length + 1, 1000);
    free(buf);
    if (ESP_OK != ret)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        return false;
    }
    return true;
}
