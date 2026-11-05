/*
 * i2c_bitbang.h — software I2C master on two open-drain GPIOs
 *
 * Why it exists: on the CTRL v2.0 board the I2C3 SCL/SDA traces to the
 * oscillator bus (PCA9536 + DAC8571, behind an ISO1640) are crossed.
 * The H7A3 cannot remap I2C3 onto the swapped pins, so the hardware
 * peripheral is abandoned and the bus is driven in software:
 *     PC9 (labelled OSC_I2C3_SDA) -> actually SCL
 *     PA8 (labelled OSC_I2C3_SCL) -> actually SDA
 * The API mirrors the HAL I2C calls (8-bit address, HAL status codes)
 * so callers are source-compatible with a future hardware-I2C board.
 *
 * Properties
 *  - Blocking. One 3-byte DAC write ~= 90 us at 400 kHz.
 *  - Interrupt-tolerant: an interrupt only lengthens a phase; I2C has
 *    no maximum times, so no critical sections are needed.
 *  - Clock-stretch tolerant and isolator-delay tolerant: SCL is read
 *    back after every release before the high period is timed.
 *  - Timing from DWT->CYCCNT, derived from SystemCoreClock at init.
 */
#ifndef I2C_BITBANG_H
#define I2C_BITBANG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdbool.h>

typedef struct
{
    GPIO_TypeDef *sclPort; uint16_t sclPin;
    GPIO_TypeDef *sdaPort; uint16_t sdaPin;
    uint32_t      speedKHz;        /* 100 or 400                        */
} I2CBB_Config;

typedef struct
{
    I2CBB_Config cfg;
    uint32_t     tLow, tHigh, tSu, tHd, tSta, tBuf;  /* CPU cycles      */
    uint32_t     sclTimeout;                    /* cycles, stretch wait */
    uint32_t     errCount;
} I2CBB_Bus;

/* Configures both pins as open-drain outputs (released), starts the
 * cycle counter, computes timings, and performs a bus recovery if SDA
 * is found held low (slave mid-transaction after an MCU reset).       */
void I2CBB_Init(I2CBB_Bus *bus, const I2CBB_Config *cfg);

/* HAL-style primitives. addr8 = 7-bit address << 1.                   */
HAL_StatusTypeDef I2CBB_IsDeviceReady(I2CBB_Bus *bus, uint16_t addr8,
                                      uint32_t trials);
HAL_StatusTypeDef I2CBB_MasterTransmit(I2CBB_Bus *bus, uint16_t addr8,
                                       const uint8_t *data, uint16_t len);
HAL_StatusTypeDef I2CBB_MasterReceive(I2CBB_Bus *bus, uint16_t addr8,
                                      uint8_t *data, uint16_t len);
HAL_StatusTypeDef I2CBB_MemWrite(I2CBB_Bus *bus, uint16_t addr8, uint8_t reg,
                                 const uint8_t *data, uint16_t len);
HAL_StatusTypeDef I2CBB_MemRead(I2CBB_Bus *bus, uint16_t addr8, uint8_t reg,
                                uint8_t *data, uint16_t len);

/* Clocks SCL up to 9 times until SDA is released, then issues STOP.
 * Returns true if SDA is high afterwards.                             */
bool I2CBB_BusRecover(I2CBB_Bus *bus);

#ifdef __cplusplus
}
#endif
#endif
