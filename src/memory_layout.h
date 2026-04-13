#ifndef MEMORY_LAYOUT_H
#define MEMORY_LAYOUT_H

#include <stdint.h>

/**
 * HiFive Unmatched Memory Layout
 * ================================
 * Total DDR: 2 GB (0x80000000 - 0xFFFFFFFF)
 *
 * Active compact multihart layout:
 * - Hart 0 boot/runtime:   board-managed
 * - Hart 1 (Zephyr):       256 KB @ 0x80240000
 * - Hart 2 (FreeRTOS):     256 KB @ 0x80280000
 * - Hart 3 (Bare-metal):   256 KB @ 0x802C0000
 * - Hart 4 (Bare-metal):   256 KB @ 0x80300000
 * - Shared IPC region:     768 KB @ 0x80340000
 *
 * Alternate Linux AMP layout scaffold:
 * - Hart 0 (Linux):      124 MB @ 0x80400000
 * - Hart 1 (Zephyr):     256 KB @ 0x80240000
 * - Hart 2 (FreeRTOS):   256 KB @ 0x80280000
 * - Hart 3 (Bare-metal): 256 KB @ 0x802C0000
 * - Shared IPC region:   768 KB @ 0x80340000
 */

/* ============================================================
   HART 0 (S7 Boot Coordinator)
   ============================================================ */
#define HART0_DRAM_BASE     0x80000000
#define HART0_DRAM_SIZE     0x00100000  /* 1 MB */
#define HART0_DRAM_END      (HART0_DRAM_BASE + HART0_DRAM_SIZE)

/* Hart 0 stack (grows downward from end) */
#define HART0_STACK_SIZE    0x2000      /* 8 KB */
#define HART0_STACK_TOP     HART0_DRAM_END
#define HART0_STACK_BASE    (HART0_STACK_TOP - HART0_STACK_SIZE)

/* ============================================================
   HART 1 (U74 - Zephyr RTOS)
   ============================================================ */
#define HART1_DRAM_BASE     0x80240000
#define HART1_DRAM_SIZE     0x00040000  /* 256 KB */
#define HART1_DRAM_END      (HART1_DRAM_BASE + HART1_DRAM_SIZE)

#define HART1_STACK_SIZE    0x4000      /* 16 KB */
#define HART1_STACK_TOP     HART1_DRAM_END
#define HART1_STACK_BASE    (HART1_STACK_TOP - HART1_STACK_SIZE)

/* ============================================================
   HART 2 (U74 - FreeRTOS)
   ============================================================ */
#define HART2_DRAM_BASE     0x80280000
#define HART2_DRAM_SIZE     0x00040000  /* 256 KB */
#define HART2_DRAM_END      (HART2_DRAM_BASE + HART2_DRAM_SIZE)

#define HART2_STACK_SIZE    0x4000      /* 16 KB */
#define HART2_STACK_TOP     HART2_DRAM_END
#define HART2_STACK_BASE    (HART2_STACK_TOP - HART2_STACK_SIZE)

/* ============================================================
   HART 3 (U74 - Bare-Metal App 1)
   ============================================================ */
#define HART3_DRAM_BASE     0x802C0000
#define HART3_DRAM_SIZE     0x00040000  /* 256 KB */
#define HART3_DRAM_END      (HART3_DRAM_BASE + HART3_DRAM_SIZE)

#define HART3_STACK_SIZE    0x2000      /* 8 KB */
#define HART3_STACK_TOP     HART3_DRAM_END
#define HART3_STACK_BASE    (HART3_STACK_TOP - HART3_STACK_SIZE)

/* ============================================================
   HART 4 (U74 - Bare-Metal App 2)
   ============================================================ */
#define HART4_DRAM_BASE     0x80300000
#define HART4_DRAM_SIZE     0x00040000  /* 256 KB */
#define HART4_DRAM_END      (HART4_DRAM_BASE + HART4_DRAM_SIZE)

#define HART4_STACK_SIZE    0x2000      /* 8 KB */
#define HART4_STACK_TOP     HART4_DRAM_END
#define HART4_STACK_BASE    (HART4_STACK_TOP - HART4_STACK_SIZE)

/* ============================================================
   HALO IPC Shared Memory Region
   ============================================================
   Used for inter-hart communication, shared data structures,
   and synchronization primitives.
 */
#define HALO_IPC_BASE       0x80340000
#define HALO_IPC_SIZE       0x000C0000  /* 768 KB */
#define HALO_IPC_END        (HALO_IPC_BASE + HALO_IPC_SIZE)
#define HALO_UART_LOG_LOCK_ADDR   (HALO_IPC_END - 0x4)
#define HALO_UART_LOG_LOCK_OFFSET (HALO_IPC_SIZE - 0x4)

/* IPC subregions (4 KB mailboxes + shared payload area; last 4 bytes reserved for UART log lock) */
#define HALO_IPC_CTRL       (HALO_IPC_BASE + 0x00000000)
#define HALO_IPC_HART01     (HALO_IPC_BASE + 0x00001000)
#define HALO_IPC_HART02     (HALO_IPC_BASE + 0x00002000)
#define HALO_IPC_HART03     (HALO_IPC_BASE + 0x00003000)
#define HALO_IPC_HART04     (HALO_IPC_BASE + 0x00004000)
#define HALO_IPC_SHARED     (HALO_IPC_BASE + 0x00010000)

#define HALO_IPC_CHANNEL_SIZE 0x1000   /* 4 KB per mailbox */

/* ============================================================
   Alternate Linux AMP Region
   ============================================================ */
#define LINUX_AMP_LINUX_BASE  0x80400000
#define LINUX_AMP_LINUX_SIZE  0x07C00000  /* 124 MB */
#define LINUX_AMP_LINUX_END   (LINUX_AMP_LINUX_BASE + LINUX_AMP_LINUX_SIZE)


/* CLINT offsets */
#define CLINT_MSIP_BASE     (CLINT_BASE + 0x0000)
#define CLINT_MTIMECMP_BASE (CLINT_BASE + 0x4000)
#define CLINT_MTIME_BASE    (CLINT_BASE + 0xBFF8)

#endif /* MEMORY_LAYOUT_H */
