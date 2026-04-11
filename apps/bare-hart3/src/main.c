#include "uart.h"
#include "../../../src/shared_memory.h"
#if !defined(USE_HALO) || (USE_HALO == 0)
#include "classical_api.h"
#else
#include "halo_api.h"
#endif

static void delay_loop(void)
{
    for (volatile uint64_t i = 0; i < 250000000ULL; ++i) {
        __asm__ volatile ("nop");
    }
}

static void uart_write_uint(uint32_t value)
{
    char buf[11];
    int i = 0;

    if (value == 0U) {
        uart_write_char('0');
        return;
    }

    while (value > 0U) {
        buf[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (i > 0) {
        uart_write_char(buf[--i]);
    }
}

void _start_c(void) {
    uint32_t heartbeat = 0U;
    uint32_t seen[HALO_MAX_HARTS] = {0};

    uart_init();
#if !defined(USE_HALO) || (USE_HALO == 0)
    classical_hello();
#else
    uart_write_string("\n╔════════════════════════════════════════════════════════════════╗\n");
    uart_write_string("║               Hart 3 - HALO Application 3                       ║\n");
    uart_write_string("╚════════════════════════════════════════════════════════════════╝\n\n");
#endif

    halo_shared_publish(3U, "bare3", 0U);
#if defined(USE_HALO) && (USE_HALO == 1)
    halo_baremetal_h3_init_riscv64_h3_baremetal();
#endif
    
    /* Main loop - Hart 3 monitoring */
    while (1) {
        halo_shared_publish(3U, "bare3", heartbeat);
        uart_write_string("[APP3] heartbeat ");
        uart_write_uint(heartbeat++);
        uart_write_string("\n");

        for (uint32_t hart = 1U; hart < HALO_MAX_HARTS; ++hart) {
            volatile halo_shared_slot_t *slot;

            if (hart == 3U) {
                continue;
            }

            slot = halo_shared_slot(hart);

            if (slot->magic == HALO_SLOT_MAGIC && slot->heartbeat != seen[hart]) {
                seen[hart] = slot->heartbeat;
                uart_write_string("[APP3] a saw Hart");
                uart_write_uint(hart);
                uart_write_string(" (");
                uart_write_string((const char *)slot->name);
                uart_write_string(") heartbeat ");
                uart_write_uint(slot->heartbeat);
                uart_write_string("\n");
            }
        }

        delay_loop();
    }
}
