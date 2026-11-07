/*
 * xmos_link.c — AD-1 CTL v2.0, step 1.2
 *
 * I2C4 slave receiver for the XMOS packets + XMOS_RDY bookkeeping.
 *
 * HAL slave flow (interrupt context, all in this file):
 *   HAL_I2C_EnableListen_IT            arm, wait for our address
 *   HAL_I2C_AddrCallback               addressed: master writes -> queue a
 *                                      2-byte sequential receive
 *   HAL_I2C_SlaveRxCpltCallback        2 bytes in -> push to the ring
 *   HAL_I2C_ListenCpltCallback         STOP seen -> re-arm listen
 *   HAL_I2C_ErrorCallback              AF (benign) or bus error -> count,
 *                                      re-arm if the HAL fell back to READY
 *
 * These five HAL callbacks are defined HERE for every I2C instance: the
 * other buses (I2C1 EEPROM, I2C2 ES9843) only use blocking HAL calls,
 * which never invoke them, and I2C3 is bit-banged. Do not define them
 * in a second file.
 */
#include "xmos_link.h"
#include "main.h"
#include "debug_uart.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c4;

/* ---- ISR -> thread ring of raw packets ------------------------------ */
#define PKT_RING   8u
static volatile uint8_t  ring[PKT_RING][2];
static volatile uint8_t  ringHead;              /* written by the ISR   */
static volatile uint8_t  ringTail;              /* read by the service  */
static uint8_t           rxBuf[2];              /* HAL target buffer    */
static uint8_t           txDummy = 0xFFu;       /* for a master READ    */

static volatile xmos_link_stats_t st;

/* ---- decoded state (service / thread context only) ------------------ */
static bool     streaming;
static uint8_t  lastRateCode;                   /* 0 = none yet         */
static uint32_t rateSeq;
static bool     lastRdy;
static uint8_t  busyFrames;

/* ---- XMOS_RDY (EXTI) ------------------------------------------------- */
static volatile uint8_t rdyEvent, rdyLevel;

bool Xmos_Ready(void)
{
    return HAL_GPIO_ReadPin(XMOS_RDY_GPIO_Port, XMOS_RDY_Pin) == GPIO_PIN_SET;
}

void Xmos_OnReadyEdge(void)
{
    rdyLevel = Xmos_Ready() ? 1u : 0u;
    rdyEvent = 1u;
    st.rdyEdges++;
}

bool Xmos_ReadyChanged(bool *nowReady)
{
    if (!rdyEvent) return false;
    rdyEvent = 0u;
    if (nowReady) *nowReady = (rdyLevel != 0u);
    return true;
}

void Xmos_Set2ChFlag(bool on)
{
    HAL_GPIO_WritePin(XMOS_2CH_GPIO_Port, XMOS_2CH_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ---- I2C4 slave ------------------------------------------------------ */
static void arm_listen(void)
{
    if (hi2c4.State == HAL_I2C_STATE_READY)
    {
        (void)HAL_I2C_EnableListen_IT(&hi2c4);
    }
}

static void link_reinit(void)
{
    /* MspDeInit/MspInit (CubeMX) take the clock, pins and NVIC with them;
     * the Init structure stays in the handle, so no MX_ call is needed. */
    (void)HAL_I2C_DeInit(&hi2c4);
    (void)HAL_I2C_Init(&hi2c4);
    (void)HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE);
    (void)HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0u);
    arm_listen();
    st.recoveries++;
}

void Xmos_LinkInit(void)
{
    memset((void *)&st, 0, sizeof st);
    ringHead = ringTail = 0u;
    streaming    = false;
    lastRateCode = 0u;
    rateSeq      = 0u;
    busyFrames   = 0u;
    lastRdy      = Xmos_Ready();

    arm_listen();

    DBG_Printf("XMOS link: I2C4 slave addr 0x%02X (7-bit), %s, XMOS_RDY=%u\r\n",
               (unsigned)(hi2c4.Init.OwnAddress1 >> 1),
               (hi2c4.State == HAL_I2C_STATE_LISTEN) ? "listening" : "NOT LISTENING",
               lastRdy ? 1u : 0u);
}

const char *Xmos_RateCodeName(uint8_t code)
{
    static const char *const n[6] =
        { "44.1k", "48k", "88.2k", "96k", "176.4k", "192k" };
    if (code >= XMOS_EV_RATE_BASE && code <= XMOS_EV_RATE_LAST)
        return n[code - XMOS_EV_RATE_BASE];
    return "?";
}

bool Xmos_Streaming(void)
{
    return streaming && Xmos_Ready();
}

bool Xmos_RateRequest(uint8_t *code, uint32_t *seq)
{
    if (lastRateCode == 0u) return false;
    if (code) *code = lastRateCode;
    if (seq)  *seq  = rateSeq;
    return true;
}

void Xmos_GetStats(xmos_link_stats_t *out)
{
    if (out) memcpy(out, (const void *)&st, sizeof *out);
}

void Xmos_Service(void)
{
    /* 1) XMOS_RDY level: the stream latch dies with the host ---------- */
    const bool rdy = Xmos_Ready();
    if (rdy != lastRdy)
    {
        lastRdy = rdy;
        if (!rdy) streaming = false;
        DBG_Printf("XMOS_RDY -> %u%s\r\n", rdy ? 1u : 0u,
                   rdy ? "" : " (stream latch cleared)");
    }

    /* 2) decode what the ISR queued ----------------------------------- */
    while (ringTail != ringHead)
    {
        const uint8_t b0 = ring[ringTail][0];
        const uint8_t b1 = ring[ringTail][1];
        ringTail = (uint8_t)((ringTail + 1u) % PKT_RING);

        if (b0 != XMOS_PKT_MAGIC)
        {
            st.badPackets++;
            DBG_Printf("XMOS rx: bad magic %02X %02X\r\n", b0, b1);
            continue;
        }
        switch (b1)
        {
        case XMOS_EV_STREAM_START:
            streaming = true;
            DBG_Printf("XMOS: stream ON%s\r\n", rdy ? "" : " (RDY low: not shown)");
            break;
        case XMOS_EV_STREAM_STOP:
            streaming = false;
            DBG_Printf("XMOS: stream OFF\r\n");
            break;
        default:
            if (b1 >= XMOS_EV_RATE_BASE && b1 <= XMOS_EV_RATE_LAST)
            {
                lastRateCode = b1;
                rateSeq++;
                DBG_Printf("XMOS: rate request %s (#%lu)\r\n",
                           Xmos_RateCodeName(b1), (unsigned long)rateSeq);
            }
            else
            {
                st.badPackets++;
                DBG_Printf("XMOS rx: unknown code %02X\r\n", b1);
            }
            break;
        }
    }

    /* 3) peripheral watchdog: addressed, but the packet never ended ---- */
    const HAL_I2C_StateTypeDef s = hi2c4.State;
    if (s == HAL_I2C_STATE_BUSY_RX_LISTEN || s == HAL_I2C_STATE_BUSY_TX_LISTEN)
    {
        if (++busyFrames >= XMOS_LINK_STUCK_FRAMES)
        {
            busyFrames = 0u;
            link_reinit();
            DBG_Printf("XMOS link: transfer stuck, I2C4 re-initialised (#%lu)\r\n",
                       (unsigned long)st.recoveries);
        }
    }
    else
    {
        busyFrames = 0u;
        arm_listen();           /* no-op unless the HAL dropped to READY */
    }
}

/* ======================= HAL I2C callbacks ============================ */

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection,
                          uint16_t AddrMatchCode)
{
    (void)AddrMatchCode;
    if (hi2c->Instance != I2C4) return;

    if (TransferDirection == I2C_DIRECTION_TRANSMIT)
    {
        /* master transmits -> we receive the 2-byte packet */
        (void)HAL_I2C_Slave_Seq_Receive_IT(hi2c, rxBuf, 2u, I2C_FIRST_AND_LAST_FRAME);
    }
    else
    {
        /* master READ is not part of the protocol: answer one dummy byte
         * so the bus is released cleanly (the master's final NACK lands
         * in HAL_I2C_ErrorCallback as AF, counted as 'nacks')          */
        st.readRequests++;
        (void)HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &txDummy, 1u, I2C_FIRST_AND_LAST_FRAME);
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C4) return;
    const uint8_t next = (uint8_t)((ringHead + 1u) % PKT_RING);
    if (next == ringTail)
    {
        st.dropped++;
        return;
    }
    ring[ringHead][0] = rxBuf[0];
    ring[ringHead][1] = rxBuf[1];
    ringHead = next;
    st.rxPackets++;
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    (void)hi2c;
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C4) return;
    arm_listen();               /* STOP seen: wait for the next packet  */
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C4) return;
    const uint32_t e = hi2c->ErrorCode;
    hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    if (e & HAL_I2C_ERROR_AF)  st.nacks++;      /* short write / read NACK */
    if (e & ~HAL_I2C_ERROR_AF) st.busErrors++;  /* BERR, ARLO, OVR, ...    */
    arm_listen();               /* HAL keeps LISTEN on AF; re-arm otherwise */
}
