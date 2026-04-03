#include "uart.h"

static volatile uint8_t* uart = (volatile uint8_t*)UART0_BASE;
static int uart_initialized = 0;

void uart_init(void) {
    if (uart_initialized) return;
    uart[UART_LCR] = 0x80;
    uart[0x00] = 1;
    uart[0x01] = 0;
    uart[UART_LCR] = 0x03;
    uart[UART_FCR] = 0x07;
    uart[UART_IER] = 0x01;
    uart_initialized = 1;
}

void uart_write_char(char c) {
    while (!(uart[UART_LSR] & 0x20)) __asm__ volatile("nop");
    uart[UART_THR] = c;
}

void uart_write_string(const char *s) {
    while (*s) uart_write_char(*s++);
}

char uart_read_char(void) {
    while (!(uart[UART_LSR] & 0x01)) __asm__ volatile("nop");
    return uart[UART_RBR];
}
