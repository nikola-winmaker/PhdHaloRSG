/**
 *                                                                          
 *      ▄▄▄▄                                       ▀                        
 *     █▀   ▀ ▄   ▄  ▄▄▄▄    ▄▄▄    ▄ ▄▄  ▄   ▄  ▄▄▄     ▄▄▄    ▄▄▄    ▄ ▄▄ 
 *     ▀█▄▄▄  █   █  █▀ ▀█  █▀  █   █▀  ▀ ▀▄ ▄▀    █    █   ▀  █▀ ▀█   █▀  ▀
 *         ▀█ █   █  █   █  █▀▀▀▀   █      █▄█     █     ▀▀▀▄  █   █   █    
 *     ▀▄▄▄█▀ ▀▄▄▀█  ██▄█▀  ▀█▄▄▀   █       █    ▄▄█▄▄  ▀▄▄▄▀  ▀█▄█▀   █    
 *                   █                                                      
 *                   ▀                                                      
 */

 /************************* INCLUDE SECTION *************************/
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
#include "bms.h"
#include "console_misc.h"
#if !defined( USE_HALO ) || ( USE_HALO == 0 )
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

/************************* GLOBAL SECTION *************************/

// Flag to control the main loop execution
static volatile sig_atomic_t keep_running = 1;

#if !defined( USE_HALO ) || ( USE_HALO == 0 )
    typedef struct ChargeStatus {
        char charger_state[5];
        uint32_t requested_current_ma;
        uint32_t requested_voltage_mv;
        uint32_t fault_state;
    } ChargeStatus;

    typedef struct SafetyState {
        uint8_t safe_mode;
        uint8_t breaker_open;
        uint8_t charging_allowed;
        uint32_t heartbeat_counter;
    } SafetyState;

    typedef struct OperatorCommand {
        uint32_t command_id;
        int32_t command_param;
    } OperatorCommand;
#endif
/************************* FUNCTION SECTION *************************/
static void on_signal( int sig );

int main( void )
{
    signal( SIGINT, on_signal );
    signal( SIGTERM, on_signal );
    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );
    int command_rcv = 0;
    int input_fd;

    // Configure input file descriptor for non-blocking reads
    input_fd = configure_input_fd();
    // Map shared memory region in user space
    if( map_shared_region() != 0 )
    {   
        printf( "[APP4] failed to map shared memory region\n" );
        while( keep_running )
        {
            __asm__ volatile ( "wfi" );
        }
    }
    // Set user space memory regions for buffers
    set_memory_regions();

#if !defined( USE_HALO ) || ( USE_HALO == 0 )


/*
    * Workshop steps for implementing the Supervision loop:
    ______________________________________________________________________________________________________________________________
    !!!!!!!!Shared Memory Note!!!!!!!
    In this application, we are using shared memory to communicate between the supervisor and the peer. 
    This means that both the supervisor and the peer will read and write to the same address in memory to exchange messages.

    MEM Adress Map in User Space:
    *   OperatorCommand: Written by Supervisor, read by Charging Controller
        Address: get_external_buffer( VIRTUAL_OPERATOR_COMMAND ) returns pointer to this buffer
        Size: VIRTUAL_OPERATOR_COMMAND_SIZE (16 bytes) - event like protocol

    *   ChargeStatus:  Written by Charging Controller, read by Supervisor
        Address: get_external_buffer( VIRTUAL_CHARGE_STATUS ) returns pointer to this buffer
        Size: VIRTUAL_CHARGE_STATUS_SIZE (64 bytes) - blackboard like protocol

    *  SafetyState: Written by Safety Monitor, read by Supervisor
        Address: get_external_buffer( VIRTUAL_SAFETY_STATE ) returns pointer to this buffer
        Size: VIRTUAL_SAFETY_STATE_SIZE (64 bytes) - blackboard like protocol

    ______________________________________________________________________________________________________________________________

    * 1. Define OperatorCommand struct from Workshop specification -> OperatorCommandIf and declare a variable of this type
        command id (uint32_t command_id = 0)
        parameter (int32_t command_param = 0)

    * 2. Define ChargeStatus struct from Workshop specification -> ChargeStatusIf and declare a variable of this type
        charger state (char charger_state[5], default "idle")
        requested current (uint32_t requested_current_ma = 0)
        requested voltage (uint32_t requested_voltage_mv = 0)
        fault state (uint32_t fault_state = 0)

    * 3. Define SafetyState struct from Workshop specification -> SafetyStateIf and declare a variable of this type
        safe mode (uint8_t safe_mode = 0)
        breaker open (uint8_t breaker_open = 0)
        charging allowed (uint8_t charging_allowed = 0)
        heartbeat (uint32_t heartbeat_counter = 0)

    * 4. Declare heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 5. In a while loop in which the Supervisor will run continuously:
        - Get input commands from console by calling
            command_rcv = service_console_input( input_fd, &OperatorCommand );
            if( command_rcv < 0 )
            {
                printf( "[APP4] unknown command %s\n", line_buffer );
            }

        - If command_rcv > 0, it means a valid command was received, so send the OperatorCommand to the peer
            -- Publish/log/send the OperatorCommand command to the peer using shared memory access (write to defined memory address for OperatorCommand)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.

        - Receive ChargeStatus message from peer using shared memory access (read from defined memory address for ChargeStatus)
            -- It's up to you how you want to implement the shared memory protocol, you can use pointer dereferencing to read from the specific memory address where the ChargeStatus is written by the peer. 
            Synchronization is important here, so make sure to implement a simple protocol to check if new data is available before reading.

        - Receive SafetyState message from peer using shared memory access (read from defined memory address for SafetyState)
            -- Similar to ChargeStatus, use pointer dereferencing to read the SafetyState from the defined memory address. Synchronization is important here as well.

        - Use logging has to have [APP4] in every message and perform logging on change to avoid flooding the console with repeated messages. 
            For example, only log when data changes or every N iterations.
*/

    /* 1. Declare a variable of type OperatorCommand */
    OperatorCommand operator_command = {0};
    /* 2. Declare a variable of type ChargeStatus to hold the last evaluated state */
    ChargeStatus charge_status = {0};
    /* 3. Declare a variable of type SafetyState to hold the last evaluated state */
    SafetyState safety_state = {0};
    /* 4. Define heartbeat_counter as a uint32_t that increments on each loop iteration */
    uint32_t heartbeat_counter = 0U;

    while( keep_running )
    {
        /* This is a demo loop to showcase the application running */
        /* Delete the following line once you implement the actual logic */
        //classical_hello();

        command_rcv = service_console_input( input_fd, &operator_command );
        if( command_rcv < 0 )
        {
            printf( "[APP4] unknown command %s\n", line_buffer );
        }

        /* If command_rcv > 0, it means a valid command was received, so send the OperatorCommand to the peer
            -- Publish/log/send the OperatorCommand command to the peer using shared memory access (write to defined memory address for OperatorCommand)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.
        */
        if( command_rcv > 0 )
        {
            *( ( OperatorCommand * ) get_external_buffer( VIRTUAL_OPERATOR_COMMAND ) ) = operator_command;
        }

        /* Receive ChargeStatus message from peer using shared memory access (read from defined memory address for ChargeStatus)
            -- It's up to you how you want to implement the shared memory protocol, you can use pointer dereferencing to read from the specific memory address where the ChargeStatus is written by the peer. 
            Synchronization is important here, so make sure to implement a simple protocol to check if new data is available before reading.
        */
        charge_status = *( ( ChargeStatus * ) get_external_buffer( VIRTUAL_CHARGE_STATUS ) );

        /* Receive SafetyState message from peer using shared memory access (read from defined memory address for SafetyState)
            -- Similar to ChargeStatus, use pointer dereferencing to read the SafetyState from the defined memory address. Synchronization is important here as well.
        */
        safety_state = *( ( SafetyState * ) get_external_buffer( VIRTUAL_SAFETY_STATE ) );

         /* Use logging has to have [APP4] in every message and perform logging on change to avoid flooding the console with repeated messages. 
            For example, only log when data changes or every N iterations.
        */
        if(( heartbeat_counter % 20U ) == 0U )
        {
            console_lock_acquire();
            printf( "[APP4] CC status state=%s current=%u voltage=%u faults=0x%x\n",
                    bms_charge_state( &charge_status ),
                    charge_status.requested_current_ma,
                    charge_status.requested_voltage_mv,
                    charge_status.fault_state );
            console_lock_release();
        }


        if( (heartbeat_counter % 10U) == 0U ) {
            console_lock_acquire();
            printf( "[APP4] CC safe_mode=%u breaker_closed=%u charging_allowed=%u heartbeat=%u\n",
                    safety_state.safe_mode,
                    safety_state.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
                    safety_state.charging_allowed,
                    safety_state.heartbeat_counter );
            console_lock_release();
        }

        heartbeat_counter++;
        usleep( WORKSHOP_COMMAND_PERIOD_MS * 1000U );
    }
#else

/*
    * Workshop steps for implementing the Supervision loop:

    * 1. Declare a variable of type OperatorCommand to hold the last received command 
        - OperatorCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 2. Declare a variable of type ChargeStatus to hold the last received status
        - ChargeStatus struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 3. Declare a variable of type SafetyState to hold the last received status
        - SafetyState struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 4. Define heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 5. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h4_linux/src/halo_api.c

    * 6. In a while loop in which the Supervisor will run continuously:
        - Get input commands from console by calling
            command_rcv = service_console_input( input_fd, &OperatorCommand );
            if( command_rcv < 0 )
            {
                printf( "[APP4] unknown command %s\n", line_buffer );
            }

        - If command_rcv > 0, it means a valid command was received, so send the OperatorCommand to the peer
            -- Use halo_send_ API functions defined in halo_api.h to send the OperatorCommand to the peer
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                printf( "[APP4] failed to send %s", line_buffer );
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c

        - Receive ChargeStatus message from peer using halo_recv_ API functions defined in halo_api.h
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c
            -- Check the return value of halo_recv_ function to ensure the message was received successfully

        - Receive SafetyState message from peer using halo_recv_ API functions defined in halo_api.h
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c
            -- Check the return value of halo_recv_ function to ensure the message was received successfully

        - Use logging has to have [APP4] in every message and perform logging on change to avoid flooding the console with repeated messages. 
            For example, only log when data changes or every N iterations.
    */



    /* 1. Declare a variable of type OperatorCommand to hold the last received command 
        - OperatorCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 2. Declare a variable of type ChargeStatus to hold the last evaluated state
        - ChargeStatus struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 3. Declare a variable of type SafetyState to hold the last received status
        - SafetyState struct is available from HALO generated code from deps/halo/codegen/riscv64_h4_linux/include/halo_structs.h

    * 4. Define heartbeat_counter as a uint32_t that increments on each loop iteration.
    */

    OperatorCommand command;
    ChargeStatus status;
    SafetyState safety;
    ChargeStatus last_status;
    SafetyState last_safety;
    memset( &last_status, 0, sizeof( last_status ) );
    memset( &last_safety, 0, sizeof( last_safety ) );

    uint32_t heartbeat_counter = 0U;
    int recv_rc;

    /* 5. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h4_linux/src/halo_api.c
    */
    halo_linux_h4_init_riscv64_h4_linux();

    while( keep_running )
    {


        /* Get input commands from console by calling
            command_rcv = service_console_input( input_fd, &OperatorCommand );
            if( command_rcv < 0 )
            {
                printf( "[APP4] unknown command %s\n", line_buffer );
            }
        */
        command_rcv = service_console_input( input_fd, &command );
        if( command_rcv < 0 )
        {
            printf( "[APP4] unknown command %s\n", line_buffer );
        }

        /* If command_rcv > 0, it means a valid command was received, so send the OperatorCommand to the peer
            -- Use halo_send_ API functions defined in halo_api.h to send the OperatorCommand to the peer
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                printf( "[APP4] failed to send %s", line_buffer );
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c
        */

        else if( command_rcv > 0 )
        {
            int send_rc = halo_send_OperatorCommandIf_OperatorCommand( &command );
            if( send_rc <= 0 )
            {
                printf( "[APP4] failed to send %s", line_buffer );
            }
            else
            {
                printf( "[APP4] sent '%s'\n", line_buffer );
            }
        }

        /* Receive ChargeStatus message from peer using halo_recv_ API functions defined in halo_api.h
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c
            -- Check the return value of halo_recv_ function to ensure the message was received successfully
        */
        if( (recv_rc = halo_recv_ChargeStatusIf_ChargeStatus( &status ) ) > 0 )
        {
   
        }
        if(recv_rc < 0 )
        {
            printf( "[APP4] failed to receive ChargeStatus\n" );
        }  

        /* Receive SafetyState message from peer using halo_recv_ API functions defined in halo_api.h
            -- Full path is in .deps/halo/codegen/riscv64_h4_linux/include/halo_api.h and deps/halo/codegen/riscv64_h4_linux/src/halo_channels.c
            -- Check the return value of halo_recv_ function to ensure the message was received successfully
        */
        recv_rc = halo_recv_SafetyStateIf_SafetyState( &safety );
        if( recv_rc < 0 )
        {
            printf( "[APP4] failed to receive SafetyState\n" );
        } 

        /* Use logging has to have [APP4] in every message and perform logging on change to avoid flooding the console with repeated messages. 
            For example, only log when data changes or every N iterations.
        */
        if(status.requested_current_ma != last_status.requested_current_ma ||
            status.requested_voltage_mv != last_status.requested_voltage_mv ||
            status.fault_state != last_status.fault_state )
        {
            console_lock_acquire();
            printf( "[APP4] status state=%s current=%u voltage=%u faults=0x%x\n",
                    bms_charge_state( &status ),
                    status.requested_current_ma,
                    status.requested_voltage_mv,
                    status.fault_state );
            console_lock_release();
            last_status = status;
        }

        /* Use logging has to have [APP4] in every message and perform logging on change to avoid flooding the console with repeated messages. 
            For example, only log when data changes or every N iterations.
        */
        if( (heartbeat_counter % 10U) == 0U ) {
            console_lock_acquire();
            printf( "[APP4] safe_mode=%u breaker_closed=%u charging_allowed=%u heartbeat=%u\n",
                    safety.safe_mode,
                    safety.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
                    safety.charging_allowed,
                    safety.heartbeat_counter );
            console_lock_release();
        }


        heartbeat_counter++;
        usleep( WORKSHOP_COMMAND_PERIOD_MS * 1000U );
    }

#endif

    if( input_fd > STDERR_FILENO )
    {
        close( input_fd );
    }

    if( g_shmem_region != NULL )
    {
        munmap( g_shmem_region, SHMEM_SIZE );
    }

    return 0;
}

// Signal handler to gracefully exit the main loop
static void on_signal( int sig )
{
    ( void ) sig;
    keep_running = 0;
}