/*
 * eeprom_24aa02.c — AD-1 CTL
 */
#include "eeprom_24aa02.h"
#include "i2c.h"                       /* hi2c1                         */

#define EEP_I2C        (&hi2c1)
#define EEP_ADDR8      (0x50u << 1)
#define EEP_TIMEOUT_MS (10u)
#define EEP_WRITE_POLL (20u)           /* ACK polls, ~1 ms apart        */

bool EEPROM_IsPresent(void)
{
    return HAL_I2C_IsDeviceReady(EEP_I2C, EEP_ADDR8, 2u, EEP_TIMEOUT_MS)
           == HAL_OK;
}

HAL_StatusTypeDef EEPROM_Read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    if ((uint16_t)addr + len > EEPROM_SIZE) return HAL_ERROR;
    return HAL_I2C_Mem_Read(EEP_I2C, EEP_ADDR8, addr, I2C_MEMADD_SIZE_8BIT,
                            buf, len, EEP_TIMEOUT_MS + len);
}

static HAL_StatusTypeDef wait_write_done(void)
{
    /* device NACKs while the internal write cycle runs (<= 5 ms)       */
    for (uint8_t i = 0u; i < EEP_WRITE_POLL; i++)
    {
        if (HAL_I2C_IsDeviceReady(EEP_I2C, EEP_ADDR8, 1u, 2u) == HAL_OK)
            return HAL_OK;
        HAL_Delay(1u);
    }
    return HAL_TIMEOUT;
}

HAL_StatusTypeDef EEPROM_Write(uint8_t addr, const uint8_t *buf, uint16_t len)
{
    if ((uint16_t)addr + len > EEPROM_USER_END) return HAL_ERROR;

    while (len > 0u)
    {
        /* bytes remaining in the current 8-byte page                   */
        uint16_t chunk = EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE);
        if (chunk > len) chunk = len;

        HAL_StatusTypeDef st = HAL_I2C_Mem_Write(EEP_I2C, EEP_ADDR8, addr,
                                                 I2C_MEMADD_SIZE_8BIT,
                                                 (uint8_t*)buf, chunk,
                                                 EEP_TIMEOUT_MS);
        if (st != HAL_OK) return st;

        st = wait_write_done();
        if (st != HAL_OK) return st;

        addr = (uint8_t)(addr + chunk);
        buf  += chunk;
        len  -= chunk;
    }
    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_ReadEUI64(uint8_t id[8])
{
    return EEPROM_Read(EEPROM_EUI64_ADDR, id, 8u);
}
