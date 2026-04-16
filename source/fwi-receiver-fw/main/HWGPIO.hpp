#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "SSD1306.hpp"
#include "HWConfig.h"

namespace HWGPIO
{
    void init();

    void setSanityLED(bool is_enabled, bool is_armed);

    void clearRelayBus();

    void writeSingleRelay(uint32_t output_index, bool value);

    void writeMasterPowerRelay(bool value);

    bool readMasterPowerSense();

    bool readConnectionSense();

    void refreshLEDStrip();

    void setOutputRelayStatusColor(uint32_t output_index, uint8_t r, uint8_t g, uint8_t b);

    bool isEncoderSwitchON();

    uint32_t getRelayArea(uint32_t output_index);

    int32_t getEncoderCount();

    SSD1306* getSSD1306Handle();
}
