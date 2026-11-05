/* cpld_ctrl.h — AES/EBU CPLD lines: RST (PC12, active low),
 * MUTE (PC11, active low), LOCK (PD1 EXTI1, high = locked)           */
#ifndef CPLD_CTRL_H
#define CPLD_CTRL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

void CPLD_Reset(bool inReset);          /* true = hold in reset        */
void CPLD_Mute(bool mute);
bool CPLD_Locked(void);                 /* live level                  */
/* Blocks up to timeoutMs waiting for lock. Returns final lock state.  */
bool CPLD_WaitLock(uint32_t timeoutMs);

/* EXTI1 consumer: true once per lock-state change; *nowLocked = level
 * sampled in the ISR at the edge.                                     */
bool CPLD_LockChanged(bool *nowLocked);
/* called from HAL_GPIO_EXTI_Callback — not for application use        */
void CPLD_OnLockEdge(void);

#ifdef __cplusplus
}
#endif
#endif
