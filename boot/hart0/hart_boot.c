/**
 * Hart Boot Management - OpenSBI HSM Integration
 * Calls into OpenSBI M-mode firmware via SBI ecalls
 * Uses OpenSBI's sbi_printf for console output
 */

#include <stdint.h>
#include "hart_boot.h"
#include "uart.h"

/* OpenSBI SBI ecall interface and console definitions */
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_error.h>

/* Use standalone UART for testing without OpenSBI */
#define BOOT_DEBUG 0  /* Set to 1 to enable debug messages */
#if BOOT_DEBUG
#define boot_printf uart_printf
#else
#define boot_printf(fmt, ...) do { } while (0)
#endif

/**
 * SBI ecall wrapper - raw inline assembly to call into OpenSBI M-mode firmware
 * Parameters match RISC-V ABI: a0-a7 for args, returns result in a0
 * Calling convention: a7=ext_id, a6=func_id, a0-a2=args
 */
static long sbi_ecall(long ext, long fid, long arg0, long arg1, long arg2) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = ext;

    __asm__ __volatile__("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a6), "r"(a7)
        : "memory");

    return a0;
}

/**
 * Start a hart via OpenSBI HSM
 * Uses SBI_EXT_HSM ecall with HART_START function
 */
int hart_boot_start(uint32_t hartid, uint64_t start_addr, uint64_t opaque) {
    long ret;

    boot_printf("[BOOT] Starting Hart %u at 0x%lx\n", hartid, start_addr);

    /* Call SBI_EXT_HSM/HART_START via ecall */
    ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, (long)hartid, start_addr, opaque);

    if (ret != SBI_SUCCESS) {
        boot_printf("[ERROR] Hart %u HSM start failed: %ld\n", hartid, ret);
        return -1;
    }

    return 0;
}

/**
 * Get hart status via OpenSBI HSM
 * Uses SBI_EXT_HSM ecall with HART_GET_STATUS function
 */
int hart_boot_get_status(uint32_t hartid) {
    long status = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_GET_STATUS, (long)hartid, 0, 0);

    if (status < 0) {
        boot_printf("[ERROR] Hart %u get_status failed: %ld\n", hartid, status);
        return -1;
    }

    return (int)status;
}
