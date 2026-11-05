/* osc_ctrl.h — oscillator bus on the CTRL v2.0 board.
 *
 * Bus = software I2C (see i2c_bitbang.h) because the I2C3 traces are
 * crossed on v2.0: SCL is on PC9, SDA is on PA8. The I2C3 peripheral
 * must be REMOVED in CubeMX and the two pins set as GPIO outputs.
 *
 * Devices (on the ADC board, behind the ISO1640):
 *   PCA9536  0x41  4-bit expander  -> VCXO family enables (48 / 44.1)
 *   DAC8571  0x4C  16-bit DAC      -> VCXO control voltage (FLL)
 */
#ifndef OSC_CTRL_H
#define OSC_CTRL_H
#ifdef __cplusplus
extern "C" {
#endif
#include "stm32h7xx_hal.h"
#include <stdbool.h>

#define PCA9536_ADDR8     (0x41u << 1)
#define DAC8571_ADDR8     (0x4Cu << 1)    /* A0 = GND                   */

/* PCA9536 registers */
#define PCA9536_REG_IN    (0x00u)
#define PCA9536_REG_OUT   (0x01u)
#define PCA9536_REG_POL   (0x02u)
#define PCA9536_REG_CFG   (0x03u)         /* 1 = input (POR), 0 = output */

/* DAC8571 control byte: write to DAC register and update output       */
#define DAC8571_CTRL_WRITE_UPDATE (0x10u)

void Osc_Init(void);                        /* bit-bang bus up, recovered */
void Osc_Probe(bool *pcaOk, bool *dacOk);   /* ACK probes, log only       */

/* PCA9536 raw access (family switching logic is step 2)               */
HAL_StatusTypeDef Pca9536_WriteReg(uint8_t reg, uint8_t val);
HAL_StatusTypeDef Pca9536_ReadReg (uint8_t reg, uint8_t *val);

/* DAC8571: 16-bit code, output updates on the STOP (step 4 FLL)       */
HAL_StatusTypeDef Dac8571_Write(uint16_t code);

uint32_t Osc_BusErrorCount(void);

#ifdef __cplusplus
}
#endif
#endif
