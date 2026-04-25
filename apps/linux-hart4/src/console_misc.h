#ifndef CONSOLE_MISC_H
#define CONSOLE_MISC_H

#include <stdio.h>
#include "workshop_protocol.h"

#define INPUT_BUFFER_SIZE 128U
#define SHMEM_BASE HALO_IPC_BASE
#define SHMEM_SIZE HALO_IPC_SIZE
#define EVENTCHANNEL_PAGE_SIZE 0x1000UL

extern unsigned char * g_shmem_region;
extern char line_buffer[ INPUT_BUFFER_SIZE ];

int configure_input_fd( void );
int parse_operator_command( const char * line, void * command );
int maybe_send_operator_command( const char * line, void * command );
int service_console_input( int input_fd, void * command );
int map_shared_region( void );
void set_memory_regions(void);
void * get_external_buffer( uint32_t offset );

#endif /* CONSOLE_MISC_H */