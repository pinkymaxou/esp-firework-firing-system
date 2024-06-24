#ifndef HWGPIO_H_
#define HWGPIO_H_

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "SSD1306.h"
#include "HWConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void HWGPIO_Init();

void HWGPIO_SetSanityLED(bool isEnabled, bool isArmed);

void HWGPIO_ClearRelayBus();

void HWGPIO_WriteSingleRelay(uint32_t u32OutputIndex, bool bValue);

void HWGPIO_WriteMasterPowerRelay(bool bValue);

bool HWGPIO_ReadMasterPowerSense();

bool HWGPIO_ReadConnectionSense();

void HWGPIO_RefreshLEDStrip();

void HWGPIO_SetOutputRelayStatusColor(uint32_t u32OutputIndex, uint8_t r, uint8_t g, uint8_t b);

bool HWGPIO_IsEncoderSwitchON();

uint32_t HWGPIO_GetRelayArea(uint32_t u32OutputIndex);

int32_t HWGPIO_GetEncoderCount();

SSD1306_handle* GPIO_GetSSD1306Handle();

#ifdef __cplusplus
}
#endif

#endif