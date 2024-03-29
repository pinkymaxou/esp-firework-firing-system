#include "SSD1306.h"
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
static bool sendCommand(SSD1306_handle* pHandle, uint8_t* value, int n);

static bool sendData1(SSD1306_handle* pHandle, uint8_t value);
static bool sendData(SSD1306_handle* pHandle, uint8_t* value, int n);
static bool writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, uint8_t* value, int length);

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y);

bool SSD1306_Init(SSD1306_handle* pHandle, i2c_port_t i2c_port, SSD1306_config* pconfig)
{
    pHandle->i2c_port = i2c_port;
    pHandle->sConfig = *pconfig;

    pHandle->u32Width = 128;
    pHandle->u32Height = 64;

    pHandle->bTextColor = true;

    pHandle->u32BufferLen = (pHandle->u32Height * pHandle->u32Width) / 8;
    pHandle->u8Buffer = malloc(sizeof(uint8_t) * pHandle->u32BufferLen);

    pHandle->bIsInit = false;

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
    sendCommand1(pHandle, pHandle->u32Height - 1);
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
    sendCommand1(pHandle, 0x40);	        //0x40 default, to lower the contrast, put 0
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
    pHandle->u32BaselineY = 0;

    for(int i = 0; i < (pHandle->font->last - pHandle->font->first); i++)
    {
        if (pHandle->u32BaselineY < pHandle->font->glyph[i].height)
            pHandle->u32BaselineY = pHandle->font->glyph[i].height;
    }

    pHandle->bIsInit = true;
    // Sometime to help initialization ..
    vTaskDelay(pdMS_TO_TICKS(10)+1);
    return true;
}

void SSD1306_Uninit(SSD1306_handle* pHandle)
{
    pHandle->bIsInit = false;
    if (pHandle->u8Buffer != NULL)
    {
        free(pHandle->u8Buffer);
    }
}

void SSD1306_ClearDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->bIsInit)
        return;
    memset(pHandle->u8Buffer, 0, pHandle->u32BufferLen);
}

void SSD1306_UpdateDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->bIsInit)
        return;
    const uint8_t displayInit[] = {
      SSD1306_PAGEADDR,
      0,                      // Page start address
      0xFF,                   // Page end (not really, but works here)
      SSD1306_COLUMNADDR, 0, pHandle->u32Width - 1}; // Column start address

    sendCommand(pHandle, displayInit, sizeof(displayInit));
    sendData(pHandle, pHandle->u8Buffer, pHandle->u32BufferLen);
}

void SSD1306_DisplayState(SSD1306_handle* pHandle, bool isActive)
{
    if (!pHandle->bIsInit)
        return;
    if (isActive)
        sendCommand1(pHandle, SSD1306_DISPLAYON);
    else
        sendCommand1(pHandle, SSD1306_DISPLAYOFF);
}

void SSD1306_InvertDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->bIsInit)
        return;
    sendCommand1(pHandle, SSD1306_INVERTDISPLAY);
}

void SSD1306_NormalDisplay(SSD1306_handle* pHandle)
{
    if (!pHandle->bIsInit)
        return;
    sendCommand1(pHandle, SSD1306_NORMALDISPLAY);
}

void SSD1306_SetPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!pHandle->bIsInit || !IsXYValid(pHandle, x, y))
        return;
    pHandle->u8Buffer[x + (y >> 3) * pHandle->u32Width] |=  (1 << (y & 7));
}

void SSD1306_ClearPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!pHandle->bIsInit || !IsXYValid(pHandle, x, y))
        return;
    pHandle->u8Buffer[x + (y >> 3) * pHandle->u32Width] &=  ~(1 << (y & 7));
}

/*void SSD1306_DrawBitmap(SSD1306_handle* pHandle, const uint8_t* pU8BitmapDatas, uint32_t u32X, uint32_t u32Y, uint32_t u32Width, uint32_t u32Height)
{

}
*/
int SSD1306_DrawChar(SSD1306_handle* pHandle, uint16_t x, uint16_t y, unsigned char c)
{
    if (!pHandle->bIsInit)
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
        const int y1 = i / glyph->width + y + (pHandle->u32BaselineY + glyph->yOffset);

        if (i % 8 == 0)
            byte = pMap[i / 8];

        if (byte & 0x80)
        {
            if (pHandle->bTextColor)
                SSD1306_SetPixel(pHandle, x1, y1);
            else
                SSD1306_ClearPixel(pHandle, x1, y1);
        }
        byte <<= 1;
    }

    return glyph->xAdvance;
}

void SSD1306_DrawString(SSD1306_handle* pHandle, uint16_t x, uint16_t y, const char* buffer)
{
    if (!pHandle->bIsInit)
        return;

    const int len = strlen(buffer);
    int x1 = x, y1 = y;

    for(int i = 0; i < len; i++)
    {
        const unsigned char c = (unsigned char)buffer[i];
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

void SSD1306_SetTextColor(SSD1306_handle* pHandle, bool bTextColor)
{
    pHandle->bTextColor = bTextColor;
}

void SSD1306_FillRect(SSD1306_handle* pHandle, uint32_t u32X, uint32_t u32Y, uint32_t u32Width, uint32_t u32Height, bool bColor)
{
    const uint32_t u32Right = MIN(u32X + u32Width, pHandle->u32Width - 1);
    const uint32_t u32Bottom = MIN(u32Y + u32Height, pHandle->u32Height - 1);

    for(uint32_t y = u32Y; y < u32Bottom; y++)
    {
        for(uint32_t x = u32X; x < u32Right; x++)
        {
            if (bColor)
                SSD1306_SetPixel(pHandle, x, y);
            else
                SSD1306_ClearPixel(pHandle, x, y);
        }
    }
}

void SSD1306_DrawRect(SSD1306_handle* pHandle, uint32_t u32X, uint32_t u32Y, uint32_t u32Width, uint32_t u32Height, bool bColor)
{
    const uint32_t u32Right = MIN(u32X + u32Width, pHandle->u32Width - 1);
    const uint32_t u32Bottom = MIN(u32Y + u32Height, pHandle->u32Height - 1);

    for(uint32_t y = u32Y; y < u32Bottom; y++)
    {
        if (bColor)
        {
            SSD1306_SetPixel(pHandle, u32X, y);
            SSD1306_SetPixel(pHandle, u32Right, y);
        }
        else
        {
            SSD1306_ClearPixel(pHandle, u32X, y);
            SSD1306_ClearPixel(pHandle, u32Right, y);
        }
    }

    for(uint32_t x = u32X; x < u32Right; x++)
    {
        if (bColor)
        {
            SSD1306_SetPixel(pHandle, x, u32Y);
            SSD1306_SetPixel(pHandle, x, u32Bottom);
        }
        else
        {
            SSD1306_ClearPixel(pHandle, x, u32Y);
            SSD1306_ClearPixel(pHandle, x, u32Bottom);
        }
    }
}

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    return (x < pHandle->u32Width) && (y < pHandle->u32Height);
}

static bool sendCommand1(SSD1306_handle* pHandle, uint8_t value)
{
    return writeI2C(pHandle, ControlByte_Command, &value, 1);
}

static bool sendCommand(SSD1306_handle* pHandle, uint8_t* value, int length)
{
    return writeI2C(pHandle, ControlByte_Command, value, length);
}

static bool sendData1(SSD1306_handle* pHandle, uint8_t value)
{
    return writeI2C(pHandle, ControlByte_Data, &value, 1);
}

static bool sendData(SSD1306_handle* pHandle, uint8_t* value, int length)
{
    return writeI2C(pHandle, ControlByte_Data, value, length);
}

static bool writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, uint8_t* value, int length)
{
    bool retF = true;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// Write sampling command
	ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_start(cmd));
	ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_byte(cmd, (pHandle->sConfig.i2cAddress<<1), true));
    // CO = 0, D/C = 0 (Data = 1, Command = 0)
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_byte(cmd, (uint8_t)controlByte, true));  // Data mode (bit 6)
    for(int i = 0; i < length; i++)
    {
	    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_byte(cmd, *(value + i), true));
    }
	ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_stop(cmd));

	const esp_err_t ret = i2c_master_cmd_begin(pHandle->i2c_port, cmd, pdMS_TO_TICKS(1000));
	if (ret != ESP_OK)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        retF = false;
    }

	i2c_cmd_link_delete(cmd);
    return retF;
}
