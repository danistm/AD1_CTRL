/*
 * rate_ctrl.h — AD-1 CTL v2.0, step 1.2: sample-rate arbiter
 *
 * One place that knows what rate the ADC runs at and who may change it:
 *
 *   XMOS_RDY high : the USB host owns the rate. XMOS rate requests are
 *                   applied; the menu item is locked (dimmed); a menu
 *                   EXIT only STORES the user's choice.
 *   XMOS_RDY low  : the user owns it. The stored menu setting
 *                   (g_settings.rateIdx) is applied; XMOS requests that
 *                   arrive with RDY low are held and applied when RDY
 *                   rises.
 *
 * g_settings.rateIdx keeps its meaning of "the stand-alone (AES/EBU
 * only) rate" and is never written by the XMOS path.
 *
 * Step 1.2 scope: 48 kHz family only (idx 0/1/2 = 48/96/192). The 44.1
 * family requests (0x10/0x12/0x14) are logged and ignored until step 2
 * adds the MCLK family switch (PCA9536) — this file is where that goes.
 *
 * RateCtl_Service() runs every GUI frame from Model::tick(); it is inert
 * until RateCtl_Arm() is called by the boot sequencer after the ADC is
 * initialised and the audio path is open.
 */
#ifndef RATE_CTRL_H
#define RATE_CTRL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#define RATE_IDX_COUNT   3u      /* 48 / 96 / 192 (step 2: 6)           */

/* When the USB host disappears (XMOS_RDY falls), go back to the user's
 * stored rate (1) or keep running at the last host rate (0).          */
#ifndef RATE_REVERT_ON_USB_GONE
#define RATE_REVERT_ON_USB_GONE  1
#endif

void     RateCtl_Init(void);       /* main(), after Settings_Load()     */
void     RateCtl_Arm(void);        /* boot_seq, at ST_WAIT_FINISH       */
void     RateCtl_Service(void);    /* Model::tick()                     */

/* Menu EXIT: apply g_settings.rateIdx if the user owns the rate.
 * Returns true if applied (or already running), false if deferred
 * because the USB host owns the rate (setting is still stored).       */
bool     RateCtl_ApplyUser(void);

uint8_t  RateCtl_LiveIdx(void);    /* what the ADC runs now (0..2)      */
uint16_t RateCtl_LiveKhz(void);
uint16_t RateCtl_IdxToKhz(uint8_t idx);
bool     RateCtl_XmosOwns(void);   /* == Xmos_Ready()                   */

#ifdef __cplusplus
}
#endif
#endif
