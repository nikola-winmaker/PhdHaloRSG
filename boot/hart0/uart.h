/**
 * Simple UART0 Driver Header
 */

#ifndef UART_H
#define UART_H

#include <stdarg.h>
#include <stdint.h>

/**
 * Write a single character to UART0
 */
void uart_write_char(char c);

/**
 * Write a string to UART0
 */
void uart_write_string(const char *str);

/**
 * Read a single character from UART0 (blocking)
 */
char uart_read_char(void);

/**
 * Read a single character from UART0 (non-blocking)
 * Returns: character if available, -1 if no data
 */
int uart_read_char_nonblock(void);

/**
 * Write a 32-bit unsigned integer in decimal
 */
void uart_write_uint32(uint32_t val);

/**
 * Write a 32-bit unsigned integer in hexadecimal
 */
void uart_write_hex32(uint32_t val);

/**
 * Write a 64-bit unsigned integer in hexadecimal
 */
void uart_write_uint64(uint64_t val);

/**
 * Printf-like function for UART0 - disabled due to relocation issues
 * Define as no-op to avoid linker errors
 */
#define uart_printf(fmt, ...) do { } while (0)

#endif // UART_H
