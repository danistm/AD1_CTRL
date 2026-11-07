/*
 * boot_seq.h — AD-1 CTL: boot sequencer driven from the splash screen
 *
 * main() does only what must precede the panel lighting (console,
 * settings, backlight, CPLD held in reset + muted, I2C buses up).
 * Everything that takes time or can fail runs here, one step per GUI
 * frame, so the splash progress bar keeps moving and a failure can be
 * shown on the glass. Steps are time-scheduled across the settle
 * window (BOOT_SETTLE_MS); the bar is purely time-based.
 *
 * Fatal checks (freeze on the splash with a red message):
 *   ADC not responding / init failed / clock invalid
 *   oscillator expander (PCA9536) no ACK
 *   oscillator DAC (DAC8571) no ACK
 *   CPLD not locked within the window
 *   no I2S reaching the CTRL (SAI DMA never advanced)
 * Non-fatal (UART log only): EEPROM absent -> defaults applied.
 * NOT a check at all: XMOS/USB host presence — AES/EBU-only operation
 * with USB unplugged is a normal condition; the main screen's USB icon
 * just follows the XMOS_RDY line.
 */
#ifndef BOOT_SEQ_H
#define BOOT_SEQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define BOOT_SETTLE_MS     (3000u)

/* DEVELOPMENT SWITCH. 1 = failed checks are logged on the UART but do
 * not freeze the splash: the sequence runs to the end, unmutes, and the
 * main screen comes up regardless (e.g. CTRL board alone on the bench,
 * no ADC/CPLD attached). Set to 0 for the product build.              */
#ifndef BOOT_CHECKS_NONFATAL
#define BOOT_CHECKS_NONFATAL   1
#endif

typedef enum
{
    BOOT_RUNNING = 0,
    BOOT_DONE,          /* all checks passed, ADC + CPLD unmuted        */
    BOOT_FAILED         /* frozen; Boot_ErrorText() valid               */
} boot_state_t;

void         Boot_Begin(void);         /* call once when the splash shows */
boot_state_t Boot_Tick(void);          /* call once per GUI frame         */
uint8_t      Boot_ProgressPct(void);   /* 0..100, time-based              */
const char  *Boot_ErrorText(void);     /* uppercase, <= 20 chars          */
bool         Boot_AdcOk(void);         /* for the main screen's status    */
uint8_t      Boot_FailCount(void);     /* checks that failed (nonfatal) */

#ifdef __cplusplus
}
#endif
#endif
