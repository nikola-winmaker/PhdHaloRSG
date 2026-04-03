/**
 * IPC Initialization
 * Sets up shared memory region for inter-hart communication
 */

#ifndef IPC_INIT_H
#define IPC_INIT_H

#include <stdint.h>

/**
 * Initialize the HALO IPC shared memory region
 * Sets up mailboxes, barriers, etc. for hart-to-hart communication
 */
void ipc_init(void);

/**
 * Notify a hart via mailbox
 */
void ipc_notify_hart(int target_hart, uint32_t msg);

/**
 * Check hart health via mailbox/heartbeat
 */
int ipc_check_hart(int hartid);

#endif // IPC_INIT_H
