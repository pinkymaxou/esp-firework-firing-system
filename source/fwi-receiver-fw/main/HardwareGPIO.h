#ifndef HARDWAREGPIO_H_
#define HARDWAREGPIO_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void HARDWAREGPIO_Init();

void HARDWAREGPIO_SetSanityLED(float fltPercent);

#endif