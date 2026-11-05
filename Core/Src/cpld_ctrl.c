/* cpld_ctrl.c — AD-1 CTL v2.0 */
#include "cpld_ctrl.h"
#include "main.h"

static volatile uint8_t lockEvent;
static volatile uint8_t lockLevelAtEdge;

void CPLD_Reset(bool inReset)
{
    /* active low: in reset = pin LOW */
    HAL_GPIO_WritePin(CPLD_RST_GPIO_Port, CPLD_RST_Pin,
                      inReset ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void CPLD_Mute(bool mute)
{
    /* active low: muted = pin LOW */
    HAL_GPIO_WritePin(CPLD_MUTE_GPIO_Port, CPLD_MUTE_Pin,
                      mute ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool CPLD_Locked(void)
{
    return HAL_GPIO_ReadPin(CPLD_LOCK_GPIO_Port, CPLD_LOCK_Pin) == GPIO_PIN_SET;
}

bool CPLD_WaitLock(uint32_t timeoutMs)
{
    const uint32_t t0 = HAL_GetTick();
    while (!CPLD_Locked())
    {
        if ((HAL_GetTick() - t0) >= timeoutMs) break;
        HAL_Delay(1u);
    }
    return CPLD_Locked();
}

void CPLD_OnLockEdge(void)
{
    lockLevelAtEdge = CPLD_Locked() ? 1u : 0u;
    lockEvent       = 1u;
}

bool CPLD_LockChanged(bool *nowLocked)
{
    if (!lockEvent) return false;
    lockEvent = 0u;
    if (nowLocked) *nowLocked = (lockLevelAtEdge != 0u);
    return true;
}
