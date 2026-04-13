#ifndef CONSOLE_LOCK_H
#define CONSOLE_LOCK_H

#include <stdint.h>
#include <stddef.h>
#include "../memory_layout.h"

typedef struct {
    volatile uint32_t state;
} console_lock_t;

int console_lock_init_mapped_region(void *mapped_base, uintptr_t mapped_phys_base, size_t mapped_size);
void console_lock_acquire(void);
void console_lock_release(void);

#endif