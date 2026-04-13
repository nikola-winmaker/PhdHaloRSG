#include "console_lock.h"

// Adress of the console lock in the shared IPC region
#define CONSOLE_LOCK_ADDR ((console_lock_t *)HALO_UART_LOG_LOCK_ADDR)

// console log should be static mutex here from addr CONSOLE_LOCK_ADDR
static console_lock_t* console_lock = CONSOLE_LOCK_ADDR;

void console_lock_acquire(void) {
    for (;;) {
        while (__atomic_load_n(&console_lock->state, __ATOMIC_ACQUIRE) != 0U) {
            __asm__ volatile("nop");
        }

        if (__atomic_exchange_n(&console_lock->state, 1U, __ATOMIC_ACQ_REL) == 0U) {
            return;
        }
    }
}

void console_lock_release(void) {
    __atomic_store_n(&console_lock->state, 0U, __ATOMIC_RELEASE);
}