#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>

#define UART0_BASE 0x10000000

#define UART_RBR 0x00  /* Receiver Buffer Register (read only) */
#define UART_THR 0x00  /* Transmitter Holding Register (write only) */
#define UART_IER 0x01  /* Interrupt Enable Register */
#define UART_FCR 0x02  /* FIFO Control Register */
#define UART_LCR 0x03  /* Line Control Register */
#define UART_MCR 0x04  /* Modem Control Register */
#define UART_LSR 0x05  /* Line Status Register */
#define UART_MSR 0x06  /* Modem Status Register */
#define UART_SCR 0x07  /* Scratch Register */

void uart_init(void);
void uart_write_char(char c);
void uart_write_string(const char *s);
void uart_write_int(uint32_t value);
char uart_read_char(void);

#endif
