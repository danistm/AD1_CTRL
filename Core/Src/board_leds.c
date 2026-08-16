/*
 * board_leds.c
 *
 *  Created on: 14 Aug 2026
 *      Author: Delinea
 */

#include "board_leds.h"
#include "main.h"

/* set to 1 if the LEDs are active-low (direct MCU sink drive) */
#define CLIP_LED_ACTIVE_LOW  0

void Board_SetClipLed(uint8_t ch, bool on)
{
    static GPIO_TypeDef* const port[4] =
        { CLIP_1_GPIO_Port, CLIP_2_GPIO_Port, CLIP_3_GPIO_Port, CLIP_4_GPIO_Port };
    static const uint16_t pin[4] =
        { CLIP_1_Pin, CLIP_2_Pin, CLIP_3_Pin, CLIP_4_Pin };

    if (ch >= 4) return;
#if CLIP_LED_ACTIVE_LOW
    on = !on;
#endif
    HAL_GPIO_WritePin(port[ch], pin[ch], on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
