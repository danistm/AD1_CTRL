/*
 * boot_seq.c — AD-1 CTL v2.0 step 1.2 (1.1 rev 2 + RateCtl_Arm at finish)
 *
 * Why the ADC is not touched before ~1 s:
 *  - AVCC comes from an LT3045 with R_SET ~45 k and C_SET >= 10 uF ->
 *    tau = R*C ~ 0.45 s. Even with the LT3045 fast start-up (which stops
 *    near 90 %), the last 10 % settles on that tau: 99 % about 1 s and
 *    99.9 % about 2 s after the rail starts. Without fast start-up the
 *    whole ramp runs on tau (99 % at ~2.1 s).
 *  - The ES9843 sequence (datasheet Fig. 26) wants supplies stable
 *    before CHIP_EN, and CHIP_EN also gates MCLK (LMK OE), which the
 *    chip's I2C needs. So CHIP_EN is asserted late and the presence
 *    check POLLS until the chip answers, instead of a one-shot read.
 *  - The audio path stays muted until BOOT_SETTLE_MS regardless, which
 *    also covers the ES9843 DC-blocker settling after enable.
 */
#include "boot_seq.h"
#include "stm32h7xx_hal.h"
#include "debug_uart.h"
#include "settings_store.h"
#include "adc_es9843.h"
#include "cpld_ctrl.h"
#include "xmos_link.h"
#include "osc_ctrl.h"
#include "sai_metering.h"
#include "rate_ctrl.h"

/* absolute times, ms after Boot_Begin()                                */
#define T_ADC_EN           1000u   /* assert CHIP_EN (rails >~97 %)     */
#define T_ADC_DEADLINE     2000u   /* chip must answer by here          */
#define ADC_POLL_MS         100u
/* relative times after the ADC is up                                   */
#define D_SAI_START         100u   /* after CPLD reset release          */
#define D_CPLD_LOCK_MAX     800u   /* lock deadline after release       */
#define D_I2S_CHECK         600u   /* after SAI start                   */
#define D_FINISH_MIN        100u   /* after the last check              */
#define T_FINISH_MIN       BOOT_SETTLE_MS

typedef enum
{
    ST_IDLE, ST_WAIT_ADC_EN, ST_WAIT_ADC, ST_OSC, ST_CPLD_REL, ST_WAIT_SAI,
    ST_WAIT_LOCK, ST_WAIT_I2S, ST_WAIT_FINISH, ST_DONE, ST_FAILED
} step_t;

static step_t       step   = ST_IDLE;
static uint32_t     t0     = 0u;
static const char  *errTxt = "";
static bool         adcOk  = false;
static uint8_t      failCount = 0u;
static uint32_t     tNextPoll = 0u, tRelease = 0u, tSaiStart = 0u, tFinish = 0u;
static uint32_t     saiActivityAtStart = 0u;

static inline uint32_t elapsed(void) { return HAL_GetTick() - t0; }

/* Returns true if the caller must stop (fatal build). In the
 * development build it only logs, counts, and continues with 'next'. */
static bool fail(const char *msg, step_t next)
{
    failCount++;
    DBG_Printf("BOOT %s: %s (t=%lu ms)\r\n",
               BOOT_CHECKS_NONFATAL ? "WARN" : "FAIL",
               msg, (unsigned long)elapsed());
#if BOOT_CHECKS_NONFATAL
    step = next;
    return false;
#else
    (void)next;
    errTxt = msg;
    step   = ST_FAILED;
    CPLD_Mute(true);                    /* safe state                  */
    (void)ES9843_SetMuteAll(true);
    return true;
#endif
}

static void schedule_finish(void)
{
    const uint32_t e = elapsed() + D_FINISH_MIN;
    tFinish = (e > T_FINISH_MIN) ? e : T_FINISH_MIN;
    step = ST_WAIT_FINISH;
}

void Boot_Begin(void)
{
    t0 = HAL_GetTick();
    step = ST_WAIT_ADC_EN;
    errTxt = ""; adcOk = false; failCount = 0u;
    DBG_Printf("boot sequence start (checks %s)\r\n",
               BOOT_CHECKS_NONFATAL ? "NON-FATAL / dev" : "fatal");
}

uint8_t Boot_ProgressPct(void)
{
    if (step == ST_IDLE || step == ST_FAILED) return 0u;
    const uint32_t e = elapsed();
    if (e >= BOOT_SETTLE_MS) return 100u;
    return (uint8_t)((e * 100u) / BOOT_SETTLE_MS);
}

const char *Boot_ErrorText(void) { return errTxt; }
bool        Boot_AdcOk(void)     { return adcOk; }
uint8_t     Boot_FailCount(void) { return failCount; }

static void adc_configure(void)
{
    static const es9843_rate_t rateMap[3] =
        { ES9843_RATE_48K, ES9843_RATE_96K, ES9843_RATE_192K };

    (void)ES9843_SetMuteAll(true);             /* muted during splash  */
    if (g_settings.rateIdx != 0u)
    {
        (void)ES9843_ChangeSampleRate(rateMap[g_settings.rateIdx]);
        (void)ES9843_SetMuteAll(true);         /* ChangeSampleRate unmutes */
    }
    (void)ES9843_SetStereoMode(g_settings.ch2On);   /* ADC + CPLD + XMOS */
    (void)ES9843_SetFilter((es9843_filter_t)g_settings.filtIdx);
}

boot_state_t Boot_Tick(void)
{
    switch (step)
    {
    case ST_WAIT_ADC_EN:
        if (elapsed() < T_ADC_EN) break;
        ES9843_SetEnabled(true);               /* CHIP_EN -> MCLK on   */
        tNextPoll = elapsed() + 20u;           /* > t_data_out         */
        DBG_Printf("ADC enabled at %lu ms\r\n", (unsigned long)elapsed());
        step = ST_WAIT_ADC;
        break;

    case ST_WAIT_ADC:
        if (elapsed() < tNextPoll) break;
        {
            uint8_t id = 0u;
            if (ES9843_ReadReg(ES9843_REG_CHIP_ID, &id) == HAL_OK &&
                id == ES9843_CHIP_ID_VALUE)
            {
                DBG_Printf("ADC answered at %lu ms\r\n", (unsigned long)elapsed());
                if (ES9843_Init() != HAL_OK)
                { fail("ADC INIT FAILED", ST_OSC); break; }
                adc_configure();

                bool anomaly = true;
                if (ES9843_ReadClockStatus(NULL, &anomaly) != HAL_OK || anomaly)
                { fail("ADC CLOCK INVALID", ST_OSC); break; }

                adcOk = true;
                DBG_Printf("ES9843 init OK, rate idx %u\r\n", g_settings.rateIdx);
                step = ST_OSC;
            }
            else if (elapsed() >= T_ADC_DEADLINE)
            {
                fail("ADC NOT RESPONDING", ST_OSC);
            }
            else
            {
                tNextPoll = elapsed() + ADC_POLL_MS;
            }
        }
        break;

    case ST_OSC:
        {
            bool pca, dac;
            Osc_Probe(&pca, &dac);
            DBG_Printf("OSC bus: PCA9536 %s, DAC8571 %s\r\n",
                       pca ? "ACK" : "--", dac ? "ACK" : "--");
            if (!pca) { if (fail("OSC EXPANDER NO ACK", ST_CPLD_REL)) break; }
            if (!dac) { if (fail("OSC DAC NO ACK",      ST_CPLD_REL)) break; }
            step = ST_CPLD_REL;
        }
        break;

    case ST_CPLD_REL:
        CPLD_Reset(false);                     /* ADC clocks are valid */
        tRelease = elapsed();
        DBG_Printf("CPLD reset released at %lu ms\r\n", (unsigned long)tRelease);
        step = ST_WAIT_SAI;
        break;

    case ST_WAIT_SAI:
        if (elapsed() < tRelease + D_SAI_START) break;
        saiActivityAtStart = SAI_Metering_ActivityCount();
        SAI_Metering_Start();
        tSaiStart = elapsed();
        DBG_Printf("SAI metering started\r\n");
        step = ST_WAIT_LOCK;
        break;

    case ST_WAIT_LOCK:
        if (CPLD_Locked())
        {
            DBG_Printf("CPLD lock OK (t=%lu ms)\r\n", (unsigned long)elapsed());
            step = ST_WAIT_I2S;
        }
        else if (elapsed() >= tRelease + D_CPLD_LOCK_MAX)
        {
            (void)fail("CPLD NOT LOCKED", ST_WAIT_I2S);
        }
        break;

    case ST_WAIT_I2S:
        if (elapsed() < tSaiStart + D_I2S_CHECK) break;
        if (SAI_Metering_ActivityCount() == saiActivityAtStart)
        { if (fail("NO I2S INPUT", ST_WAIT_FINISH)) break; schedule_finish(); break; }
        DBG_Printf("I2S input alive\r\n");
        schedule_finish();
        break;

    case ST_WAIT_FINISH:
        if (elapsed() < tFinish) break;
        (void)ES9843_SetMuteAll(false);        /* release the audio path */
        CPLD_Mute(false);
        DBG_Printf("USB host: %s\r\n", Xmos_Ready() ? "connected" : "absent");
        RateCtl_Arm();        /* step 1.2: XMOS may now set the rate   */
#if BOOT_CHECKS_NONFATAL
        if (failCount)
            DBG_Printf("*** DEV BUILD: %u boot check(s) failed, continuing ***\r\n",
                       failCount);
#endif
        DBG_Printf("boot done at %lu ms\r\n", (unsigned long)elapsed());
        step = ST_DONE;
        break;

    default:
        break;
    }

    if (step == ST_DONE)   return BOOT_DONE;
    if (step == ST_FAILED) return BOOT_FAILED;
    return BOOT_RUNNING;
}
