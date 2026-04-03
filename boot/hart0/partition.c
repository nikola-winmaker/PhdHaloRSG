/**
 * Partition Loader Implementation - UART Binary Transfer
 * 
 * Receives application binaries from host over UART serial link.
 * Simple protocol: Host sends binary blobs in sequence for each partition.
 * 
 * Protocol:
 *   1. Hart 0 sends ready signal: "READY <partition_name>"
 *   2. Host sends: <4-byte size (little-endian)><binary data>
 *   3. Hart 0 receives and writes to DDR, sends "OK"
 *   4. Repeat for each partition
 */

#include "partition.h"
#include "uart.h"
#include "memmap.h"
#include <sbi/sbi_console.h>

/* Use standalone UART for testing without OpenSBI */
#define PART_DEBUG 0  /* Set to 1 to enable debug messages */
#if PART_DEBUG
#define part_printf uart_printf
#else
#define part_printf(fmt, ...) do { } while (0)
#endif

/* Define sbi_printf as no-op for compatibility */
#define sbi_printf part_printf
#include <stdint.h>

/* UART0 for binary data transfer */
#define UART0_BASE          0x10010000UL
#define UART_TXDATA         0x00
#define UART_RXDATA         0x04
#define UART_TXMARK         0x08
#define UART_RXMARK         0x0c

#define TX_FULL             (1 << 31)
#define RX_EMPTY            (1 << 31)

static volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;

/**
 * Read a byte from UART with timeout
 */
static int uart_read_byte(void) {
    uint32_t timeout = 0x1000000;
    while ((uart[UART_RXDATA / 4] & RX_EMPTY) && timeout--);
    
    if (timeout == 0) {
        return -1;  /* Timeout */
    }
    
    return uart[UART_RXDATA / 4] & 0xFF;
}

/**
 * Write a byte to UART
 */
static void uart_write_byte(uint8_t c) {
    while (uart[UART_TXDATA / 4] & TX_FULL);
    uart[UART_TXDATA / 4] = c;
}

/**
 * Send string over UART
 */
static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_write_byte('\r');
        }
        uart_write_byte(*s++);
    }
}

/**
 * Receive binary blob from UART
 * Format: 4-byte size (little-endian) + binary data
 */
static int receive_binary_uart(void *dest, uint32_t max_size, uint32_t *actual_size) {
    /* Read 4-byte size header */
    uint32_t size = 0;
    for (int i = 0; i < 4; i++) {
        int byte = uart_read_byte();
        if (byte < 0) {
            sbi_printf("[UART] ERROR: Timeout reading size header\n");
            return -1;
        }
        size |= ((uint32_t)byte << (i * 8));
    }
    
    if (size > max_size) {
        sbi_printf("[UART] ERROR: Binary size 0x%x exceeds max 0x%x\n", size, max_size);
        return -1;
    }
    
    sbi_printf("[UART] Receiving %u bytes (0x%x)...\n", size, size);
    
    /* Read binary data */
    uint8_t *buf = (uint8_t *)dest;
    for (uint32_t i = 0; i < size; i++) {
        int byte = uart_read_byte();
        if (byte < 0) {
            sbi_printf("[UART] ERROR: Timeout at byte %u of %u\n", i, size);
            return -1;
        }
        buf[i] = (uint8_t)byte;
    }
    
    *actual_size = size;
    sbi_printf("[UART] ✓ Received %u bytes\n", size);
    return 0;
}

static partition_t partitions[] = {
    {
        .id = PART_ZEPHYR,
        .ddr_base = HART1_ZEPHYR_BASE,
        .ddr_size = HART1_ZEPHYR_SIZE,
        .entry_point = HART1_ZEPHYR_BASE,
        .name = "Zephyr (Hart 1)"
    },
    {
        .id = PART_FREERTOS,
        .ddr_base = HART2_FREERTOS_BASE,
        .ddr_size = HART2_FREERTOS_SIZE,
        .entry_point = HART2_FREERTOS_BASE,
        .name = "FreeRTOS (Hart 2)"
    },
    {
        .id = PART_BM_APP1,
        .ddr_base = HART3_BM_APP1_BASE,
        .ddr_size = HART3_BM_APP1_SIZE,
        .entry_point = HART3_BM_APP1_BASE,
        .name = "Bare-Metal App 1 (Hart 3)"
    },
    {
        .id = PART_BM_APP2,
        .ddr_base = HART4_BM_APP2_BASE,
        .ddr_size = HART4_BM_APP2_SIZE,
        .entry_point = HART4_BM_APP2_BASE,
        .name = "Bare-Metal App 2 (Hart 4)"
    },
};

#define NUM_PARTITIONS (sizeof(partitions) / sizeof(partitions[0]))

void partition_init(void) {
    /* Initialize partition management for UART binary transfer */
    sbi_printf("\n[PART] Initialized %lu partitions for UART boot\n", NUM_PARTITIONS);
    sbi_printf("[PART] Waiting for host to send binaries over UART\n");
    sbi_printf("[PART] =========================================\n");
    sbi_printf("[PART] Transfer protocol:\n");
    sbi_printf("[PART]   Host sends: <4-byte size><binary data>\n");
    sbi_printf("[PART]   Size is little-endian (LSB first)\n");
    sbi_printf("[PART] =========================================\n\n");
}

partition_t *partition_get(partition_id_t id) {
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        if (partitions[i].id == id) {
            return &partitions[i];
        }
    }
    return 0;
}

int partition_load(partition_id_t id, partition_t *info) {
    partition_t *p = partition_get(id);
    if (!p) {
        sbi_printf("[PART] ERROR: Partition ID %d not found\n", id);
        return -1;
    }
    
    *info = *p;
    
    /* Send ready signal for this partition */
    sbi_printf("\n[PART] ================================================\n");
    sbi_printf("[PART] Ready to receive: %s\n", p->name);
    sbi_printf("[PART] Destination: 0x%lx, Max size: %lu bytes (0x%lx)\n",
               p->ddr_base, p->ddr_size, p->ddr_size);
    sbi_printf("[PART] Waiting for host to send binary...\n");
    sbi_printf("[PART] ================================================\n");
    
    uart_puts("\nREADY:");
    uart_puts(p->name);
    uart_puts("\n");
    
    /* Receive binary from UART */
    uint32_t received_size = 0;
    int ret = receive_binary_uart((void *)p->ddr_base, p->ddr_size, &received_size);
    if (ret != 0) {
        sbi_printf("[PART] ERROR: Failed to receive %s\n", p->name);
        return ret;
    }
    
    /* Verify received size is reasonable */
    if (received_size == 0) {
        sbi_printf("[PART] WARNING: Received 0 bytes for %s\n", p->name);
    }
    
    sbi_printf("[PART] ✓ %s loaded successfully\n", p->name);
    sbi_printf("[PART]   Base: 0x%lx\n", p->ddr_base);
    sbi_printf("[PART]   Size: %u bytes (0x%x)\n", received_size, received_size);
    sbi_printf("[PART]   Entry: 0x%lx\n\n", p->entry_point);
    
    return 0;
}
