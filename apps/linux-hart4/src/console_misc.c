#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "console_lock.h"
#include "workshop_protocol.h"
#include "console_misc.h"
#include "bms.h"


char line_buffer[ INPUT_BUFFER_SIZE ];
unsigned char * g_shmem_region = NULL;

int configure_input_fd( void )
{
    int flags = fcntl( STDIN_FILENO, F_GETFL, 0 );

    if( flags >= 0 )
    {
        ( void ) fcntl( STDIN_FILENO, F_SETFL, flags | O_NONBLOCK );
        return STDIN_FILENO;
    }

    return open( "/dev/console", O_RDONLY | O_NONBLOCK );
}

typedef struct OpCommand {
    uint32_t command_id;
    int32_t command_param;
} OpCommand;

int parse_operator_command( const char * line, void * command )
{
    int value = 0;
    OpCommand * command_casted = (OpCommand *) command;

    while( *line != '\0' && isspace( ( unsigned char ) *line ) )
    {
        line++;
    }

    if( *line == '\0' )
    {
        return 0;
    }

    command_casted->command_id = WORKSHOP_CMD_NOOP;
    command_casted->command_param = 0;

    if( strcmp( line, "start" ) == 0 )
    {
        command_casted->command_id = WORKSHOP_CMD_START;
        return 1;
    }

    if( strcmp( line, "stop" ) == 0 )
    {
        command_casted->command_id = WORKSHOP_CMD_STOP;
        return 1;
    }

    if( strcmp( line, "reset" ) == 0 )
    {
        command_casted->command_id = WORKSHOP_CMD_RESET;
        return 1;
    }

    if( strcmp( line, "mode normal" ) == 0 )
    {
        command_casted->command_id = WORKSHOP_CMD_MODE_NORMAL;
        return 1;
    }

    if( strcmp( line, "mode fast" ) == 0 )
    {
        command_casted->command_id = WORKSHOP_CMD_MODE_FAST;
        return 1;
    }

    if( sscanf( line, "set_current %d", &value ) == 1 )
    {
        command_casted->command_id = WORKSHOP_CMD_SET_CURRENT;
        command_casted->command_param = value;
        return 1;
    }

    if( sscanf( line, "set_voltage %d", &value ) == 1 )
    {
        command_casted->command_id = WORKSHOP_CMD_SET_VOLTAGE;
        command_casted->command_param = value;
        return 1;
    }

    return -1;
}



int maybe_send_operator_command( const char * line, void * command )
{
    OpCommand * command_casted = (OpCommand *) command;

    int parse_rc = parse_operator_command( line, command_casted );

    if( parse_rc == 0 )
    {
        return 0;
    }

    if( parse_rc < 0 )
    {
        return -1;
    }

    // Signalize BMS command also
    write_to_bms_command(g_shmem_region, 
                          ( uint32_t ) command_casted->command_id, 
                          ( uint32_t ) command_casted->command_param );
    return 1;
}


int service_console_input( int input_fd, void * command )
{
    static size_t line_length = 0U;
    char chunk[ 32 ];
    ssize_t read_count;

    if( input_fd < 0 )
    {
        return -1;
    }

    if( line_length == 0U )
    {
        memset( line_buffer, 0, sizeof( line_buffer ) );
    }

    while( ( read_count = read( input_fd, chunk, sizeof( chunk ) ) ) > 0 )
    {
        for( ssize_t i = 0; i < read_count; ++i )
        {
            char ch = chunk[ i ];

            if( ch == '\r' || ch == '\n' )
            {
                if( line_length != 0U )
                {
                    int parse_rc;

                    line_buffer[ line_length ] = '\0';
                    parse_rc = maybe_send_operator_command( line_buffer, command );
                    if( parse_rc < 0 )
                    {
                        line_length = 0U;
                        return -1;
                    }
                    line_length = 0U;
                    return parse_rc;
                }
                memset( line_buffer, 0, sizeof( line_buffer ) );
                continue;
            }

            if( ch == '\b' || ch == 0x7f )
            {
                if( line_length != 0U )
                {
                    line_length--;
                    line_buffer[ line_length ] = '\0';
                }
                continue;
            }

            if( line_length + 1U < sizeof( line_buffer ) )
            {
                line_buffer[ line_length ] = ch;
                line_length++;
                line_buffer[ line_length ] = '\0';
            }
        }
    }
    return 0;
}

#if defined( USE_HALO ) && ( USE_HALO == 1 )
    #include "eventchannel_common.h"
    #include "blackboard_common.h"

    void* eventchannel_get_external_buffer(eventchannel_channel_t *channel){
        (void) channel;
        return get_external_buffer( VIRTUAL_OPERATOR_COMMAND );
    }
#endif

void * get_external_buffer( uint32_t offset )
{
    if( g_shmem_region == NULL )
    {
        return NULL;
    }

    return g_shmem_region + offset;
}

int map_shared_region( void )
{
    int fd = open( "/dev/mem", O_RDWR | O_SYNC );
    void * mapped;

    if( fd < 0 )
    {        
        return -1;
    }

    mapped = mmap( NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SHMEM_BASE );
    close( fd );

    if( mapped == MAP_FAILED )
    {
        return -1;
    }

    g_shmem_region = ( unsigned char * ) mapped;
    if( console_lock_init_mapped_region( g_shmem_region, SHMEM_BASE, SHMEM_SIZE ) != 0 )
    {
        munmap( mapped, SHMEM_SIZE );
        g_shmem_region = NULL;
        return -1;
    }
    memset( g_shmem_region + VIRTUAL_OPERATOR_COMMAND, 0, OPERATOR_COMMAND_SIZE );

    return 0;
}

void set_memory_regions(void){
    #if defined( USE_HALO ) && ( USE_HALO == 1 )
        blackboard_ChargingStatus_external_buffer = get_external_buffer(VIRTUAL_CHARGE_STATUS );
        blackboard_ChargingStatus_external_size = CHARGE_STATUS_SIZE;
        blackboard_SafetyReport_external_buffer = get_external_buffer(VIRTUAL_SAFETY_STATE );
        blackboard_SafetyReport_external_size = SAFETY_STATE_SIZE;
    #endif
}