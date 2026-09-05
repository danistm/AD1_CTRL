/*
 * eeprom_24aa02.h — Microchip 24AA02E64 (256 B I2C EEPROM + EUI-64)
 * Bus: I2C1 (hi2c1). 7-bit address 0x50 (no address pins on E64).
 * Page size 8 bytes, write cycle <= 5 ms (ACK-polled).
 * 0xF8..0xFF = factory EUI-64, read-only.
 */
#ifndef EEPROM_24AA02_H
#define EEPROM_24AA02_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdbool.h>

#define EEPROM_SIZE         (256u)
#define EEPROM_PAGE_SIZE    (8u)
#define EEPROM_EUI64_ADDR   (0xF8u)
#define EEPROM_USER_END     (0xF8u)      /* user area 0x00..0xF7       */

bool EEPROM_IsPresent(void);
HAL_StatusTypeDef EEPROM_Read (uint8_t addr, uint8_t *buf, uint16_t len);
/* page-aware write with ACK-polling between pages; addr+len <= 0xF8  */
HAL_StatusTypeDef EEPROM_Write(uint8_t addr, const uint8_t *buf, uint16_t len);
HAL_StatusTypeDef EEPROM_ReadEUI64(uint8_t id[8]);

#ifdef __cplusplus
}
#endif
#endif
