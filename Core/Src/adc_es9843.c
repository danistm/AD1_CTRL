/*
 * adc_es9843.c — AD-1 CTL
 * Location: Core/Src (header in Core/Inc)
 *
 * Register access is BLOCKING (bring-up phase). Runtime traffic will
 * later route through the I2C transaction manager.
 */
#include "adc_es9843.h"
#include "i2c.h"                 /* CubeMX: hi2c2 on PF0/PF1            */
#include "main.h"                /* ADC_EN / ADC_CLIP pin labels        */

#define ES9843_I2C_HANDLE       (&hi2c2)
#define ES9843_I2C_TIMEOUT_MS   (10u)

/* t_data_out after CHIP_EN at 24.576 MHz is ~4.1 ms; generous margin. */
#define ES9843_ENABLE_DELAY_MS  (10u)
#define ES9843_RESET_DELAY_MS   (5u)

static volatile uint32_t i2cErrCount;
static volatile uint8_t  clipFlag;

/* ------------------------------------------------------------------ */
/* Register access                                                     */
/* ------------------------------------------------------------------ */
HAL_StatusTypeDef ES9843_WriteReg(uint8_t reg, uint8_t val)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(ES9843_I2C_HANDLE,
                                             ES9843_I2C_ADDR8, reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             &val, 1u,
                                             ES9843_I2C_TIMEOUT_MS);
    if (st != HAL_OK) i2cErrCount++;
    return st;
}

HAL_StatusTypeDef ES9843_ReadReg(uint8_t reg, uint8_t *val)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(ES9843_I2C_HANDLE,
                                            ES9843_I2C_ADDR8, reg,
                                            I2C_MEMADD_SIZE_8BIT,
                                            val, 1u,
                                            ES9843_I2C_TIMEOUT_MS);
    if (st != HAL_OK) i2cErrCount++;
    return st;
}

HAL_StatusTypeDef ES9843_UpdateBits(uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t cur;
    HAL_StatusTypeDef st = ES9843_ReadReg(reg, &cur);
    if (st != HAL_OK) return st;

    const uint8_t next = (uint8_t)((cur & (uint8_t)~mask) | (val & mask));
    if (next == cur) return HAL_OK;
    return ES9843_WriteReg(reg, next);
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */
void ES9843_SetEnabled(bool en)
{
    HAL_GPIO_WritePin(ADC_EN_GPIO_Port, ADC_EN_Pin,
                      en ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

HAL_StatusTypeDef ES9843_Init(void)
{
    HAL_StatusTypeDef st;

    ES9843_SetEnabled(true);
    HAL_Delay(ES9843_ENABLE_DELAY_MS);

    /* probe first: chip must answer and identify itself               */
    uint8_t id = 0u;
    st = ES9843_ReadReg(ES9843_REG_CHIP_ID, &id);
    if (st != HAL_OK)              return st;
    if (id != ES9843_CHIP_ID_VALUE) return HAL_ERROR;

    /* soft reset to power-on defaults; datasheet: write 0xA0 to keep
     * EN_MCLK_IN set during the reset                                  */
    st = ES9843_WriteReg(ES9843_REG_SYS_CONFIG,
                         ES9843_SOFT_RESET | ES9843_EN_MCLK_IN);
    if (st != HAL_OK) return st;
    HAL_Delay(ES9843_RESET_DELAY_MS);

    /* base configuration:
     *  - clock in enabled, ACLK1, all 4 decimation paths on            */
    st = ES9843_WriteReg(ES9843_REG_SYS_CONFIG,
                         ES9843_EN_MCLK_IN |
                         ES9843_EN_ADC_CH1 | ES9843_EN_ADC_CH2 |
                         ES9843_EN_ADC_CH3 | ES9843_EN_ADC_CH4);
    if (st != HAL_OK) return st;

    /*  - PCM master mode: chip generates BCK (DATA_CLK) and WS (DATA1);
     *    other bits of reg 8 stay at their defaults                     */
    st = ES9843_UpdateBits(ES9843_REG_MASTER_MODE, ES9843_PCM_MASTER_EN,
                           ES9843_PCM_MASTER_EN);
    if (st != HAL_OK) return st;

    /*  - default rate 48 kHz (MASTER_DCLK_DIV reset value is already 7,
     *    written explicitly so the init state is self-documenting)      */
    st = ES9843_WriteReg(ES9843_REG_MASTER_CLK_DIV,
                         (uint8_t)ES9843_RATE_48K);
    if (st != HAL_OK) return st;

    /*  - clock detection circuit on (WS/BCK monitors default on)       */
    st = ES9843_UpdateBits(ES9843_REG_MISC_CLK_CONFIG, ES9843_EN_CLK_DET,
                           ES9843_EN_CLK_DET);
    if (st != HAL_OK) return st;

    /*  - DC blocking on all four channels: this is the reset default,
     *    written explicitly so the init state is self-documenting      */
    st = ES9843_UpdateBits(ES9843_REG_DCBLK_HI, ES9843_DC_BLOCK_EN_ALL,
                           ES9843_DC_BLOCK_EN_ALL);
    if (st != HAL_OK) return st;

    /*  - overload flags CH1..CH4 feed the status OR; peak-det masked   */
    st = ES9843_WriteReg(ES9843_REG_STATUS_MASK, ES9843_MASK_OVERLOAD_ALL);
    if (st != HAL_OK) return st;

    /*  - GPIO9 = OR of Status Bits (overload half of the IRQ line)    */
    st = ES9843_UpdateBits(ES9843_REG_GPIO910_CFG, 0x0Fu,
                           ES9843_GPIO_FN_STATUS_OR);
    if (st != HAL_OK) return st;

    /*  - GPIO10 = clock-valid, inverted in-chip (high on MCLK loss),
     *    for the external OR gate with GPIO9 onto the PG14 IRQ line    */
    st = ES9843_UpdateBits(ES9843_REG_GPIO910_CFG, 0xF0u,
                           (uint8_t)(ES9843_GPIO_FN_CLOCK_VALID << 4));
    if (st != HAL_OK) return st;
    st = ES9843_UpdateBits(ES9843_REG_GPIO_INV_HI, ES9843_GPIO10_INV,
                           ES9843_GPIO10_INV);
    if (st != HAL_OK) return st;

    /*  - GPIO9+GPIO10 output drivers ON (all GPIOs default TRISTATE:
     *    without this the OR gate watches floating pins)               */
    st = ES9843_UpdateBits(ES9843_REG_GPIO_OE_HI,
                           ES9843_GPIO9_OE | ES9843_GPIO10_OE,
                           ES9843_GPIO9_OE | ES9843_GPIO10_OE);
    if (st != HAL_OK) return st;

    /*  - clear any stale latched flags                                 */
    st = ES9843_ClearOverloadLatches();
    if (st != HAL_OK) return st;

    /* post-init sanity: the TDM encoder must declare the configured
     * output interface valid (retry briefly while clocks settle).
     * TODO bench: confirm typical assertion delay after master enable. */
    for (uint8_t tries = 0u; tries < 10u; tries++)
    {
        uint8_t cv = 0u;
        if (ES9843_ReadReg(ES9843_REG_CODEC_VALIDITY, &cv) == HAL_OK &&
            (cv & ES9843_TDM_ENC_VALID) != 0u)
        {
            return HAL_OK;
        }
        HAL_Delay(5u);
    }
    return HAL_ERROR;   /* interface config not accepted by the chip   */
}

/* ------------------------------------------------------------------ */
/* Product functions                                                   */
/* ------------------------------------------------------------------ */
HAL_StatusTypeDef ES9843_SetSampleRate(es9843_rate_t rate)
{
    return ES9843_WriteReg(ES9843_REG_MASTER_CLK_DIV, (uint8_t)rate);
}

HAL_StatusTypeDef ES9843_SetMuteAll(bool mute)
{
    const uint8_t v = mute ? ES9843_VOLUME_MUTE : ES9843_VOLUME_0DB;
    for (uint8_t ch = 0u; ch < 4u; ch++)
    {
        HAL_StatusTypeDef st = ES9843_WriteReg(
            (uint8_t)(ES9843_REG_VOLUME_CH1 + ch), v);
        if (st != HAL_OK) return st;
    }
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_ChangeSampleRate(es9843_rate_t rate)
{
    HAL_StatusTypeDef st;

    st = ES9843_SetMuteAll(true);
    if (st != HAL_OK) return st;
    HAL_Delay(10u);                 /* let the volume ramp complete     */

    st = ES9843_SetSampleRate(rate);
    if (st != HAL_OK) { (void)ES9843_SetMuteAll(false); return st; }

    /* wait for the clock detector to revalidate the new geometry      */
    bool anomaly = true;
    for (uint8_t tries = 0u; tries < 10u; tries++)
    {
        HAL_Delay(5u);
        if (ES9843_ReadClockStatus(NULL, &anomaly) != HAL_OK) continue;
        if (!anomaly) break;
    }

    HAL_Delay(5u);                  /* decimator settle margin          */
    st = ES9843_SetMuteAll(false);
    if (st != HAL_OK) return st;

    return anomaly ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef ES9843_ReadAutoFsRaw(uint8_t *raw)
{
    return ES9843_ReadReg(ES9843_REG_AUTO_FS_READ, raw);
}

HAL_StatusTypeDef ES9843_ReadClockStatus(uint8_t *raw, bool *anomaly)
{
    uint8_t v;
    HAL_StatusTypeDef st = ES9843_ReadReg(ES9843_REG_CLOCK_VALIDITY, &v);
    if (st != HAL_OK) return st;

    if (raw != NULL) *raw = v;
    if (anomaly != NULL)
    {
        *anomaly = ((v & (ES9843_WS_INVALID | ES9843_BCK_INVALID)) != 0u)
                || ((v & ES9843_RATIO_VALID) == 0u);
    }
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_SetStereoMode(bool on)
{
    /* in-chip channel summing... */
    HAL_StatusTypeDef st = ES9843_UpdateBits(ES9843_REG_FS_CONFIG,
                                             ES9843_STEREO_MODE,
                                             on ? ES9843_STEREO_MODE : 0u);
    if (st != HAL_OK) return st;

    /* ...and the MODE_2CH line to the CPLD follows the same command    */
    return ES9843_SetGpio5Level(on);
}

HAL_StatusTypeDef ES9843_SetGpio5Level(bool high)
{
    HAL_StatusTypeDef st;

    /* function "Output 0" = constant low at the pad...                 */
    st = ES9843_UpdateBits(ES9843_REG_GPIO56_CFG, 0x0Fu,
                           ES9843_GPIO_FN_OUTPUT_0);
    if (st != HAL_OK) return st;

    /* ...level selected by the invert bit: INV=1 -> constant high      */
    st = ES9843_UpdateBits(ES9843_REG_GPIO_INV_LO, ES9843_GPIO5_INV,
                           high ? ES9843_GPIO5_INV : 0u);
    if (st != HAL_OK) return st;

    /* output driver on (defaults to tristate)                          */
    return ES9843_UpdateBits(ES9843_REG_GPIO_OE_LO, ES9843_GPIO5_OE,
                             ES9843_GPIO5_OE);
}

static HAL_StatusTypeDef write16(uint8_t regLo, uint16_t v)
{
    /* multi-byte rule: LSB first, value latches when MSB is written    */
    HAL_StatusTypeDef st = ES9843_WriteReg(regLo, (uint8_t)(v & 0xFFu));
    if (st != HAL_OK) return st;
    return ES9843_WriteReg((uint8_t)(regLo + 1u), (uint8_t)(v >> 8));
}

static HAL_StatusTypeDef read16(uint8_t regLo, uint16_t *v)
{
    uint8_t lo, hi;
    HAL_StatusTypeDef st = ES9843_ReadReg(regLo, &lo);
    if (st != HAL_OK) return st;
    st = ES9843_ReadReg((uint8_t)(regLo + 1u), &hi);
    if (st != HAL_OK) return st;
    *v = (uint16_t)((uint16_t)hi << 8 | lo);
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_SetThdComp(bool pair34, int16_t c2, int16_t c3)
{
    const uint8_t c2lo = pair34 ? ES9843_REG_THD_C2_CH34_LO
                                : ES9843_REG_THD_C2_CH12_LO;
    const uint8_t c3lo = pair34 ? ES9843_REG_THD_C3_CH34_LO
                                : ES9843_REG_THD_C3_CH12_LO;

    HAL_StatusTypeDef st = write16(c2lo, (uint16_t)c2);
    if (st != HAL_OK) return st;
    return write16(c3lo, (uint16_t)c3);
}

HAL_StatusTypeDef ES9843_GetThdComp(bool pair34, int16_t *c2, int16_t *c3)
{
    const uint8_t c2lo = pair34 ? ES9843_REG_THD_C2_CH34_LO
                                : ES9843_REG_THD_C2_CH12_LO;
    const uint8_t c3lo = pair34 ? ES9843_REG_THD_C3_CH34_LO
                                : ES9843_REG_THD_C3_CH12_LO;
    uint16_t v;
    HAL_StatusTypeDef st = read16(c2lo, &v);
    if (st != HAL_OK) return st;
    *c2 = (int16_t)v;
    st = read16(c3lo, &v);
    if (st != HAL_OK) return st;
    *c3 = (int16_t)v;
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_SetFilter(es9843_filter_t f)
{
    return ES9843_UpdateBits(ES9843_REG_ADC_FIR_FILTER,
                             ES9843_FILTER_SHAPE_MASK,
                             (uint8_t)((uint8_t)f << ES9843_FILTER_SHAPE_POS));
}

HAL_StatusTypeDef ES9843_GetFilter(es9843_filter_t *f)
{
    uint8_t raw;
    HAL_StatusTypeDef st = ES9843_ReadReg(ES9843_REG_ADC_FIR_FILTER, &raw);
    if (st != HAL_OK) return st;
    *f = (es9843_filter_t)((raw & ES9843_FILTER_SHAPE_MASK)
                           >> ES9843_FILTER_SHAPE_POS);
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_LoadFirCoeffs(bool stage4x,
                                       const int32_t *coeffs, uint8_t count)
{
    HAL_StatusTypeDef st;

    if (count > 128u) return HAL_ERROR;

    /* open the coefficient RAM for writing                             */
    st = ES9843_UpdateBits(ES9843_REG_DCBLK_HI, ES9843_PROG_FIR_COEFF_WE,
                           ES9843_PROG_FIR_COEFF_WE);
    if (st != HAL_OK) return st;

    for (uint8_t i = 0u; i < count; i++)
    {
        const uint32_t v = (uint32_t)coeffs[i] & 0xFFFFFFu;

        st = ES9843_WriteReg(ES9843_REG_PRAM_ADDR,
                             (uint8_t)((stage4x ? ES9843_PRAM_STAGE_4X : 0u)
                                       | (i & ES9843_PRAM_ADDR_MASK)));
        if (st != HAL_OK) break;

        /* NOTE: LSB-at-lowest-address inferred from the family's other
         * multi-byte registers (e.g. GPIO OE 0x37/0x38). Bench-verify
         * once via the PROG_FIR_COEFF_OUT readback (0xF4 area).         */
        st = ES9843_WriteReg(ES9843_REG_PRAM_DATA0, (uint8_t)(v & 0xFFu));
        if (st != HAL_OK) break;
        st = ES9843_WriteReg(ES9843_REG_PRAM_DATA1, (uint8_t)((v >> 8) & 0xFFu));
        if (st != HAL_OK) break;
        st = ES9843_WriteReg(ES9843_REG_PRAM_DATA2, (uint8_t)((v >> 16) & 0xFFu));
        if (st != HAL_OK) break;
    }

    /* close the RAM write window regardless of outcome                 */
    (void)ES9843_UpdateBits(ES9843_REG_DCBLK_HI, ES9843_PROG_FIR_COEFF_WE, 0u);
    return st;
}

HAL_StatusTypeDef ES9843_EnableCustomFir(bool en)
{
    return ES9843_UpdateBits(ES9843_REG_DCBLK_HI, ES9843_PROG_FIR_COEFF_EN,
                             en ? ES9843_PROG_FIR_COEFF_EN : 0u);
}

HAL_StatusTypeDef ES9843_ReadOverloadLatched(uint8_t *flags)
{
    uint8_t raw;
    HAL_StatusTypeDef st = ES9843_ReadReg(ES9843_REG_OVERLOAD_FLAGS, &raw);
    if (st != HAL_OK) return st;
    *flags = (uint8_t)(raw & 0x0Fu);        /* [3:0] latched CH4..CH1?  */
    /* Datasheet bit order: [0]=LAT_CH1 .. [3]=LAT_CH4 -> bit i = CHi+1 */
    return HAL_OK;
}

HAL_StatusTypeDef ES9843_ClearOverloadLatches(void)
{
    /* Pulse the clear bits: assert then release.
     * TODO: bench-verify whether the release write is required or the
     * bits self-clear after acting.                                    */
    HAL_StatusTypeDef st = ES9843_WriteReg(ES9843_REG_STATUS_CLEAR,
                                           ES9843_CLEAR_OVERLOAD_ALL);
    if (st != HAL_OK) return st;
    return ES9843_WriteReg(ES9843_REG_STATUS_CLEAR, 0x00u);
}

/* ------------------------------------------------------------------ */
/* Overload flag line (EXTI)                                           */
/* ------------------------------------------------------------------ */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == ADC_CLIP_Pin)
    {
        clipFlag = 1u;
    }
    /* future EXTI sources dispatch from this same callback             */
}

bool ES9843_ClipConsume(void)
{
    if (clipFlag)
    {
        clipFlag = 0u;
        return true;
    }
    return false;
}

HAL_StatusTypeDef ES9843_PollErrors(es9843_errors_t *e)
{
    e->overload      = 0u;
    e->clock_anomaly = false;
    e->clock_raw     = 0u;
    e->comms_fail    = false;

    if (ES9843_ReadOverloadLatched(&e->overload) != HAL_OK)
    {
        e->comms_fail = true;
        return HAL_ERROR;
    }

    if (ES9843_ReadClockStatus(&e->clock_raw, &e->clock_anomaly) != HAL_OK)
    {
        e->comms_fail = true;
        return HAL_ERROR;
    }

    /* release the GPIO9 status-OR line by clearing consumed latches   */
    if (e->overload != 0u)
    {
        if (ES9843_ClearOverloadLatches() != HAL_OK)
        {
            e->comms_fail = true;
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

uint32_t ES9843_I2CErrorCount(void)
{
    return i2cErrCount;
}
