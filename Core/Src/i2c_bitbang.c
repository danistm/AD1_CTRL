/*
 * i2c_bitbang.c — AD-1 CTL v2.0 software I2C master
 *
 * Compiled at -O2 regardless of the project level: at -O0 every pin
 * helper and DWT read is a real call, which added ~2 us per bit and
 * made a "400 kHz" bus run at ~217 kHz (bench, 2026-11-04).
 */
#pragma GCC optimize ("O2")
#include "i2c_bitbang.h"

/* ------------------------------------------------------------------ */
/* cycle-accurate delays (DWT)                                         */
/* ------------------------------------------------------------------ */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR    = 0xC5ACCE55u;           /* Cortex-M7: unlock DWT regs */
    DWT->CYCCNT = 0u;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_cyc(uint32_t cyc)
{
    const uint32_t t0 = DWT->CYCCNT;
    while ((DWT->CYCCNT - t0) < cyc) { __NOP(); }
}

static inline uint32_t ns_to_cyc(uint32_t ns)
{
    /* SystemCoreClock / 1e9 * ns, without overflow for ns < 20 us     */
    return (uint32_t)(((uint64_t)SystemCoreClock * ns + 999999999ull) / 1000000000ull);
}

/* ------------------------------------------------------------------ */
/* pin helpers — open-drain: "high" = release, "low" = drive           */
/* ------------------------------------------------------------------ */
static inline void scl_hi(I2CBB_Bus *b) { b->cfg.sclPort->BSRR = b->cfg.sclPin; }
static inline void scl_lo(I2CBB_Bus *b) { b->cfg.sclPort->BSRR = (uint32_t)b->cfg.sclPin << 16; }
static inline void sda_hi(I2CBB_Bus *b) { b->cfg.sdaPort->BSRR = b->cfg.sdaPin; }
static inline void sda_lo(I2CBB_Bus *b) { b->cfg.sdaPort->BSRR = (uint32_t)b->cfg.sdaPin << 16; }
static inline bool scl_rd(I2CBB_Bus *b) { return (b->cfg.sclPort->IDR & b->cfg.sclPin) != 0u; }
static inline bool sda_rd(I2CBB_Bus *b) { return (b->cfg.sdaPort->IDR & b->cfg.sdaPin) != 0u; }

/* release SCL and wait until the line really is high (pull-up rise,
 * isolator propagation, or a stretching slave). false on timeout.     */
static bool scl_release_wait(I2CBB_Bus *b)
{
    scl_hi(b);
    const uint32_t t0 = DWT->CYCCNT;
    while (!scl_rd(b))
    {
        if ((DWT->CYCCNT - t0) > b->sclTimeout) return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* protocol primitives                                                 */
/* ------------------------------------------------------------------ */
static bool i2c_start(I2CBB_Bus *b)
{
    /* (repeated) START: SDA high, SCL high, then SDA falls while SCL high */
    sda_hi(b);
    delay_cyc(b->tSu);
    if (!scl_release_wait(b)) return false;
    delay_cyc(b->tSta);                  /* tSU;STA                     */
    sda_lo(b);
    delay_cyc(b->tSta);                  /* tHD;STA                     */
    scl_lo(b);
    delay_cyc(b->tHd);
    return true;
}

static void i2c_stop(I2CBB_Bus *b)
{
    sda_lo(b);
    delay_cyc(b->tHd);
    (void)scl_release_wait(b);
    delay_cyc(b->tSta);                  /* tSU;STO                     */
    sda_hi(b);
    delay_cyc(b->tBuf);                  /* tBUF before next START      */
}

/* returns false on SCL timeout                                        */
/* Per bit, SCL low time = tHd (spent at the end of the previous bit)
 * + tSu; SCL high time = tHigh after the line is seen high.           */
static bool i2c_write_bit(I2CBB_Bus *b, bool bit)
{
    if (bit) sda_hi(b); else sda_lo(b);
    delay_cyc(b->tSu);                   /* rest of tLOW = data setup   */
    if (!scl_release_wait(b)) return false;
    delay_cyc(b->tHigh);
    scl_lo(b);
    delay_cyc(b->tHd);                   /* data hold after SCL fall    */
    return true;
}

static bool i2c_read_bit(I2CBB_Bus *b, bool *bit)
{
    sda_hi(b);                           /* release for the slave       */
    delay_cyc(b->tSu);
    if (!scl_release_wait(b)) return false;
    delay_cyc(b->tHigh);
    *bit = sda_rd(b);                    /* sample while SCL high       */
    scl_lo(b);
    delay_cyc(b->tHd);
    return true;
}

/* write a byte, return: 1 = ACK, 0 = NACK, -1 = bus timeout           */
static int i2c_write_byte(I2CBB_Bus *b, uint8_t v)
{
    for (int i = 7; i >= 0; i--)
    {
        if (!i2c_write_bit(b, (v >> i) & 1u)) return -1;
    }
    bool nack;
    if (!i2c_read_bit(b, &nack)) return -1;
    return nack ? 0 : 1;
}

static bool i2c_read_byte(I2CBB_Bus *b, uint8_t *v, bool ack)
{
    uint8_t r = 0u;
    for (int i = 0; i < 8; i++)
    {
        bool bit;
        if (!i2c_read_bit(b, &bit)) return false;
        r = (uint8_t)((r << 1) | (bit ? 1u : 0u));
    }
    *v = r;
    return i2c_write_bit(b, !ack);       /* ACK = drive low             */
}

/* ------------------------------------------------------------------ */
/* public API                                                          */
/* ------------------------------------------------------------------ */
void I2CBB_Init(I2CBB_Bus *bus, const I2CBB_Config *cfg)
{
    bus->cfg      = *cfg;
    bus->errCount = 0u;

    dwt_init();

    /* Bit period budget: 56 % low (>= 1.3 us at 400k), 32 % high
     * (>= 0.6 us), the remaining 12 % absorbs SCL rise + code overhead.
     * Low = tHd + tSu. Measured at -O2 on the H7 @160 MHz: ~390 kHz.   */
    const uint32_t periodNs = 1000000u / (cfg->speedKHz ? cfg->speedKHz : 100u);
    const uint32_t lowNs    = periodNs * 56u / 100u;      /* 1400 @400k */
    bus->tHd   = ns_to_cyc(300u);                         /* data hold  */
    bus->tSu   = ns_to_cyc(lowNs - 300u);                 /* 1100 @400k */
    bus->tHigh = ns_to_cyc(periodNs * 32u / 100u);        /*  800 @400k */
    bus->tLow  = ns_to_cyc(lowNs);                        /* recovery   */
    bus->tSta  = ns_to_cyc(700u);        /* tSU;STA, tHD;STA, tSU;STO   */
    bus->tBuf  = ns_to_cyc(1300u);
    bus->sclTimeout = ns_to_cyc(2000000u);         /* 2 ms stretch max  */

    /* both lines released, then configure as open-drain outputs       */
    scl_hi(bus);
    sda_hi(bus);

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;               /* external pull-ups on board  */
    g.Speed = GPIO_SPEED_FREQ_MEDIUM;
    g.Pin   = cfg->sclPin; HAL_GPIO_Init(cfg->sclPort, &g);
    g.Pin   = cfg->sdaPin; HAL_GPIO_Init(cfg->sdaPort, &g);

    delay_cyc(bus->tBuf);
    if (!sda_rd(bus))
    {
        (void)I2CBB_BusRecover(bus);
    }
}

bool I2CBB_BusRecover(I2CBB_Bus *b)
{
    sda_hi(b);
    for (int i = 0; i < 9 && !sda_rd(b); i++)
    {
        scl_lo(b);
        delay_cyc(b->tLow);
        (void)scl_release_wait(b);
        delay_cyc(b->tHigh);
    }
    i2c_stop(b);
    return sda_rd(b);
}

static HAL_StatusTypeDef fail(I2CBB_Bus *b, HAL_StatusTypeDef st)
{
    b->errCount++;
    i2c_stop(b);                         /* best effort: free the bus  */
    return st;
}

HAL_StatusTypeDef I2CBB_IsDeviceReady(I2CBB_Bus *b, uint16_t addr8, uint32_t trials)
{
    if (trials == 0u) trials = 1u;
    while (trials--)
    {
        if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);
        const int r = i2c_write_byte(b, (uint8_t)(addr8 & 0xFEu));
        i2c_stop(b);
        if (r < 0)  { b->errCount++; return HAL_TIMEOUT; }
        if (r == 1) return HAL_OK;
    }
    return HAL_ERROR;                    /* no ACK                      */
}

HAL_StatusTypeDef I2CBB_MasterTransmit(I2CBB_Bus *b, uint16_t addr8,
                                       const uint8_t *data, uint16_t len)
{
    if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);
    int r = i2c_write_byte(b, (uint8_t)(addr8 & 0xFEu));
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    for (uint16_t i = 0u; i < len; i++)
    {
        r = i2c_write_byte(b, data[i]);
        if (r < 0)  return fail(b, HAL_TIMEOUT);
        if (r == 0) return fail(b, HAL_ERROR);
    }
    i2c_stop(b);
    return HAL_OK;
}

HAL_StatusTypeDef I2CBB_MasterReceive(I2CBB_Bus *b, uint16_t addr8,
                                      uint8_t *data, uint16_t len)
{
    if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);
    int r = i2c_write_byte(b, (uint8_t)(addr8 | 0x01u));
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    for (uint16_t i = 0u; i < len; i++)
    {
        if (!i2c_read_byte(b, &data[i], (i + 1u) < len))
            return fail(b, HAL_TIMEOUT);
    }
    i2c_stop(b);
    return HAL_OK;
}

HAL_StatusTypeDef I2CBB_MemWrite(I2CBB_Bus *b, uint16_t addr8, uint8_t reg,
                                 const uint8_t *data, uint16_t len)
{
    if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);
    int r = i2c_write_byte(b, (uint8_t)(addr8 & 0xFEu));
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    r = i2c_write_byte(b, reg);
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    for (uint16_t i = 0u; i < len; i++)
    {
        r = i2c_write_byte(b, data[i]);
        if (r < 0)  return fail(b, HAL_TIMEOUT);
        if (r == 0) return fail(b, HAL_ERROR);
    }
    i2c_stop(b);
    return HAL_OK;
}

HAL_StatusTypeDef I2CBB_MemRead(I2CBB_Bus *b, uint16_t addr8, uint8_t reg,
                                uint8_t *data, uint16_t len)
{
    if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);
    int r = i2c_write_byte(b, (uint8_t)(addr8 & 0xFEu));
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    r = i2c_write_byte(b, reg);
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);

    if (!i2c_start(b)) return fail(b, HAL_TIMEOUT);      /* repeated START */
    r = i2c_write_byte(b, (uint8_t)(addr8 | 0x01u));
    if (r < 0)  return fail(b, HAL_TIMEOUT);
    if (r == 0) return fail(b, HAL_ERROR);
    for (uint16_t i = 0u; i < len; i++)
    {
        if (!i2c_read_byte(b, &data[i], (i + 1u) < len))
            return fail(b, HAL_TIMEOUT);
    }
    i2c_stop(b);
    return HAL_OK;
}
