/*
 * xmos_link.h — AD-1 CTL v2.0, step 1.2: link to the USB (XMOS XU316) board
 *
 * Two independent signals come from the XMOS board:
 *
 *  XMOS_RDY  PG13, EXTI13 both edges, level: 1 = USB board up with a host
 *            enumerated. Drives the USB icon. Anything the XMOS reports
 *            over I2C is only SHOWN / ACTED ON while this line is high.
 *
 *  I2C4      STM32 = slave, own address 0x42 (7-bit), XMOS = master at
 *            100 kHz (through the ISO1642, clock stretching allowed).
 *            Packet = 2 bytes [XMOS_PKT_MAGIC, code], write only, fire-
 *            and-forget on the XMOS side (a NACK is ignored there).
 *
 *  XMOS_2CH  PG12 output CTRL -> XMOS: 2-channel mode flag (unchanged).
 *
 * Interrupt context does the minimum (2 bytes into a small ring).
 * Xmos_Service(), called once per GUI frame from Model::tick(), decodes
 * the packets, keeps the latches (streaming, last rate request), logs on
 * the debug UART and re-initialises I2C4 if a transfer never completes
 * (XMOS reset in the middle of a packet).
 *
 * Consumers (views, rate_ctrl) poll LEVELS, never events:
 *   Xmos_Ready()        -> USB icon, menu rate lock
 *   Xmos_Streaming()    -> STREAM icon (false whenever XMOS_RDY is low)
 *   Xmos_RateRequest()  -> rate_ctrl applies it while XMOS_RDY is high
 */
#ifndef XMOS_LINK_H
#define XMOS_LINK_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

/* ---- protocol (must match the XMOS firmware ad1_send()) ------------- */
#define XMOS_PKT_MAGIC          0xA5u
#define XMOS_EV_STREAM_START    0x01u
#define XMOS_EV_STREAM_STOP     0x02u
#define XMOS_EV_RATE_BASE       0x10u   /* 0x10 + (rateCode - 1)        */
#define XMOS_EV_RATE_44K1       0x10u
#define XMOS_EV_RATE_48K        0x11u
#define XMOS_EV_RATE_88K2       0x12u
#define XMOS_EV_RATE_96K        0x13u
#define XMOS_EV_RATE_176K4      0x14u
#define XMOS_EV_RATE_192K       0x15u
#define XMOS_EV_RATE_LAST       0x15u

/* GUI frames (~17.5 ms) a transfer may stay open before I2C4 is reset  */
#ifndef XMOS_LINK_STUCK_FRAMES
#define XMOS_LINK_STUCK_FRAMES  6u
#endif

typedef struct
{
    uint32_t rxPackets;     /* complete 2-byte packets received         */
    uint32_t badPackets;    /* wrong magic / unknown code               */
    uint32_t dropped;       /* ring full (service not running?)         */
    uint32_t nacks;         /* AF: master stopped short / NACKed dummy  */
    uint32_t busErrors;     /* BERR / ARLO / OVR                        */
    uint32_t recoveries;    /* I2C4 re-initialised by the watchdog      */
    uint32_t readRequests;  /* master READ (not in the protocol)        */
    uint32_t rdyEdges;      /* XMOS_RDY EXTI edges seen                 */
} xmos_link_stats_t;

/* ---- lifecycle ------------------------------------------------------ */
void Xmos_LinkInit(void);          /* after MX_I2C4_Init(): start listening */
void Xmos_Service(void);           /* once per GUI frame (Model::tick)   */

/* ---- levels for the GUI / rate control ------------------------------ */
bool Xmos_Ready(void);             /* XMOS_RDY pin level                 */
bool Xmos_Streaming(void);         /* stream latch AND Xmos_Ready()      */
/* Last rate request received (any time, even during boot or with RDY
 * low). false = none since power-up. seq increments on every request,
 * so a consumer can tell a repeated request from an old one.           */
bool Xmos_RateRequest(uint8_t *code, uint32_t *seq);
const char *Xmos_RateCodeName(uint8_t code);   /* "48k", "176.4k", ...  */

void Xmos_GetStats(xmos_link_stats_t *out);

/* ---- outputs / IRQ hooks (unchanged from step 1) -------------------- */
void Xmos_Set2ChFlag(bool on);     /* drives XMOS_2CH (PG12)             */
void Xmos_OnReadyEdge(void);       /* from HAL_GPIO_EXTI_Callback        */
bool Xmos_ReadyChanged(bool *nowReady);  /* legacy EXTI consumer (unused) */

#ifdef __cplusplus
}
#endif
#endif
