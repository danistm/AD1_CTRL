/* xmos_link.h — XMOS_RDY (PG13 EXTI13, high = USB host ready) and
 * XMOS_2CH (PG12 output). I2C4 slave link is step 2+.                 */
#ifndef XMOS_LINK_H
#define XMOS_LINK_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
bool Xmos_Ready(void);                     /* live level              */
bool Xmos_ReadyChanged(bool *nowReady);    /* EXTI consumer           */
void Xmos_Set2ChFlag(bool on);             /* drives PG12             */
void Xmos_OnReadyEdge(void);               /* from EXTI callback      */
#ifdef __cplusplus
}
#endif
#endif
