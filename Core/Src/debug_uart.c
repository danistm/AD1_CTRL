/* debug_uart.c — AD-1 CTL: printf -> USART2 */
#include "debug_uart.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void DebugUart_Init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);      /* no buffering: lines appear now */
}

/* newlib retarget */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, 100u);
    return len;
}

void DBG_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100u);
}
