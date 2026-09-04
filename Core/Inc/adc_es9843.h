/*
 * adc_es9843.h — AD-1 CTL: ES9843PRO ADC access module
 * Register map transcribed from ES9843PRO datasheet v0.4.1.
 *
 * Owns: I2C register access (I2C2 @ PF0=SDA / PF1=SCL), CHIP_EN (PD1),
 *       overload/clip flag input (PG14, EXTI).
 */
#ifndef ADC_ES9843_H
#define ADC_ES9843_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Device address — Table 3: 8-bit address by ADDR2/ADDR1 straps:      */
/*   GND/GND = 0x40, GND/AVDD = 0x42, AVDD/GND = 0x44, AVDD/AVDD=0x46  */
/* TODO: confirm the strap on the AD-1 schematic.                      */
/* ------------------------------------------------------------------ */
#define ES9843_I2C_ADDR8            (0x40u)

/* ------------------------------------------------------------------ */
/* Register map (v0.4.1)                                               */
/* ------------------------------------------------------------------ */
#define ES9843_REG_SYS_CONFIG       (0x00u)
#define   ES9843_SOFT_RESET           (0x80u)  /* write 0xA0 for reset  */
#define   ES9843_EN_MCLK_IN           (0x20u)
#define   ES9843_SEL_MCLK_ACLK2       (0x10u)
#define   ES9843_EN_ADC_CH4           (0x08u)
#define   ES9843_EN_ADC_CH3           (0x04u)
#define   ES9843_EN_ADC_CH2           (0x02u)
#define   ES9843_EN_ADC_CH1           (0x01u)

#define ES9843_REG_FS_CONFIG        (0x01u)
#define   ES9843_STEREO_MODE          (0x20u)  /* CH1+CH3, CH2+CH4 sum  */
#define   ES9843_MONO_MODE            (0x10u)
#define   ES9843_AUTO_FS_DETECT       (0x01u)  /* default 1             */

#define ES9843_REG_BACKEND_CLK      (0x05u)
#define   ES9843_MCLK_24M_DIV2        (0x10u)
#define ES9843_REG_MASTER_CLK_DIV   (0x06u)  /* BCK = MCLK/(value+1)    */
#define ES9843_REG_MISC_CLK_CONFIG  (0x07u)
#define   ES9843_EN_WS_MONITOR        (0x08u)  /* default 1             */
#define   ES9843_EN_BCK_MONITOR       (0x04u)  /* default 1             */
#define   ES9843_EN_CLK_DET           (0x01u)  /* default 0: enable!    */
#define ES9843_REG_MASTER_MODE      (0x08u)
#define   ES9843_SLAVE_BCK_INVERT     (0x80u)
#define   ES9843_MASTER_WS_INVERT     (0x08u)
#define   ES9843_MASTER_BCK_INVERT    (0x04u)  /* default 1             */
#define   ES9843_DSD_MASTER_EN        (0x02u)
#define   ES9843_PCM_MASTER_EN        (0x01u)

#define ES9843_REG_STATUS_MASK      (0x20u)  /* 1 = bit contributes to OR */
#define   ES9843_MASK_OVERLOAD_CH4    (0x80u)
#define   ES9843_MASK_OVERLOAD_CH3    (0x40u)
#define   ES9843_MASK_OVERLOAD_CH2    (0x20u)
#define   ES9843_MASK_OVERLOAD_CH1    (0x10u)
#define   ES9843_MASK_OVERLOAD_ALL    (0xF0u)
#define   ES9843_MASK_PEAKDET_ALL     (0x0Fu)

#define ES9843_REG_STATUS_CLEAR     (0x21u)  /* 1 clears latched bit    */
#define   ES9843_CLEAR_OVERLOAD_ALL   (0xF0u)
#define   ES9843_CLEAR_PEAKDET_ALL    (0x0Fu)

/* GPIO config registers: two 4-bit function fields per register       */
#define ES9843_REG_GPIO12_CFG       (0x2Fu)
#define ES9843_REG_GPIO34_CFG       (0x30u)
#define ES9843_REG_GPIO56_CFG       (0x31u)  /* [3:0]=GPIO5, [7:4]=GPIO6 */
#define ES9843_REG_GPIO78_CFG       (0x32u)
#define ES9843_REG_GPIO910_CFG      (0x33u)  /* [3:0]=GPIO9, [7:4]=GPIO10*/
#define ES9843_REG_GPIO11_CFG       (0x34u)

/* GPIO function codes (4-bit)                                         */
#define   ES9843_GPIO_FN_SHUTDOWN     (0x0u)
#define   ES9843_GPIO_FN_OUTPUT_0     (0x1u)  /* drives constant low    */
#define   ES9843_GPIO_FN_STATUS_OR    (0x2u)  /* OR of masked status    */
#define   ES9843_GPIO_FN_CLOCK_VALID  (0x3u)
#define   ES9843_GPIO_FN_PWM1         (0x5u)
#define   ES9843_GPIO_FN_CH_DET(ch)   (0xCu + (ch))   /* ch = 0..3      */

/* GPIO output enable (16-bit across 0x37/0x38; ALL default tristate)  */
#define ES9843_REG_GPIO_OE_LO       (0x37u)  /* [7:0] = GPIO8..GPIO1_OE */
#define   ES9843_GPIO5_OE             (0x10u)  /* 0x37 bit 4            */
#define ES9843_REG_GPIO_OE_HI       (0x38u)  /* [2:0] = GPIO11..9_OE    */
#define   ES9843_GPIO9_OE             (0x01u)  /* 0x38 bit 0            */
#define   ES9843_GPIO10_OE            (0x02u)  /* 0x38 bit 1            */

/* GPIO invert (16-bit across 0x39/0x3A): inverts input and output     */
#define ES9843_REG_GPIO_INV_LO      (0x39u)  /* [7:0] = GPIO8..GPIO1    */
#define   ES9843_GPIO5_INV            (0x10u)  /* 0x39 bit 4            */
#define ES9843_REG_GPIO_INV_HI      (0x3Au)  /* [2:0] = GPIO11..9       */
#define   ES9843_GPIO10_INV           (0x02u)  /* 0x3A bit 1            */

/* DC blocking / programmable-FIR control (16-bit pair 0x4B/0x4C)      */
#define ES9843_REG_DCBLK_LO         (0x4Bu)  /* DC-block coeff [7:0]    */
#define ES9843_REG_DCBLK_HI         (0x4Cu)
#define   ES9843_DC_BLOCK_EN_ALL      (0xF0u)  /* [7:4] CH4..CH1, dflt 1 */
#define   ES9843_PROG_FIR_COEFF_WE   (0x08u)  /* pair bit 11             */
#define   ES9843_PROG_FIR_COEFF_EN   (0x04u)  /* pair bit 10             */

/* Programmable FIR coefficient RAM access                             */
#define ES9843_REG_PRAM_ADDR        (0x4Du)
#define   ES9843_PRAM_STAGE_4X        (0x80u)  /* 0 = 2x stage (default) */
#define   ES9843_PRAM_ADDR_MASK       (0x7Fu)  /* coefficient 0..127     */
#define ES9843_REG_PRAM_DATA0       (0x4Eu)  /* coeff [7:0]  (see note) */
#define ES9843_REG_PRAM_DATA1       (0x4Fu)  /* coeff [15:8]            */
#define ES9843_REG_PRAM_DATA2       (0x50u)  /* coeff [23:16]           */

/* Volume / mute (software-mode mute = VOLUME CHx at 0xFF, soft-ramped) */
#define ES9843_REG_VOLUME_CH1       (0x51u)  /* ..CH4 at 0x52/53/54     */
#define   ES9843_VOLUME_0DB           (0x00u)
#define   ES9843_VOLUME_MUTE          (0xFFu)

/* THD compensation: out = x + C2*x^2 + C3*x^3, signed 16-bit coeffs,
 * per channel PAIR. Multi-byte rule: write LSB first, latches on MSB. */
#define ES9843_REG_THD_C2_CH12_LO   (0x5Au)
#define ES9843_REG_THD_C2_CH12_HI   (0x5Bu)
#define ES9843_REG_THD_C3_CH12_LO   (0x5Cu)
#define ES9843_REG_THD_C3_CH12_HI   (0x5Du)
#define ES9843_REG_THD_C2_CH34_LO   (0x5Eu)
#define ES9843_REG_THD_C2_CH34_HI   (0x5Fu)
#define ES9843_REG_THD_C3_CH34_LO   (0x60u)
#define ES9843_REG_THD_C3_CH34_HI   (0x61u)

/* Decimation filter (register 74)                                     */
#define ES9843_REG_ADC_FIR_FILTER   (0x4Au)
#define   ES9843_FILTER_SHAPE_MASK    (0xE0u)  /* [7:5]                 */
#define   ES9843_FILTER_SHAPE_POS     (5u)

typedef enum
{
    ES9843_FILT_MIN_PHASE            = 0,  /* default                   */
    ES9843_FILT_LIN_FAST_APODIZING   = 1,
    ES9843_FILT_LIN_FAST             = 2,
    ES9843_FILT_LIN_FAST_LOW_RIPPLE  = 3,
    ES9843_FILT_LIN_SLOW             = 4,
    ES9843_FILT_MIN_FAST             = 5,
    ES9843_FILT_MIN_SLOW             = 6,
    ES9843_FILT_MIN_SLOW_LOW_DISP    = 7
} es9843_filter_t;

/* Read-only registers (0xE0-0xFF)                                     */
#define ES9843_REG_CODEC_VALIDITY   (0xE0u)
#define   ES9843_TDM_DEC_VALID        (0x40u)
#define   ES9843_TDM_ENC_VALID        (0x20u)
#define ES9843_REG_AUTO_FS_READ     (0xE6u)  /* FS-detect result X/Y/Z  */
#define ES9843_REG_CLOCK_VALIDITY   (0xE7u)
#define   ES9843_RATIO_VALID          (0x04u)
#define   ES9843_BCK_INVALID          (0x02u)
#define   ES9843_WS_INVALID           (0x01u)
#define ES9843_REG_CHIP_ID          (0xE1u)
#define   ES9843_CHIP_ID_VALUE        (0x8Fu)
#define ES9843_REG_OVERLOAD_FLAGS   (0xEAu)
#define   ES9843_OVL_LIVE_CH(ch)      (0x10u << (ch))  /* [7:4] live    */
#define   ES9843_OVL_LATCH_CH(ch)     (0x01u << (ch))  /* [3:0] latched */

/* ------------------------------------------------------------------ */
/* Register access                                                     */
/* ------------------------------------------------------------------ */
HAL_StatusTypeDef ES9843_WriteReg  (uint8_t reg, uint8_t val);
HAL_StatusTypeDef ES9843_ReadReg   (uint8_t reg, uint8_t *val);
HAL_StatusTypeDef ES9843_UpdateBits(uint8_t reg, uint8_t mask, uint8_t val);

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */
/* CHIP_EN high -> t_data_out wait -> soft reset -> chip-ID probe ->
 * base config (all 4 channels on, overload OR routed to GPIO9).       */
HAL_StatusTypeDef ES9843_Init(void);
void ES9843_SetEnabled(bool en);            /* raw CHIP_EN control     */

/* ------------------------------------------------------------------ */
/* Product functions                                                   */
/* ------------------------------------------------------------------ */
typedef enum
{
    ES9843_RATE_48K  = 7,   /* MASTER_DCLK_DIV values: BCK=MCLK/(v+1)  */
    ES9843_RATE_96K  = 3,   /* 24.576 MHz -> 64fs BCK per rate         */
    ES9843_RATE_192K = 1
} es9843_rate_t;

/* Master-mode sample rate: reprograms the BCK/WS generation divider.
 * AUTO_FS_DETECT (default on) makes the decimator follow the new WS.
 * Raw divider write only — no muting. See ES9843_ChangeSampleRate().  */
HAL_StatusTypeDef ES9843_SetSampleRate(es9843_rate_t rate);

/* Pop-free rate change: ramped mute -> divider -> wait clock-valid ->
 * ramped unmute. Blocking (tens of ms); datasheet mandates no reset,
 * the mute only suppresses the decimator re-lock transient on the
 * live outputs. Returns HAL_ERROR if clocks don't revalidate.         */
HAL_StatusTypeDef ES9843_ChangeSampleRate(es9843_rate_t rate);

/* Ramped mute/unmute of all four channels (volume = 0 dB on unmute).  */
HAL_StatusTypeDef ES9843_SetMuteAll(bool mute);

/* Raw FS-detect result (0xE6): [5:0]=X, [6]=Y half-div, [7]=Z 64fs.
 * FS = Y*MCLK/((X+1)*...) per datasheet — use as rate-change
 * confirmation telemetry (compare against a known-good capture).      */
HAL_StatusTypeDef ES9843_ReadAutoFsRaw(uint8_t *raw);

/* Clock validity snapshot (register 0xE7). anomaly = true if WS or BCK
 * is invalid or the MCLK/WS ratio is not valid. Call from the CTL-side
 * stall/error path to classify clock faults for the error display.    */
HAL_StatusTypeDef ES9843_ReadClockStatus(uint8_t *raw, bool *anomaly);

/* 4->2 parallel mode: STEREO_MODE sums CH1+CH3 / CH2+CH4 in the chip,
 * and drives GPIO5 (MODE_2CH to the CPLD/relays) to match: on = high. */
HAL_StatusTypeDef ES9843_SetStereoMode(bool on);

/* Static level on ES9843 GPIO5 (MODE_2CH line to the CPLD).
 * Function "Output 0" + GPIO5_INV: INV=0 -> low, INV=1 -> high.       */
HAL_StatusTypeDef ES9843_SetGpio5Level(bool high);

/* THD compensation tuning (service menu). pair34: false = CH1/2,
 * true = CH3/4. c2/c3: signed 16-bit correction coefficients.         */
HAL_StatusTypeDef ES9843_SetThdComp(bool pair34, int16_t c2, int16_t c3);
HAL_StatusTypeDef ES9843_GetThdComp(bool pair34, int16_t *c2, int16_t *c3);

/* Select the 8x-decimation FIR filter shape (sonic evaluation).       */
HAL_StatusTypeDef ES9843_SetFilter(es9843_filter_t f);
HAL_StatusTypeDef ES9843_GetFilter(es9843_filter_t *f);

/* Load custom oversampling-filter coefficients into on-chip RAM.
 * stage4x: false = 2x stage, true = 4x stage. coeffs: 24-bit signed
 * values in int32 (bits above 23 ignored), count <= 128. Volatile RAM:
 * reload after every power cycle / soft reset. Does NOT activate them —
 * call ES9843_EnableCustomFir(true) afterwards.                        */
HAL_StatusTypeDef ES9843_LoadFirCoeffs(bool stage4x,
                                       const int32_t *coeffs, uint8_t count);

/* Select coefficient source: false = built-in FILTER_SHAPE (default),
 * true = the programmed custom coefficients.                           */
HAL_StatusTypeDef ES9843_EnableCustomFir(bool en);

/* Per-channel latched modulator-overload flags: bit0=CH1 .. bit3=CH4. */
HAL_StatusTypeDef ES9843_ReadOverloadLatched(uint8_t *flags);
/* Clear all latched overload flags (pulse of STATUS BITS CLEAR).      */
HAL_StatusTypeDef ES9843_ClearOverloadLatches(void);

/* ------------------------------------------------------------------ */
/* Unified error poll                                                  */
/* ------------------------------------------------------------------ */
typedef struct
{
    uint8_t overload;       /* latched modulator overload, bit0..3=CH1..4 */
    bool    clock_anomaly;  /* WS/BCK invalid or MCLK ratio not valid     */
    uint8_t clock_raw;      /* raw CLOCK VALIDITY register (0xE7)         */
    bool    comms_fail;     /* I2C did not answer (treat as ADC error)    */
} es9843_errors_t;

/* Reads overload (0xEA) and clock validity (0xE7), fills *e, and if
 * any overload latch was set clears the latches — which releases the
 * GPIO9 status-OR line (the IRQ line resets by calling this).
 *
 * IRQ-line design (external OR gate on the ADC board):
 *   PG14 = GPIO9 ("OR of Status Bits" = overload)  OR
 *          GPIO10 ("Clock valid", inverted in-chip = high on MCLK loss)
 * Fully event-driven, no periodic polling. On the PG14 EXTI, call this
 * and classify:
 *   - overload bits set            -> clip latches (per channel)
 *   - clock_anomaly                -> clock fault, MCLK still alive
 *   - comms_fail (I2C timeout)     -> hard clock/power fault: the I2C
 *     slave is MCLK-synchronous (fSCL < CLK/20), so a dead MCLK takes
 *     register access down with it — the timeout IS the diagnosis.
 * The CTL-side SAI stall watchdog remains as an independent backstop. */
HAL_StatusTypeDef ES9843_PollErrors(es9843_errors_t *e);

/* ------------------------------------------------------------------ */
/* Overload flag line (PG14 EXTI, OR of masked status bits via GPIO9)  */
/* ------------------------------------------------------------------ */
bool ES9843_ClipConsume(void);              /* read-and-clear          */

/* Diagnostics                                                         */
uint32_t ES9843_I2CErrorCount(void);

#ifdef __cplusplus
}
#endif
#endif
