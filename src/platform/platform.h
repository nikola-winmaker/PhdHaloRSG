/**
 * HiFive Unmatched Platform Definitions
 * Memory layout, hart allocation, and platform constants
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdint.h>

/* Hart IDs */
#define HART_S7_BOOT  0
#define HART_U74_1    1
#define HART_U74_2    2
#define HART_U74_3    3
#define HART_U74_4    4

/* Memory Regions */
#define HART0_MEM_BASE   0x80200000UL
#define HART1_MEM_BASE   0x80240000UL
#define HART2_MEM_BASE   0x80280000UL
#define HART3_MEM_BASE   0x802C0000UL
#define HART4_MEM_BASE   0x80300000UL

#define HART_MEM_SIZE    0x40000UL  /* 256KB per hart */

/* Shared memory region for inter-hart communication. */
#define SHARED_MEM_BASE  0x80340000UL
#define SHARED_MEM_SIZE  0x000C0000UL  /* 768 KB */
#define SHARED_MEM_END   (SHARED_MEM_BASE + SHARED_MEM_SIZE)
#define SHARED_LOG_LOCK_ADDR (SHARED_MEM_END - 0x4UL)

/* Alternate Linux AMP layout: Linux on Hart 0, AMP peers on Harts 1-3. */
#define LINUX_AMP_LINUX_BASE   0x80400000UL
#define LINUX_AMP_LINUX_SIZE   0x07C00000UL  /* 124 MB */
#define LINUX_AMP_LINUX_END    (LINUX_AMP_LINUX_BASE + LINUX_AMP_LINUX_SIZE)

/* Platform Peripherals */
#define CLINT_BASE       0x02000000UL
#define PLIC_BASE        0x0C000000UL

/* CLINT Register Offsets (MTIME, MTIMECMP) */
#define CLINT_MTIME      (CLINT_BASE + 0xBFF8)
#define CLINT_MTIMECMP0  (CLINT_BASE + 0x4000)
#define CLINT_MTIMECMP1  (CLINT_BASE + 0x4008)
#define CLINT_MTIMECMP2  (CLINT_BASE + 0x4010)
#define CLINT_MTIMECMP3  (CLINT_BASE + 0x4018)
#define CLINT_MTIMECMP4  (CLINT_BASE + 0x4020)

/* Inline helpers */
static inline uint32_t read_hart_id(void) {
    uint32_t id;
    __asm__ volatile ("csrr %0, mhartid" : "=r"(id));
    return id;
}

static inline uint64_t read_mtime(void) {
    return *(volatile uint64_t *)CLINT_MTIME;
}

static inline void wfi(void) {
    __asm__ volatile ("wfi");
}


// Delay loop: Uses a busy-wait loop based on the machine timer.
static inline void bm_delay_loop( uint32_t delay_ms )
{
    /* QEMU virt exposes a 10 MHz machine timer, so each millisecond is 10,000 ticks. */
    const uint64_t ticks_per_ms = 10000ULL;
    const uint64_t deadline = read_mtime() + ( ( uint64_t ) delay_ms * ticks_per_ms );

    while( read_mtime() < deadline )
    {
        __asm__ volatile ( "nop" );
    }
}

static inline int32_t temp_to_centic( float temp_c )
{
    return ( int32_t ) ( temp_c * 100.0f );
}

#endif /* __PLATFORM_H__ */
