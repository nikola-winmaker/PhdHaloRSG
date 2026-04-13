#ifndef CONSOLE_LOCK_H
#define CONSOLE_LOCK_H

#include <stdint.h>
#include "../memory_layout.h"

typedef struct {
    volatile uint32_t state;
} console_lock_t;

void console_lock_acquire(void);
void console_lock_release(void);

#endif