#include "UIArmed.hpp"
#include "assets/BitmapPotato.h"
#include <string.h>

void UIArmed::onEnter(void)
{
    //drawScreen();
}

void UIArmed::onExit(void)
{

}

void UIArmed::onEncoderMove(BTEvent btn_event, int32_t click_count)
{

}

void UIArmed::onTick(void)
{
    if ( (xTaskGetTickCount() - m_last_change_ticks) > pdMS_TO_TICKS(500) )
    {
        m_last_change_ticks = xTaskGetTickCount();
        drawScreen();
        m_altern_image = !m_altern_image;
    }
}

void UIArmed::drawScreen()
{
    #if HWCONFIG_OLED_ISPRESENT != 0
    SSD1306* display = HWGPIO::getSSD1306Handle();
    //display->clearDisplay();
    if (m_altern_image)
        memcpy(display->m_buffer, g_alert_data, g_alert_data_len);
    else
        memcpy(display->m_buffer, g_firework_data, g_firework_data_len);
    display->drawString( 60, 15, "ARMED");
    display->updateDisplay();
    #endif
}
