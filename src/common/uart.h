# Common UART utilities for HiFive Unmatched

.section .text
.global uart_init
.global uart_putchar
.global uart_getchar

# UART0 Base Address: 0x10010000
# TXDATA: offset 0x00
# RXDATA: offset 0x04
# TXCTRL: offset 0x08
# RXCTRL: offset 0x0C

.equ UART0_BASE, 0x10010000
.equ UART0_TXDATA, 0x00
.equ UART0_RXDATA, 0x04
.equ UART0_TXCTRL, 0x08
.equ UART0_RXCTRL, 0x0C

# uart_init: Initialize UART0 (placeholder, assumes already initialized by bootloader)
uart_init:
    ret

# uart_putchar(int c) - Write character to UART
# Argument: a0 = character to write
uart_putchar:
    la t0, UART0_BASE
1:
    lw t1, UART0_TXDATA(t0)
    bltz t1, 1b              # Wait for TXDATA ready bit (bit 31)
    sw a0, UART0_TXDATA(t0)  # Write character
    ret

# uart_getchar(void) - Read character from UART
# Return value: a0 = character read (or -1 if no data)
uart_getchar:
    la t0, UART0_BASE
    lw a0, UART0_RXDATA(t0)  # Read RXDATA register
    ret
