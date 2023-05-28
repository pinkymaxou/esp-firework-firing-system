#ifndef HARDWAREGPIO_H_
#define HARDWAREGPIO_H_

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void HARDWAREGPIO_Init();

void HARDWAREGPIO_SetSanityLED(bool isEnabled);

void HARDWAREGPIO_ClearRelayBus();

void HARDWAREGPIO_WriteSingleRelay(uint32_t u32OutputIndex, bool bValue);

void HARDWAREGPIO_WriteMasterPowerRelay(bool bValue);

bool HARDWAREGPIO_ReadMasterPowerSense();

bool HARDWAREGPIO_ReadConnectionSense();

void HARDWAREGPIO_RefreshLEDStrip();

void HARDWAREGPIO_SetOutputRelayStatusColor(uint32_t u32OutputIndex, uint8_t r, uint8_t g, uint8_t b);

#endif