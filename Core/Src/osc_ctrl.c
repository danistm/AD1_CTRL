/* osc_ctrl.c — AD-1 CTL v2.0 (software I2C on PC9=SCL / PA8=SDA)      */
#include "osc_ctrl.h"
#include "i2c_bitbang.h"
#include "main.h"

static I2CBB_Bus oscBus;

void Osc_Init(void)
{
    /* v2.0 board: the pin LABELLED SDA carries SCL and vice versa.
     * Labels below are the CubeMX ones after the pins are turned into
     * plain GPIO outputs (keep the names, they document the mistake). */
    const I2CBB_Config cfg =
    {
        .sclPort  = OSC_I2C3_SDA_GPIO_Port, .sclPin = OSC_I2C3_SDA_Pin,  /* PC9 */
        .sdaPort  = OSC_I2C3_SCL_GPIO_Port, .sdaPin = OSC_I2C3_SCL_Pin,  /* PA8 */
        .speedKHz = 400u
    };
    I2CBB_Init(&oscBus, &cfg);
}

void Osc_Probe(bool *pcaOk, bool *dacOk)
{
    if (pcaOk) *pcaOk = (I2CBB_IsDeviceReady(&oscBus, PCA9536_ADDR8, 2u) == HAL_OK);
    if (dacOk) *dacOk = (I2CBB_IsDeviceReady(&oscBus, DAC8571_ADDR8, 2u) == HAL_OK);
}

HAL_StatusTypeDef Pca9536_WriteReg(uint8_t reg, uint8_t val)
{
    return I2CBB_MemWrite(&oscBus, PCA9536_ADDR8, reg, &val, 1u);
}

HAL_StatusTypeDef Pca9536_ReadReg(uint8_t reg, uint8_t *val)
{
    return I2CBB_MemRead(&oscBus, PCA9536_ADDR8, reg, val, 1u);
}

HAL_StatusTypeDef Dac8571_Write(uint16_t code)
{
    const uint8_t frame[3] =
    {
        DAC8571_CTRL_WRITE_UPDATE,
        (uint8_t)(code >> 8),            /* MSB first                  */
        (uint8_t)(code & 0xFFu)
    };
    return I2CBB_MasterTransmit(&oscBus, DAC8571_ADDR8, frame, 3u);
}

uint32_t Osc_BusErrorCount(void)
{
    return oscBus.errCount;
}
