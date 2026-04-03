#include <stdint.h>
#include "uart.h"
#include "amp_layout.h"

/**
 * Hart 0 Boot Coordinator with OpenSBI SBI HSM
 * 
 * Uses SBI ecalls to boot secondary harts instead of direct CLINT access
 */

#define SBI_HSM_EID       0x48534D  /* Hart State Management */
#define SBI_HART_START    0x0
#define SBI_HART_STOP     0x1

/* SBI ecall function */
static inline long sbi_ecall(long eid, long fid, long arg0, long arg1, long arg2) {
    long result;
    __asm__ volatile (
        "mv a7, %1\n"      /* Extension ID in a7 */
        "mv a6, %2\n"      /* Function ID in a6 */
        "mv a0, %3\n"      /* Argument 0 in a0 */
        "mv a1, %4\n"      /* Argument 1 in a1 */
        "mv a2, %5\n"      /* Argument 2 in a2 */
        "ecall\n"
        "mv %0, a0\n"      /* Return value in a0 */
        : "=r" (result)
        : "r" (eid), "r" (fid), "r" (arg0), "r" (arg1), "r" (arg2)
        : "a0", "a1", "a2", "a6", "a7"
    );
    return result;
}

volatile uintptr_t g_hart_release_entries[5];
volatile uintptr_t g_hart_release_arg0[5];
volatile uintptr_t g_hart_release_arg1[5];
volatile uintptr_t g_hart_release_arg2[5];
volatile uint32_t g_hart_release_flags[5];

static void uart_write_hex64_plain(uint64_t val) {
    static const char hex[] = "0123456789ABCDEF";

    for (int shift = 60; shift >= 0; shift -= 4) {
        uart_write_char(hex[(val >> shift) & 0xF]);
    }
}

static void uart_write_addr_line(const char *label, uintptr_t addr, const char *suffix) {
    uart_write_string(label);
    uart_write_string("0x");
    uart_write_hex64_plain((uint64_t)addr);
    uart_write_string(suffix);
}

static inline uintptr_t read_mhartid(void) {
    uintptr_t hartid;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hartid));
    return hartid;
}

#ifdef HART0_DIRECT_MMODE
int boot_hart_direct_mmode_args(int hart_id, uintptr_t entry_addr,
                                uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

static inline uintptr_t clint_msip_addr(int hart_id) {
    return 0x02000000UL + ((uintptr_t)hart_id * sizeof(uint32_t));
}

static inline void clint_set_msip(int hart_id, uint32_t value) {
    *(volatile uint32_t *)clint_msip_addr(hart_id) = value;
}
#endif

/* Helper to read 4-byte little-endian size header */
static inline uint32_t read_size_header(void) {
    uint8_t b0 = (uint8_t)uart_read_char();
    uint8_t b1 = (uint8_t)uart_read_char();
    uint8_t b2 = (uint8_t)uart_read_char();
    uint8_t b3 = (uint8_t)uart_read_char();

    return ((uint32_t)b0) |
           ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) |
           ((uint32_t)b3 << 24);
}

uint32_t read_size_header_dbg(void) {
    int b0 = uart_read_char();
    int b1 = uart_read_char();
    int b2 = uart_read_char();
    int b3 = uart_read_char();

    return ((uint32_t)(b0 & 0xFF)) |
           ((uint32_t)(b1 & 0xFF) << 8) |
           ((uint32_t)(b2 & 0xFF) << 16) |
           ((uint32_t)(b3 & 0xFF) << 24);
}

/* Receive binary and store to target address */
int receive_binary(uintptr_t target_addr, int hart_id, uint32_t max_size, const char *role) {
    uart_write_string("[APP");
    uart_write_char('0' + hart_id);
    uart_write_string("] Waiting for ");
    uart_write_string(role);
    uart_write_string("\n");

    int byte = uart_read_char();
    while(byte != 'S') {
        byte = uart_read_char();
        uart_write_char((char)(byte & 0xFF));
    }

    uart_write_string("Ready\n");


    uint32_t size = read_size_header();

    if (size == 0 || size > max_size) {
        uart_write_string(" Invalid size\n");
        return 0;
    }

    //uart_write_string(" Size: ");
    uart_write_hex32(size);
    //uart_write_string(" bytes\n");
    uart_write_string("OK\n");

    
    /* Read binary data into memory */
    uint8_t *dst = (uint8_t *)target_addr;
    uint32_t bytes_read = 0;
    uint32_t sum = 0;
    uint8_t last = 0;
    
    for (uint32_t i = 0; i < size; i++) {
        uint8_t v = (uint8_t)uart_read_char();
        dst[i] = v;
        bytes_read++;
        sum += (uint32_t)v;
        last = v;
    }

    uart_write_string(" LAST=");
    uart_write_hex32((uint32_t)last);
    uart_write_string(" SUM=");
    uart_write_hex32(sum);
    uart_write_string("\n");

    uart_write_string("Loaded ");
    uart_write_hex32(bytes_read);
    uart_write_string(" bytes\n");
    return 1;
}

/* Boot secondary hart using OpenSBI HSM */
int boot_hart_sbi(int hart_id, uintptr_t entry_addr, uintptr_t opaque) {
    uart_write_string("[BOOT] Hart ");
    uart_write_char('0' + hart_id);
    uart_write_string(" from 0x");
    uart_write_hex64_plain((uint64_t)entry_addr);
    if (opaque != 0) {
        uart_write_string(" opaque=0x");
        uart_write_hex64_plain((uint64_t)opaque);
    }
    uart_write_string("...\n");
    
    /* Use SBI HSM sbi_hart_start ecall */
    long result = sbi_ecall(SBI_HSM_EID, SBI_HART_START, hart_id, entry_addr, opaque);
    
    uart_write_string("  SBI result: ");
    uart_write_hex32((uint32_t)result);
    if (result == 0) {
        uart_write_string(" (OK)\n");
        return 1;
    } else {
        uart_write_string(" (ERROR)\n");
        return 0;
    }
}

#ifdef HART0_DIRECT_MMODE
int boot_hart_direct_mmode(int hart_id, uintptr_t entry_addr) {
    return boot_hart_direct_mmode_args(hart_id, entry_addr, (uintptr_t)hart_id, 0, 0);
}

int boot_hart_direct_mmode_args(int hart_id, uintptr_t entry_addr,
                                uintptr_t arg0, uintptr_t arg1, uintptr_t arg2) {
    uart_write_string("[BOOT] Hart ");
    uart_write_char('0' + hart_id);
    uart_write_string(" direct M-mode release to 0x");
    uart_write_hex64_plain((uint64_t)entry_addr);
    if (arg1 != 0 || arg2 != 0) {
        uart_write_string(" a1=0x");
        uart_write_hex64_plain((uint64_t)arg1);
        uart_write_string(" a2=0x");
        uart_write_hex64_plain((uint64_t)arg2);
    }
    uart_write_string("...\n");

    g_hart_release_entries[hart_id] = entry_addr;
    g_hart_release_arg0[hart_id] = arg0;
    g_hart_release_arg1[hart_id] = arg1;
    g_hart_release_arg2[hart_id] = arg2;
    __asm__ volatile("fence rw, rw" ::: "memory");
    g_hart_release_flags[hart_id] = 1;
    __asm__ volatile("fence rw, rw" ::: "memory");
    clint_set_msip(hart_id, 1);

    uart_write_string("  Direct result: 00000000 (OK)\n");
    return 1;
}
#endif

void _start_c(uintptr_t boot_hartid, uintptr_t fdt_addr) {
    if (boot_hartid > 4) {
        boot_hartid = read_mhartid();
    }

    for (int i = 0; i < 5; i++) {
        g_hart_release_entries[i] = 0;
        g_hart_release_arg0[i] = 0;
        g_hart_release_arg1[i] = 0;
        g_hart_release_arg2[i] = 0;
        g_hart_release_flags[i] = 0;
    }

    uart_write_string("\n");
    uart_write_string("╔════════════════════════════════════════════════════════════════╗\n");
    uart_write_string("║          Hart 0 - Multi-Hart Boot Coordinator                 ║\n");
    uart_write_string("║               Ready to receive applications                   ║\n");
    uart_write_string("╚════════════════════════════════════════════════════════════════╝\n\n");

    uart_write_string("System Status:\n");
    uart_write_string("  - Boot Hart ID: ");
    uart_write_hex32((uint32_t)boot_hartid);
    uart_write_string("\n");
    uart_write_string("  - Hart Count: 5 (1 coordinator + 4 secondary)\n");
    uart_write_string("  - UART Console: Operational\n");
    uart_write_string("  - Memory Regions: Configured\n");
#ifdef HART0_DIRECT_MMODE
    uart_write_string("  - Boot Mode: Direct M-mode secondary release\n");
#else
    uart_write_string("  - Boot Mode: OpenSBI payload + SBI HSM\n");
#endif
    uart_write_string("  - OpenSBI FDT: 0x");
    uart_write_hex64_plain((uint64_t)fdt_addr);
    uart_write_string("\n");
    uart_write_string("  - Application Load: ");
#ifdef HART0_QEMU_PRELOAD
    uart_write_string("QEMU preload mode\n\n");
#else
    uart_write_string("UART receive mode\n\n");
#endif
    
    uart_write_string("Memory Layout:\n");
    uart_write_addr_line("  - Hart 0: ", HART0_ADDR, " (64 KB)\n");
    uart_write_addr_line("  - Hart 1: ", HART1_ADDR, " (256 KB app space)\n");
    uart_write_addr_line("  - Hart 2: ", HART2_ADDR, " (256 KB app space)\n");
    uart_write_addr_line("  - Hart 3: ", HART3_ADDR, " (256 KB app space)\n");
#ifdef HART0_DIRECT_MMODE
    uart_write_addr_line("  - Hart 4: OpenSBI Linux bridge @ ", OPENSBI_BRIDGE_ADDR, "\n");
    uart_write_addr_line("  - Linux kernel @ ", LINUX_ADDR, " (124 MB image space)\n\n");
#else
    uart_write_addr_line("  - Hart 4: Linux kernel @ ", LINUX_ADDR, " (124 MB image space)\n\n");
#endif

    uart_write_string("Status: ");
#ifdef HART0_QEMU_PRELOAD
#ifdef HART0_DIRECT_MMODE
    uart_write_string("QEMU preload mode enabled\n");
    uart_write_string("  Expecting app1/app2/app3/OpenSBI bridge/Linux preloaded in RAM by QEMU\n\n");
#else
    uart_write_string("QEMU preload mode enabled\n");
    uart_write_string("  Expecting app1/app2/app3/Linux preloaded in RAM by QEMU\n\n");
#endif
#else
#ifdef HART0_DIRECT_MMODE
    uart_write_string("Ready for application binaries\n");
    uart_write_string("  Send app1.bin, app2.bin, app3.bin, then Linux bridge/fw_jump\n");
    uart_write_string("  app1-3: 4-byte LE size header + payload (256 KB max)\n");
    uart_write_string("  app4:   4-byte LE size header + payload (384 KB max)\n\n");
#else
    uart_write_string("Ready for application binaries\n");
    uart_write_string("  Send app1.bin, app2.bin, app3.bin, then Linux Image as app4\n");
    uart_write_string("  app1-3: 4-byte LE size header + payload (256 KB max)\n");
    uart_write_string("  app4:   4-byte LE size header + payload (124 MB max)\n\n");
#endif
#endif
    
    uart_write_string("════════════════════════════════════════════════════════════════\n");
    uart_write_string("Loading Applications\n");
    uart_write_string("════════════════════════════════════════════════════════════════\n\n");
    
    /* Hart memory addresses and IDs */
    struct {
        uintptr_t load_addr;
        uintptr_t entry_addr;
        uintptr_t opaque;
        uint32_t max_size;
        int id;
        const char *role;
        int is_linux;
        int loaded;
    } harts[] = {
        {HART1_ADDR, HART1_ADDR, 0, SMALL_APP_MAX_SIZE, 1, "Zephyr app", 0, 0},
        {HART2_ADDR, HART2_ADDR, 0, SMALL_APP_MAX_SIZE, 2, "FreeRTOS app", 0, 0},
        {HART3_ADDR, HART3_ADDR, 0, SMALL_APP_MAX_SIZE, 3, "bare-metal app", 0, 0},
#ifdef HART0_DIRECT_MMODE
        {OPENSBI_BRIDGE_ADDR, OPENSBI_BRIDGE_ADDR, LINUX_DTB_ADDR, OPENSBI_BRIDGE_MAX_SIZE, 4, "Linux OpenSBI bridge", 1, 0}
#else
        {LINUX_ADDR, LINUX_ADDR, fdt_addr, LINUX_MAX_SIZE, 4, "Linux Image", 1, 0}
#endif
    };
    
    /* Receive and load all 4 applications */
    int loaded = 0;

#ifdef HART0_QEMU_PRELOAD
    for (int i = 0; i < 4; i++) {
        if ((uintptr_t)harts[i].id == boot_hartid) {
            uart_write_string("[PRELOAD] APP");
            uart_write_char('0' + harts[i].id);
            uart_write_string(" target hart is busy with Hart0 coordinator, remapping to hart 0\n");
            harts[i].id = 0;
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        uart_write_string("[PRELOAD] APP");
        uart_write_char('0' + harts[i].id);
        uart_write_string(" ");
        uart_write_string(harts[i].role);
        uart_write_string(" @ ");
        uart_write_string("0x");
        uart_write_hex64_plain((uint64_t)harts[i].load_addr);
        uart_write_string(" -> hart ");
        uart_write_char('0' + harts[i].id);
        if (harts[i].is_linux) {
            uart_write_string(" (entry=");
            uart_write_string("0x");
            uart_write_hex64_plain((uint64_t)harts[i].entry_addr);
            uart_write_string(", fdt=");
            uart_write_string("0x");
            uart_write_hex64_plain((uint64_t)harts[i].opaque);
            uart_write_string(")");
        }
        uart_write_string("\n");
        harts[i].loaded = 1;
        loaded++;
    }
#else
    for (int i = 0; i < 4; i++) {
        if (!receive_binary(harts[i].load_addr, harts[i].id, harts[i].max_size, harts[i].role)) {
            uart_write_string("  (skipped)\n");
            continue;
        }
        harts[i].loaded = 1;
        loaded++;
    }
#endif
    
    uart_write_string("\n");
    uart_write_string("════════════════════════════════════════════════════════════════\n");
    uart_write_string("[STATUS] Loaded ");
    uart_write_hex32(loaded);
#ifdef HART0_DIRECT_MMODE
    uart_write_string("/4 payloads - Releasing secondary harts in direct M-mode\n");
#else
    uart_write_string("/4 payloads - Booting secondary harts via OpenSBI HSM\n");
#endif
    uart_write_string("════════════════════════════════════════════════════════════════\n\n");
    
    /* Boot all secondary harts. */
    int booted = 0;
    for (int i = 0; i < 4; i++) {
        if (!harts[i].loaded) {
            continue;
        }
#ifndef HART0_DIRECT_MMODE
        if (harts[i].is_linux && harts[i].opaque == 0) {
            uart_write_string("[BOOT] Hart ");
            uart_write_char('0' + harts[i].id);
            uart_write_string(" Linux skipped: missing FDT pointer from OpenSBI\n");
            continue;
        }
        if (boot_hart_sbi(harts[i].id, harts[i].entry_addr, harts[i].opaque)) {
            booted++;
        }
#else
        if (harts[i].is_linux) {
            if (boot_hart_direct_mmode_args(harts[i].id, harts[i].entry_addr,
                                            (uintptr_t)harts[i].id,
                                            LINUX_DTB_ADDR,
                                            0)) {
                booted++;
            }
        } else if (boot_hart_direct_mmode(harts[i].id, harts[i].entry_addr)) {
            booted++;
        }
#endif
    }
    
    uart_write_string("\n[STATUS] Booted ");
    uart_write_hex32(booted);
    uart_write_string("/");
    uart_write_hex32(loaded);
    uart_write_string(" secondary harts\n");
    uart_write_string("[WAIT] Entering monitor mode. Harts should print hello messages...\n\n");
    
    /* Main loop with spinner */
    uint32_t count = 0;
    const char *spinner = "|/-\\";
    
    while (1) {
        count++;
        if ((count & 0x1FFFFFF) == 0) {
            int idx = (count >> 25) & 3;
            uart_write_string("[");
            uart_write_char(spinner[idx]);
            uart_write_string("] Hart 0 monitoring\n");
        }
        __asm__ volatile ("wfi");
    }
}

static inline unsigned long read_mcause(void) {
    unsigned long x;
    __asm__ volatile("csrr %0, mcause" : "=r"(x));
    return x;
}

static inline unsigned long read_mepc(void) {
    unsigned long x;
    __asm__ volatile("csrr %0, mepc" : "=r"(x));
    return x;
}

static inline unsigned long read_mtval(void) {
    unsigned long x;
    __asm__ volatile("csrr %0, mtval" : "=r"(x));
    return x;
}

void trap_handler(void) {
    uart_write_string("\nTRAP mcause=0x");
    uart_write_hex32((uint32_t)read_mcause());
    uart_write_string(" mepc=0x");
    uart_write_hex32((uint32_t)read_mepc());
    uart_write_string(" mtval=0x");
    uart_write_hex32((uint32_t)read_mtval());
    uart_write_string("\n");

    while (1) {
        __asm__ volatile("wfi");
    }
}
