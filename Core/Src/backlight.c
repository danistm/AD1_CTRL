/*
 * backlight.c — AD-1 CTL
 */
#include "backlight.h"
#include "tim.h"

/* keys: level 0 off, 1..10 geometric 80..8000 (1.668x/step)           */
static const uint16_t keyPwmTable[11] =
{ 0, 80, 133, 223, 372, 620, 1035, 1726, 2879, 4802, 8000 };

/* display: level 0 = MIN 240 (never dark), 0..10 geometric (1.42x/step)*/
static const uint16_t dispPwmTable[11] =
{ 240, 341, 484, 687, 976, 1386, 1968, 2795, 3968, 5635, 8000 };

void Backlight_Start(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

void Backlight_SetKey(uint8_t level10)
{
    if (level10 > 10u) level10 = 10u;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, keyPwmTable[level10]);
}

void Backlight_SetDisplay(uint8_t level10)
{
    if (level10 > 10u) level10 = 10u;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dispPwmTable[level10]);
}
