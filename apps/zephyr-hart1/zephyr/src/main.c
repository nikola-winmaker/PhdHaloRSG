#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "../../../../src/shared_memory.h"

#if !defined(USE_HALO) || (USE_HALO == 0)
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

static void spin_delay(void)
{
    for (volatile uint64_t i = 0; i < 900000000ULL; ++i) {
        __asm__ volatile ("nop");
    }
}

int main(void)
{
    uint32_t count = 0;
    uint32_t seen[HALO_MAX_HARTS] = {0};

#if !defined(USE_HALO) || (USE_HALO == 0)
    classical_hello();
#else
    printk("[APP1] Zephyr App HALO\n");
    halo_zephyr_h1_init_riscv64_h1_zephyr();
#endif


    halo_shared_publish(1U, "zephyr", 0U);

    while (1) {
        halo_shared_publish(1U, "zephyr", count);
        printk("[APP1] Zephyr heartbeat %u\n", count++);

        for (uint32_t hart = 1U; hart < HALO_MAX_HARTS; ++hart) {
            volatile halo_shared_slot_t *slot;

            if (hart == 1U) {
                continue;
            }

            slot = halo_shared_slot(hart);

            if (slot->magic == HALO_SLOT_MAGIC && slot->heartbeat != seen[hart]) {
                seen[hart] = slot->heartbeat;
                printk("[APP1] saw Hart%u (%s) heartbeat %u\n",
                       hart, slot->name, slot->heartbeat);
            }
        }

        spin_delay();
    }

    return 0;
}
