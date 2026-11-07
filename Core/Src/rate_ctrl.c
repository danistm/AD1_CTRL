/* rate_ctrl.c — AD-1 CTL v2.0, step 1.2 (see rate_ctrl.h) */
#include "rate_ctrl.h"
#include "xmos_link.h"
#include "adc_es9843.h"
#include "menu_settings.h"
#include "debug_uart.h"

static const es9843_rate_t rateMap[RATE_IDX_COUNT] =
    { ES9843_RATE_48K, ES9843_RATE_96K, ES9843_RATE_192K };
static const uint16_t rateKhz[RATE_IDX_COUNT] = { 48u, 96u, 192u };

static uint8_t  liveIdx;      /* rate the ADC is running at            */
static bool     armed;        /* false until the boot sequence is done */
static bool     lastRdy;
static uint32_t lastSeq;      /* last XMOS request sequence handled    */

static bool apply_idx(uint8_t idx, const char *who)
{
    if (idx >= RATE_IDX_COUNT) return false;
    if (idx == liveIdx)
    {
        DBG_Printf("rate: %u kHz unchanged (%s)\r\n", rateKhz[idx], who);
        return true;
    }
    if (ES9843_ChangeSampleRate(rateMap[idx]) != HAL_OK)
    {
        DBG_Printf("rate: %u kHz FAILED (%s)\r\n", rateKhz[idx], who);
        return false;
    }
    liveIdx = idx;
    DBG_Printf("rate: %u kHz applied (%s)\r\n", rateKhz[idx], who);
    return true;
}

static bool apply_code(uint8_t code, const char *who)
{
    switch (code)
    {
    case XMOS_EV_RATE_48K:   return apply_idx(0u, who);
    case XMOS_EV_RATE_96K:   return apply_idx(1u, who);
    case XMOS_EV_RATE_192K:  return apply_idx(2u, who);
    case XMOS_EV_RATE_44K1:
    case XMOS_EV_RATE_88K2:
    case XMOS_EV_RATE_176K4:
        DBG_Printf("rate: %s requested (%s) - 44.1 family needs the MCLK "
                   "switch (step 2), staying at %u kHz\r\n",
                   Xmos_RateCodeName(code), who, rateKhz[liveIdx]);
        return false;
    default:
        return false;
    }
}

void RateCtl_Init(void)
{
    liveIdx = (g_settings.rateIdx < RATE_IDX_COUNT) ? g_settings.rateIdx : 0u;
    armed   = false;
    lastSeq = 0u;
    lastRdy = Xmos_Ready();
}

void RateCtl_Arm(void)
{
    uint8_t  code;
    uint32_t seq;

    armed   = true;
    lastRdy = Xmos_Ready();
    /* a request that arrived during the splash is applied now */
    if (Xmos_RateRequest(&code, &seq))
    {
        lastSeq = seq;
        if (lastRdy) (void)apply_code(code, "XMOS, received during boot");
        else DBG_Printf("rate: %s requested during boot, held (RDY low)\r\n",
                        Xmos_RateCodeName(code));
    }
    DBG_Printf("rate control armed: %u kHz, owner %s\r\n",
               rateKhz[liveIdx], lastRdy ? "USB host" : "user");
}

void RateCtl_Service(void)
{
    if (!armed) return;

    const bool rdy = Xmos_Ready();
    uint8_t  code = 0u;
    uint32_t seq  = 0u;
    const bool have = Xmos_RateRequest(&code, &seq);

    if (rdy != lastRdy)
    {
        lastRdy = rdy;
        if (rdy)
        {
            /* host (re)appeared: honour the latest request we hold     */
            if (have)
            {
                lastSeq = seq;
                (void)apply_code(code, "XMOS, host ready");
            }
        }
        else
        {
#if RATE_REVERT_ON_USB_GONE
            (void)apply_idx(g_settings.rateIdx, "USB host gone, user setting");
#else
            DBG_Printf("rate: USB host gone, keeping %u kHz\r\n", rateKhz[liveIdx]);
#endif
        }
    }

    if (have && seq != lastSeq)
    {
        lastSeq = seq;
        if (rdy) (void)apply_code(code, "XMOS");
        else DBG_Printf("rate: %s requested while XMOS_RDY low - held\r\n",
                        Xmos_RateCodeName(code));
    }
}

bool RateCtl_ApplyUser(void)
{
    if (Xmos_Ready())
    {
        DBG_Printf("rate: user setting %u kHz stored, USB host owns the rate\r\n",
                   RateCtl_IdxToKhz(g_settings.rateIdx));
        return false;
    }
    return apply_idx(g_settings.rateIdx, "menu");
}

uint8_t  RateCtl_LiveIdx(void)  { return liveIdx; }
uint16_t RateCtl_LiveKhz(void)  { return rateKhz[liveIdx]; }
uint16_t RateCtl_IdxToKhz(uint8_t idx)
{
    return (idx < RATE_IDX_COUNT) ? rateKhz[idx] : 0u;
}
bool     RateCtl_XmosOwns(void) { return Xmos_Ready(); }
