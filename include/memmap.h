/**
 * HiFive Unmatched Memory Layout (with QEMU OpenSBI)
 * Adjusted for OpenSBI v1.3 M-mode firmware (~362 KB at 0x80000000)
 * 
 * OpenSBI Firmware:      0x80000000 - ~0x80060000
 * Hart 0 (S7):           0x80200000 - 0x80240000 (256 KB, boot coordinator)
 * Hart 3 (U74 BM App):   0x80240000 - 0x80280000 (256 KB)
 * Hart 4 (U74 BM App):   0x80280000 - 0x802C0000 (256 KB)
 * Hart 1 (U74 Zephyr):   0x802C0000 - 0x807C0000 (5 MB)
 * Hart 2 (U74 FreeRTOS): 0x807C0000 - 0x80CC0000 (5 MB)
 * HALO IPC Shared:       0x80CC0000 - 0x80DC0000 (1 MB)
 */

#ifndef MEMMAP_H
#define MEMMAP_H

#include <stdint.h>

/* DDR Base */
#define DDR_BASE            0x80000000UL

/* Hart 0 (S7) Boot Coordinator (QEMU + OpenSBI safe region) */
#define HART0_S7_BASE       0x80200000UL
#define HART0_S7_SIZE       0x00040000UL  // 256 KB

/* Hart 3 (U74) - Bare-Metal App 1 */
#define HART3_BM_APP1_BASE  0x80240000UL
#define HART3_BM_APP1_SIZE  0x00040000UL  // 256 KB

/* Hart 4 (U74) - Bare-Metal App 2 */
#define HART4_BM_APP2_BASE  0x80280000UL
#define HART4_BM_APP2_SIZE  0x00040000UL  // 256 KB

/* Hart 1 (U74) - Zephyr */
#define HART1_ZEPHYR_BASE   0x802C0000UL
#define HART1_ZEPHYR_SIZE   0x00500000UL  // 5 MB

/* Hart 2 (U74) - FreeRTOS */
#define HART2_FREERTOS_BASE 0x807C0000UL
#define HART2_FREERTOS_SIZE 0x00500000UL  // 5 MB

/* HALO IPC Shared Region (optional, if space available) */
#define HALO_IPC_BASE       0x80CC0000UL
#define HALO_IPC_SIZE       0x00100000UL  // 1 MB

/* HALO Mailboxes (per-hart, 64 bytes each) */
#define HALO_MAILBOX(hart)  (HALO_IPC_BASE + ((hart) * 0x40))

/* HALO Barrier (for hart synchronization) */
#define HALO_BARRIER_BASE   (HALO_IPC_BASE + 0x00001000UL)

/* Platform MMIO */
#define UART0_BASE          0x10000000UL  // QEMU virt UART (0x10010000 on real HiFive Unmatched)
#define CLINT_BASE          0x02000000UL  // Core Local Interruptor (timer, IPI)
#define PLIC_BASE           0x0C000000UL  // Platform-Level Interrupt Controller

/* Hart ID Constants */
#define HART_S7_COORD       0
#define HART_U74_1          1  // Zephyr
#define HART_U74_2          2  // FreeRTOS
#define HART_U74_3          3  // BM App 1
#define HART_U74_4          4  // BM App 2

/* Trap Vectors */
#define TRAP_VECTOR_BASE    0x80800000UL  // Hart 0 trap handler (if used)

#endif // MEMMAP_H
