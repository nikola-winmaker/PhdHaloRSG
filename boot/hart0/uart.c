/**
 * Simple UART0 Driver for QEMU virt machine
 * ns16550 compatible UART at 0x10010000
 * 
 * Allows Hart 0 to print without SBI/OpenSBI dependency
 */

#include <stdint.h>
#include <stdarg.h>
#include "uart.h"
#include "memmap.h"

/* NS16550 UART Registers */
#define UART_RBR        0  /* Receiver Buffer Register (read) */
#define UART_THR        0  /* Transmitter Holding Register (write) */
#define UART_IER        1  /* Interrupt Enable Register */
#define UART_IIR        2  /* Interrupt Identification Register */
#define UART_FCR        2  /* FIFO Control Register */
#define UART_LCR        3  /* Line Control Register */
#define UART_MCR        4  /* Modem Control Register */
#define UART_LSR        5  /* Line Status Register */
#define UART_MSR        6  /* Modem Status Register */
#define UART_SPR        7  /* Scratch Pad Register */

#define UART_LSR_THRE   0x20  /* Transmitter Holding Register Empty */
#define UART_LSR_DR     0x01  /* Data Ready */

#define UART_LCR_DLAB   0x80  /* Divisor Latch Access Bit */
#define UART_LCR_CONF   0x03  /* 8-bit data, 1 stop bit, no parity */

#define UART_FCR_EN     0x01  /* Enable FIFOs */
#define UART_FCR_RST    0x06  /* Clear RX/TX FIFOs */

/* QEMU virt uses 115200 baud, divisor=1 for input clock */
#define UART_DIVISOR    1

static int uart_initialized = 0;

/**
 * Initialize ns16550a UART
 * Called once at startup
 */
void uart_init(void) {
    volatile uint8_t *uart = (volatile uint8_t *)UART0_BASE;
    
    if (uart_initialized) return;
    
    /* Set DLAB bit to access divisor latch */
    uart[UART_LCR] = UART_LCR_DLAB;
    
    /* Set baud rate divisor */
    uart[0] = (UART_DIVISOR & 0xFF);        /* LSB of divisor */
    uart[1] = ((UART_DIVISOR >> 8) & 0xFF); /* MSB of divisor */
    
    /* Clear DLAB, set 8-bit data, 1 stop bit, no parity */
    uart[UART_LCR] = UART_LCR_CONF;
    
    /* Enable FIFO, clear buffers */
    uart[UART_FCR] = UART_FCR_EN | UART_FCR_RST;
    
    uart_initialized = 1;
}

/**
 * Write a single character to UART0
 */
void uart_write_char(char c) {
    volatile uint8_t *uart = (volatile uint8_t *)UART0_BASE;
    
    /* Ensure UART is initialized */
    uart_init();
    
    /* Wait for transmitter to be ready */
    while (!(uart[UART_LSR] & UART_LSR_THRE)) {
        /* Spin */
    }
    
    /* Write character */
    uart[UART_THR] = (uint8_t)c;
}

/**
 * Write a string to UART0
 */
void uart_write_string(const char *str) {
    while (*str) {
        uart_write_char(*str);
        if (*str == '\n') {
            uart_write_char('\r');  /* Add carriage return after newline */
        }
        str++;
    }
}

/**
 * Read a single character from UART0 (blocking)
 */
char uart_read_char(void) {
    volatile uint8_t *uart = (volatile uint8_t *)UART0_BASE;
    
    /* Wait for data to be available */
    while (!(uart[UART_LSR] & UART_LSR_DR)) {
        /* Spin */
    }
    
    return (char)uart[UART_RBR];
}

/**
 * Read a single character from UART0 (non-blocking)
 * Returns: character if available, -1 if no data
 */
int uart_read_char_nonblock(void) {
    volatile uint8_t *uart = (volatile uint8_t *)UART0_BASE;
    
    /* Check if data is available */
    if (uart[UART_LSR] & UART_LSR_DR) {
        return (int)uart[UART_RBR];
    }
    
    return -1;  /* No data available */
}

void uart_write_hex8(uint8_t v) {
    const char hex[] = "0123456789ABCDEF";
    uart_write_char(hex[(v >> 4) & 0xF]);
    uart_write_char(hex[v & 0xF]);
}

void uart_write_hex32(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";

    for (int shift = 28; shift >= 0; shift -= 4) {
        uart_write_char(hex[(val >> shift) & 0xF]);
    }
}

void uart_write_hex32_dbg(uint32_t val) {
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t d = (val >> shift) & 0xF;
        if (d < 10) {
            uart_write_char('0' + d);
        } else {
            uart_write_char('A' + (d - 10));
        }
    }
}

/**
 * Write a 32-bit unsigned integer in decimal
 */
void uart_write_uint32(uint32_t val) {
    static char buffer[10];
    int len = 0;
    uart_write_char('A'); // DEBUG: function entered
    if (val == 0) {
        uart_write_char('0');
        uart_write_char('B'); // DEBUG: zero case
        return;
    }
    uint32_t v = val;
    while (v > 0) {
        buffer[len++] = '0' + (v % 10);
        v /= 10;
    }
    uart_write_string(" RAW=");
    for (int i = 0; i < len; i++) {
        uart_write_char('[');
        uart_write_hex8((uint8_t)buffer[i]);
        uart_write_char(']');
    }
    for (int i = len - 1; i >= 0; i--) {
        uart_write_char(buffer[i]);
    }
    uart_write_char('D'); // DEBUG: after output loop
}

/**
 * Write a 64-bit unsigned integer in hexadecimal (0x prefix)
 */
void uart_write_uint64(uint64_t val) {
    uart_write_string("0x");
    
    char buffer[16];
    int len = 0;
    
    if (val == 0) {
        uart_write_char('0');
        return;
    }
    
    uint64_t v = val;
    while (v > 0) {
        uint8_t digit = v & 0xF;
        buffer[len++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        v >>= 4;
    }
    
    for (int i = len - 1; i >= 0; i--) {
        uart_write_char(buffer[i]);
    }
}

/**
 * Simple printf implementation for UART0
 * Supports: %s, %d, %x, %c, %%
 * NOTE: Disabled due to relocation issues - use uart_write_string instead
 */
/* uart_printf disabled to avoid static buffer relocation errors */
