#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>

#define UART0_BASE 0x10000000

#define UART_RBR 0x00
#define UART_THR 0x00
#define UART_IER 0x01
#define UART_FCR 0x02
#define UART_LCR 0x03
#define UART_MCR 0x04
#define UART_LSR 0x05
#define UART_MSR 0x06
#define UART_SCR 0x07

void uart_init(void);
void uart_write_char(char c);
void uart_write_string(const char *s);
char uart_read_char(void);
void uart_write_int(unsigned int value);

#endif
