/*
 * board_leds.h
 *
 *  Created on: 14 Aug 2026
 *      Author: Delinea
 */

#ifndef BOARD_LEDS_H
#define BOARD_LEDS_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void Board_SetClipLed(uint8_t ch, bool on);   /* ch = 0..3 */
#ifdef __cplusplus
}
#endif
#endif
