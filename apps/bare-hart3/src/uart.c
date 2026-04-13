#include "uart.h"
#include <stdarg.h>

static volatile uint8_t* uart = (volatile uint8_t*)UART0_BASE;
static int uart_initialized = 0;
static volatile uint32_t uart_log_lock = 0;

static void uart_lock(void) {
    for (;;) {
        while (__atomic_load_n(&uart_log_lock, __ATOMIC_ACQUIRE) != 0U) {
            __asm__ volatile("nop");
        }

        if (__atomic_exchange_n(&uart_log_lock, 1U, __ATOMIC_ACQ_REL) == 0U) {
            return;
        }
    }
}

static void uart_unlock(void) {
    __atomic_store_n(&uart_log_lock, 0U, __ATOMIC_RELEASE);
}

static void uart_buffer_append_char(char *buffer, size_t *length, char c) {
    if (*length + 1U >= UART_LOG_BUFFER_SIZE) {
        return;
    }

    buffer[*length] = c;
    *length += 1U;
    buffer[*length] = '\0';
}

static void uart_buffer_append_string(char *buffer, size_t *length, const char *s) {
    if (s == 0) {
        s = "(null)";
    }

    while (*s != '\0' && *length + 1U < UART_LOG_BUFFER_SIZE) {
        buffer[*length] = *s++;
        *length += 1U;
    }

    buffer[*length] = '\0';
}

static void uart_buffer_append_int(char *buffer, size_t *length, int value) {
    uint32_t magnitude;
    char digits[11];
    int pos = 10;

    digits[pos] = '\0';

    if (value < 0) {
        uart_buffer_append_char(buffer, length, '-');
        magnitude = (uint32_t)(-(int64_t)value);
    } else {
        magnitude = (uint32_t)value;
    }

    if (magnitude == 0U) {
        uart_buffer_append_char(buffer, length, '0');
        return;
    }

    while (magnitude > 0U && pos > 0) {
        digits[--pos] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    }

    uart_buffer_append_string(buffer, length, &digits[pos]);
}

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

void uart_write_int(uint32_t value) {
    char buffer[11];
    int pos = 10;
    buffer[pos] = '\0';
    if (value == 0) {
        uart_write_char('0');
        return;
    }
    while (value > 0 && pos > 0) {
        buffer[--pos] = '0' + (value % 10);
        value /= 10;
    }
    uart_write_string(&buffer[pos]);
}

void uart_log(const char *format, ...) {
    char buffer[UART_LOG_BUFFER_SIZE];
    size_t length = 0U;
    va_list args;

    if (format == 0) {
        return;
    }

    buffer[0] = '\0';
    uart_buffer_append_string(buffer, &length, "[APP3] ");

    va_start(args, format);

    while (*format != '\0') {
        if (*format != '%') {
            uart_buffer_append_char(buffer, &length, *format++);
            continue;
        }

        ++format;
        if (*format == '\0') {
            break;
        }

        switch (*format) {
            case 'd':
                uart_buffer_append_int(buffer, &length, va_arg(args, int));
                break;
            case 's': {
                uart_buffer_append_string(buffer, &length, va_arg(args, const char *));
                break;
            }
            case 'c':
                uart_buffer_append_char(buffer, &length, (char)va_arg(args, int));
                break;
            case '%':
                uart_buffer_append_char(buffer, &length, '%');
                break;
            default:
                uart_buffer_append_char(buffer, &length, '%');
                uart_buffer_append_char(buffer, &length, *format);
                break;
        }

        ++format;
    }

    va_end(args);

    uart_lock();
    uart_write_string(buffer);
    uart_unlock();
}

void uart_log_line(const char *s) {
    uart_log("%s", s);
}
