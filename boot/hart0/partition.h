/**
 * Partition Loader
 * 
 * Loads kernel/RTOS binaries from storage into DDR
 * For now: assume binaries are pre-loaded or read from QSPI
 */

#ifndef PARTITION_LOADER_H
#define PARTITION_LOADER_H

#include <stdint.h>

typedef enum {
    PART_ZEPHYR = 1,
    PART_FREERTOS = 2,
    PART_BM_APP1 = 3,
    PART_BM_APP2 = 4
} partition_id_t;

typedef struct {
    uint32_t id;
    uint64_t ddr_base;
    uint64_t ddr_size;
    uint64_t entry_point;
    const char *name;
} partition_t;

/**
 * Load a partition from storage into DDR
 * Returns 0 on success, -1 on failure
 */
int partition_load(partition_id_t id, partition_t *info);

/**
 * Initialize partition metadata
 */
void partition_init(void);

/**
 * Get partition info by ID
 */
partition_t *partition_get(partition_id_t id);

#endif // PARTITION_LOADER_H
