/**
 * IPC Initialization Implementation
 */

#include "ipc.h"
#include "uart.h"
#include "memmap.h"
#include <sbi/sbi_console.h>

/* Use standalone UART for testing without OpenSBI */
#define IPC_DEBUG 0  /* Set to 1 to enable debug messages */
#if IPC_DEBUG
#define ipc_printf uart_printf
#else
#define ipc_printf(fmt, ...) do { } while (0)
#endif

/* Define sbi_printf as no-op for compatibility */
#define sbi_printf ipc_printf

typedef struct {
    volatile uint32_t magic;       /* 0x12345678 = alive */
    volatile uint32_t last_update;
    volatile uint32_t message;
    volatile uint32_t _reserved;
} hart_mailbox_t;

#define MAILBOX_MAGIC 0x12345678

static volatile hart_mailbox_t *get_mailbox(int hartid) {
    return (volatile hart_mailbox_t *)(HALO_MAILBOX(hartid));
}

void ipc_init(void) {
    /* Initialize HALO shared memory region */
    volatile uint32_t *halo_base = (volatile uint32_t *)HALO_IPC_BASE;
    
    /* Clear the entire region */
    for (int i = 0; i < (HALO_IPC_SIZE / 4); i++) {
        halo_base[i] = 0;
    }
    
    sbi_printf("[IPC] Initialized HALO region at 0x%lx (%lu bytes)\n", 
        HALO_IPC_BASE, HALO_IPC_SIZE);
}

void ipc_notify_hart(int target_hart, uint32_t msg) {
    hart_mailbox_t *mbox = get_mailbox(target_hart);
    mbox->message = msg;
    /* TODO: Send IPI to hart */
}

int ipc_check_hart(int hartid) {
    hart_mailbox_t *mbox = get_mailbox(hartid);
    return (mbox->magic == MAILBOX_MAGIC) ? 1 : 0;
}
