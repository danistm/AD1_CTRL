/* debug_uart.h — USART2 console (PD5 TX / PD6 RX, 115200 8N1) */
#ifndef DEBUG_UART_H
#define DEBUG_UART_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void DebugUart_Init(void);                 /* retargets printf        */
void DBG_Printf(const char *fmt, ...);     /* blocking, short lines   */
#ifdef __cplusplus
}
#endif
#endif
