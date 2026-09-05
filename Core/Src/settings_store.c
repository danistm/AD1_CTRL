/*
 * settings_store.c — AD-1 CTL
 */
#include "settings_store.h"
#include "eeprom_24aa02.h"
#include <string.h>

MenuSettings g_settings;               /* the live instance             */

#define BLOCK_MAGIC0   ('A')
#define BLOCK_MAGIC1   ('D')
#define BLOCK_VERSION  (1u)

typedef struct __attribute__((packed))
{
    uint8_t  magic[2];
    uint8_t  version;
    uint8_t  rateIdx;
    uint8_t  ch2On;
    uint8_t  keyBright;
    uint8_t  dispBright;
    uint8_t  filtIdx;
    uint8_t  reserved[6];
    uint16_t crc;                      /* CRC-16/CCITT over bytes 0..13 */
} SettingsBlock;                       /* 16 bytes = 2 EEPROM pages     */

static SettingsBlock lastWritten;      /* for skip-if-unchanged         */
static bool          lastValid;

static uint16_t crc16_ccitt(const uint8_t *p, uint16_t n)
{
    uint16_t crc = 0xFFFFu;
    while (n--)
    {
        crc ^= (uint16_t)(*p++) << 8;
        for (uint8_t i = 0u; i < 8u; i++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
    }
    return crc;
}

/* Factory defaults: applied on first power-up / blank or corrupt EEPROM */
void Settings_Defaults(MenuSettings *s)
{
    s->rateIdx    = 0u;     /* 48 kHz                                   */
    s->ch2On      = false;  /* 4-channel                                */
    s->keyBright  = 5u;
    s->dispBright = 5u;
    s->filtIdx    = 2u;     /* linear phase fast roll-off: the industry-
                               standard ADC decimation filter          */
}

static void clamp(MenuSettings *s)
{
    if (s->rateIdx    > 2u)  s->rateIdx    = 0u;
    if (s->filtIdx    > 7u)  s->filtIdx    = 0u;
    if (s->keyBright  > 10u) s->keyBright  = 10u;
    if (s->dispBright > 10u) s->dispBright = 10u;
    /* display never dark: table level 0 == MIN, so <=10 is sufficient  */
}

static void pack(SettingsBlock *b, const MenuSettings *s)
{
    memset(b, 0, sizeof(*b));
    b->magic[0]   = BLOCK_MAGIC0;
    b->magic[1]   = BLOCK_MAGIC1;
    b->version    = BLOCK_VERSION;
    b->rateIdx    = s->rateIdx;
    b->ch2On      = s->ch2On ? 1u : 0u;
    b->keyBright  = s->keyBright;
    b->dispBright = s->dispBright;
    b->filtIdx    = s->filtIdx;
    b->crc        = crc16_ccitt((const uint8_t*)b, sizeof(*b) - 2u);
}

static bool unpack(const SettingsBlock *b, MenuSettings *s)
{
    if (b->magic[0] != BLOCK_MAGIC0 || b->magic[1] != BLOCK_MAGIC1) return false;
    if (b->version != BLOCK_VERSION) return false;
    if (b->crc != crc16_ccitt((const uint8_t*)b, sizeof(*b) - 2u)) return false;
    s->rateIdx    = b->rateIdx;
    s->ch2On      = (b->ch2On != 0u);
    s->keyBright  = b->keyBright;
    s->dispBright = b->dispBright;
    s->filtIdx    = b->filtIdx;
    clamp(s);
    return true;
}

bool Settings_Load(void)
{
    SettingsBlock b;
    bool ok = (EEPROM_Read(SETTINGS_EEPROM_ADDR, (uint8_t*)&b, sizeof(b))
               == HAL_OK) && unpack(&b, &g_settings);

    if (ok)
    {
        lastWritten = b;
        lastValid   = true;
        return true;
    }

    /* missing / corrupt / no EEPROM: defaults, and try to seed the block */
    Settings_Defaults(&g_settings);
    lastValid = false;
    (void)Settings_Save();
    return false;
}

bool Settings_Save(void)
{
    SettingsBlock b;
    clamp(&g_settings);
    pack(&b, &g_settings);

    if (lastValid && memcmp(&b, &lastWritten, sizeof(b)) == 0)
        return true;                   /* unchanged: no wear, no wait   */

    if (EEPROM_Write(SETTINGS_EEPROM_ADDR, (const uint8_t*)&b, sizeof(b))
        != HAL_OK)
        return false;

    /* read-back verify                                                 */
    SettingsBlock v;
    if (EEPROM_Read(SETTINGS_EEPROM_ADDR, (uint8_t*)&v, sizeof(v)) != HAL_OK)
        return false;
    if (memcmp(&v, &b, sizeof(b)) != 0) return false;

    lastWritten = b;
    lastValid   = true;
    return true;
}
