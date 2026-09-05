/*
 * backlight.h — key LEDs (TIM3 CH1) and display (TIM2 CH1) brightness,
 * 11 levels each, geometric tables. Both timers: ARR=7999 @ 20 kHz.
 */
#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Backlight_Start(void);                 /* start both PWM channels  */
void Backlight_SetKey(uint8_t level10);     /* 0 = off .. 10 = full     */
void Backlight_SetDisplay(uint8_t level10); /* 0 = MIN (never dark) ..10*/

#ifdef __cplusplus
}
#endif
#endif
