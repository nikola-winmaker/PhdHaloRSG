/**
 * Hart Boot Management Header
 */

#ifndef HART_BOOT_H
#define HART_BOOT_H

#include <stdint.h>

/**
 * Start a hart using OpenSBI HSM (Hart State Management)
 */
int hart_boot_start(uint32_t hartid, uint64_t start_addr, uint64_t opaque);

/**
 * Get hart status
 * Returns:
 *   0 = STARTED
 *   1 = STOPPED
 *   2 = START_PENDING
 *   3 = STOP_PENDING
 */
int hart_boot_get_status(uint32_t hartid);

#endif // HART_BOOT_H
