/* board_leds.c — AD-1 CTL v2.0: CLIP_1..4 on PG6/PG5/PG4/PG3, active HIGH */
#include "board_leds.h"
#include "main.h"

#define CLIP_LED_ACTIVE_LOW  0

void Board_SetClipLed(uint8_t ch, bool on)
{
    static GPIO_TypeDef* const port[4] =
        { CLIP_1_GPIO_Port, CLIP_2_GPIO_Port, CLIP_3_GPIO_Port, CLIP_4_GPIO_Port };
    static const uint16_t pin[4] =
        { CLIP_1_Pin, CLIP_2_Pin, CLIP_3_Pin, CLIP_4_Pin };

    if (ch >= 4u) return;
#if CLIP_LED_ACTIVE_LOW
    on = !on;
#endif
    HAL_GPIO_WritePin(port[ch], pin[ch], on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Board_AllClipLeds(bool on)
{
    for (uint8_t i = 0u; i < 4u; i++) Board_SetClipLed(i, on);
}
