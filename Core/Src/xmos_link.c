/* xmos_link.c — AD-1 CTL v2.0 */
#include "xmos_link.h"
#include "main.h"

static volatile uint8_t rdyEvent;
static volatile uint8_t rdyLevel;

bool Xmos_Ready(void)
{
    return HAL_GPIO_ReadPin(XMOS_RDY_GPIO_Port, XMOS_RDY_Pin) == GPIO_PIN_SET;
}

void Xmos_OnReadyEdge(void)
{
    rdyLevel = Xmos_Ready() ? 1u : 0u;
    rdyEvent = 1u;
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
