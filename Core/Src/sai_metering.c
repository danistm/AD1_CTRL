/*
 * sai_metering.c — AD-1 CTL
 *
 * Location: Core/Src (with sai_metering.h in Core/Inc)
 *
 * PREREQUISITES (see integration notes):
 *  - buffers live in AHB SRAM @0x30000000 (linker section .sai_buffers,
 *    non-cacheable MPU region) -> zero cache maintenance needed
 *  - AHBSRAM1 clock enabled before first touch (done in Start())
 *  - MX_DMA_Init() must run BEFORE MX_SAI1_Init() in main()
 */
#include "sai_metering.h"
#include "sai.h"
#include <math.h>
#include <string.h>

/* 256 frames/half: at 192 kHz a half-buffer is 1.33 ms; at 48 kHz 5.3 ms.
 * Multiple halves per UI frame are max-held into peakAbs[].             */
#define FRAMES_PER_HALF   256u
#define WORDS_PER_HALF    (FRAMES_PER_HALF * 2u)      /* 2 slots/frame  */
#define BUF_WORDS         (WORDS_PER_HALF * 2u)       /* double buffer  */

#define FULL_SCALE_ABS    8388608.0f                  /* 2^23           */
#define SILENCE_DBFS      (-90.0f)

static int32_t bufA[BUF_WORDS] __attribute__((section(".sai_buffers"), aligned(32)));
static int32_t bufB[BUF_WORDS] __attribute__((section(".sai_buffers"), aligned(32)));

static volatile uint32_t peakAbs[4];      /* max |sample| since last read */
static volatile uint32_t errCount;
static volatile uint32_t activity;        /* increments on every DMA half   */
static volatile uint8_t  restartPending;  /* set by error callback          */

/* ---- per-half processing (IRQ context, keep tight) ---- */
static void process(const int32_t *p, uint32_t frames, int chBase)
{
    uint32_t m0 = 0u, m1 = 0u;
    for (uint32_t i = 0u; i < frames; i++)
    {
        /* 24-bit data right-aligned in the 32-bit slot: sign-extend     */
        int32_t s0 = (int32_t)((uint32_t)p[2u*i]      << 8) >> 8;
        int32_t s1 = (int32_t)((uint32_t)p[2u*i + 1u] << 8) >> 8;
        uint32_t a0 = (s0 < 0) ? (uint32_t)(-s0) : (uint32_t)s0;
        uint32_t a1 = (s1 < 0) ? (uint32_t)(-s1) : (uint32_t)s1;
        if (a0 > m0) m0 = a0;
        if (a1 > m1) m1 = a1;
    }
    if (m0 > peakAbs[chBase])      peakAbs[chBase]      = m0;
    if (m1 > peakAbs[chBase + 1])  peakAbs[chBase + 1]  = m1;
}

/* ---- HAL callbacks (shared across SAI handles, dispatch by instance) */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if      (hsai == &hsai_BlockA1) { process(&bufA[0],             FRAMES_PER_HALF, 0); activity++; }
    else if (hsai == &hsai_BlockB1) { process(&bufB[0],             FRAMES_PER_HALF, 2); activity++; }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if      (hsai == &hsai_BlockA1) { process(&bufA[WORDS_PER_HALF], FRAMES_PER_HALF, 0); activity++; }
    else if (hsai == &hsai_BlockB1) { process(&bufB[WORDS_PER_HALF], FRAMES_PER_HALF, 2); activity++; }
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    /* Frame-sync errors (rate change, clock glitch) make the HAL abort
     * the DMA asynchronously. Never restart from here: flag it and let
     * SAI_Metering_Service() do a clean stop/start in thread context.   */
    (void)hsai;
    errCount++;
    restartPending = 1u;
}

/* ---- public API ---- */
void SAI_Metering_Start(void)
{
    /* AHB SRAM1 @0x30000000 is clock-gated on the H7A3: enable before
     * the first CPU or DMA access. (Unlike AXI SRAM, which is always on.) */
#ifdef __HAL_RCC_AHBSRAM1_CLK_ENABLE
    __HAL_RCC_AHBSRAM1_CLK_ENABLE();
#else
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_AHBSRAM1EN);
#endif

    memset((void*)bufA, 0, sizeof(bufA));
    memset((void*)bufB, 0, sizeof(bufB));
    for (int i = 0; i < 4; i++) peakAbs[i] = 0u;

    /* slave RX: transfers begin when external BCLK/FS arrive            */
    HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)bufA, BUF_WORDS);
    HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*)bufB, BUF_WORDS);
}

static void restart_capture(void)
{
    /* DMAStop is safe in READY or BUSY; brings both blocks to READY     */
    (void)HAL_SAI_DMAStop(&hsai_BlockA1);
    (void)HAL_SAI_DMAStop(&hsai_BlockB1);

    /* Wipe stale audio and pending peaks: after a clock loss, noise on
     * floating inputs can complete a DMA half over a buffer that still
     * holds old samples — which would re-inject the last level.         */
    memset((void*)bufA, 0, sizeof(bufA));
    memset((void*)bufB, 0, sizeof(bufB));
    __disable_irq();
    for (int i = 0; i < 4; i++) peakAbs[i] = 0u;
    __enable_irq();

    (void)HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)bufA, BUF_WORDS);
    (void)HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*)bufB, BUF_WORDS);
}

void SAI_Metering_Service(void)
{
    static uint32_t lastActivity = 0u;
    static uint16_t stallFrames  = 0u;

    if (restartPending)
    {
        restartPending = 0u;
        stallFrames    = 0u;
        restart_capture();
        return;
    }

    /* Stall watchdog: clocks removed mid-buffer raise no error at all.
     * ~30 UI frames (~0.5 s) without a DMA half -> re-arm reception.
     * Harmless when idle: re-arming a clockless slave just waits.       */
    if (activity == lastActivity)
    {
        if (++stallFrames >= 30u)
        {
            stallFrames = 0u;
            restart_capture();
        }
    }
    else
    {
        lastActivity = activity;
        stallFrames  = 0u;
    }
}

void SAI_Metering_GetPeaksDb(float dbfs[4])
{
    uint32_t snap[4];

    __disable_irq();                       /* a few cycles: snapshot+reset */
    for (int i = 0; i < 4; i++) { snap[i] = peakAbs[i]; peakAbs[i] = 0u; }
    __enable_irq();

    for (int i = 0; i < 4; i++)
    {
        dbfs[i] = (snap[i] != 0u)
                ? 20.0f * log10f((float)snap[i] / FULL_SCALE_ABS)
                : SILENCE_DBFS;
    }
}

uint32_t SAI_Metering_ErrorCount(void)
{
    return errCount;
}
