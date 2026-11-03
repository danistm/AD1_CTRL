#ifndef BOARD_LEDS_H
#define BOARD_LEDS_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void Board_SetClipLed(uint8_t ch, bool on);   /* ch = 0..3 */
void Board_AllClipLeds(bool on);
#ifdef __cplusplus
}
#endif
#endif
