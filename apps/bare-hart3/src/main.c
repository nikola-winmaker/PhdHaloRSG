/**
 *                                                                                                      
 *      ▄▄▄▄           ▄▀▀           ▄                  ▄    ▄                 ▀      ▄                 
 *     █▀   ▀  ▄▄▄   ▄▄█▄▄   ▄▄▄   ▄▄█▄▄  ▄   ▄         ██  ██  ▄▄▄   ▄ ▄▄   ▄▄▄    ▄▄█▄▄   ▄▄▄    ▄ ▄▄ 
 *     ▀█▄▄▄  ▀   █    █    █▀  █    █    ▀▄ ▄▀         █ ██ █ █▀ ▀█  █▀  █    █      █    █▀ ▀█   █▀  ▀
 *         ▀█ ▄▀▀▀█    █    █▀▀▀▀    █     █▄█          █ ▀▀ █ █   █  █   █    █      █    █   █   █    
 *     ▀▄▄▄█▀ ▀▄▄▀█    █    ▀█▄▄▀    ▀▄▄   ▀█           █    █ ▀█▄█▀  █   █  ▄▄█▄▄    ▀▄▄  ▀█▄█▀   █    
 *                                         ▄▀                                                           
 *                                        ▀▀                                                            
 */

/************************* INCLUDE SECTION *************************/
#include "uart.h"
#include "safety_eval.h"
#include "workshop_protocol.h"
#include "platform/platform.h"
#include "memory_layout.h"
#if !defined(USE_HALO) || (USE_HALO == 0)
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

/************************* GLOBAL SECTION *************************/
//TODO Classical: Structure definitions for ChargeCommand and SafetyState 
// based on the workshop specification only for the classical implementation!.


/************************* FUNCTION SECTION *************************/
void _start_c( void )
{
    uart_init();

#if !defined(USE_HALO) || (USE_HALO == 0)
    /*
    * Workshop steps for implementing the safety monitor loop:
    ______________________________________________________________________________________________________________________________
    !!!!!!!!Shared Memory Note!!!!!!!
    In this bare-metal application, we are using shared memory to communicate between the safety monitor and the peer. 
    This means that both the safety monitor and the peer will read and write to the same address in memory to exchange messages.

    MEM Adress Map:
    *   ChargeCommand: Written by peer, read by Safety Monitor
        Address: CHARGE_COMMAND_BASE
        Size: CHARGE_COMMAND_SIZE (512 bytes) - ring buffer protocol

    *   SafetyState: Written by Safety Monitor, read by peer
        Address: SAFETY_STATE_BASE
        Size: SAFETY_STATE_SIZE (64 bytes) - blackboard like protocol
    ______________________________________________________________________________________________________________________________

    * 1. Define ChargeCommand struct from Workshop specification -> ChargeCommandIf and declare a variable of this type
        enable charging (unsigned int enable_charging = False)
        current limit (unsigned int current_limit_ma = 0)
        voltage limit (unsigned int voltage_limit_mv = 0)
        charging mode (string charging_mode = "normal" / char charging_mode[7])


    * 2. Define a SafetyState struct from Workshop specification -> SafetyStateIf and declare a variable of this type
        safe mode (uint8_t safe_mode = False)
        breaker open (uint8_t breaker_open = False)
        charging allowed (uint8_t charging_allowed = False)
        heartbeat (uint32_t heartbeat_counter = 0)

    * 3. Define heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 4. Create a while loop in which the safety monitor will run continuously with below steps:
        - Receive ChargeCommand message from peer using shared memory access (read from defined memory address for ChargeCommand)
            -- It's up to you how you want to implement the shared memory protocol, but you can use pointer dereferencing to read from 
            the specific memory address where the ChargeCommand is written by the peer. Synchronization is important here, so make sure to implement a simple 
            protocol to check if new data is available before reading.
    
        - Call evaluate_safety( &ChargeCommand, &SafetyState, heartbeat_counter++ ).

        - Publish/log/send the SafetyState state to the peer using Blackboard protocol (write to defined memory address for SafetyState)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read. 
        
        - Log the Info to the console using uart_log("[APP3] ") which is behaving similar to printf
            -- [APP3] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when safe_mode, breaker_open, or charging_allowed changes, or every N cycles.

            -- Example log messages:
                Received command if mode changes:
                    uart_log( "[APP3] Charging mode=%s\n",
                            command.charging_mode );

                Send state if any value changes or every N cycles:
                    uart_log( "[APP3] safe_mode=%d breaker_closed=%d charging_allowed=%d heartbeat=%d\n",
                        ( uint32_t ) state.safe_mode,
                        ( uint32_t ) state.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
                        ( uint32_t ) state.charging_allowed,
                        ( uint32_t ) state.heartbeat_counter );

        - Use bm_delay_loop( ms ) to create a delay in the loop of 200ms.
    */


    /* TODO Classical: 1. Declare ChargeCommand variable */

    /* TODO Classical: 2. Declare SafetyState variable */

    /* 3. Define heartbeat_counter as a uint32_t that increments on each loop iteration. */
    uint32_t heartbeat_counter = 0;

    while( 1 )
    {

        /* This is a demo loop to showcase the application running 
        TODO Classical: delete it when writing the actual implementation */
        if( ( heartbeat_counter % 10U ) == 0U ){
            uart_log( "[APP3] classical demo loop\n" );
        }

        /*TODO Classical: 4. Receive ChargeCommand message from peer using shared memory access (read from defined memory address for ChargeCommand)
            -- It's up to you how you want to implement the shared memory protocol, but you can use pointer dereferencing to read from 
            the specific memory address where the ChargeCommand is written by the peer. Synchronization is important here, so make sure to implement a simple 
            protocol to check if new data is available before reading.
        */

        /*TODO Classical: 5. Call function evaluate_safety( &ChargeCommand, &SafetyState, heartbeat_counter). */
        // evaluate_safety( &command, &state, heartbeat_counter );

        /*TODO Classical: 6. Publish/log/send the SafetyState state to the peer using Blackboard protocol (write to defined memory address for SafetyState)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.
        */

        /*TODO Classical: 7. Log the Info to the console using uart_log("[APP3] ") which is behaving similar to printf
            -- [APP3] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when safe_mode, breaker_open, or charging_allowed changes, or every N cycles.
            -- Variables are placeholders for the actual variables you will define based on the workshop specification
         */
        // if( ( heartbeat_counter % 10U ) == 0U || 
        //     state.safe_mode != prev_state.safe_mode ||
        //     state.breaker_open != prev_state.breaker_open ||
        //     state.charging_allowed != prev_state.charging_allowed )
        //  {
        //     uart_log( "[APP3] CC safe_mode=%d breaker_closed=%d charging_allowed=%d heartbeat=%d\n",
        //                     ( uint32_t ) state.safe_mode,
        //                     ( uint32_t ) state.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
        //                     ( uint32_t ) state.charging_allowed,
        //                     ( uint32_t ) state.heartbeat_counter );
        // }

        heartbeat_counter++;
        bm_delay_loop( WORKSHOP_SAFETY_PERIOD_MS );
    }

#else
    /*
    * Workshop steps for implementing the safety monitor loop:
    * 1. Declare a variable of type ChargeCommand to hold the last received command
        - ChargeCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h3_baremetal/include/halo_structs.h 

    * 2. Declare a variable of type SafetyState to hold the last evaluated state
        - SafetyState struct is available from HALO generated code from deps/halo/codegen/riscv64_h3_baremetal/include/halo_structs.h 

    * 3. Define heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 4. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h3_baremetal/src/halo_api.c

    * 5. Create a while loop in which the safety monitor will run continuously with below steps:
        - Receive ChargeCommand message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h3_baremetal/include/halo_api.h and deps/halo/codegen/riscv64_h3_baremetal/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command
                }

        - Call evaluate_safety( &ChargeCommand, &SafetyState, heartbeat_counter++ )

        - Send the SafetyState state to the peer every using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP3] SafetyState send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h3_baremetal/include/halo_api.h and deps/halo/codegen/riscv64_h3_baremetal/src/halo_channels.c

        - Log the Info to the console using uart_log("[APP3] ") which is behaving similar to printf
            -- [APP3] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when safe_mode, breaker_open, or charging_allowed changes, or every N cycles.

            -- Example log messages:
                Received command if mode changes:
                    uart_log( "[APP3] Charging mode=%s\n",
                            command.charging_mode );

                Send state if any value changes or every N cycles:
                    uart_log( "[APP3] safe_mode=%d breaker_closed=%d charging_allowed=%d heartbeat=%d\n",
                        ( uint32_t ) state.safe_mode,
                        ( uint32_t ) state.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
                        ( uint32_t ) state.charging_allowed,
                        ( uint32_t ) state.heartbeat_counter );

            

        - Use bm_delay_loop( ms ) to create a delay in the loop of 100ms.
    */

    /*TODO HALO: 1. Declare a variable of type ChargeCommand to hold the last received command
      - ChargeCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h3_baremetal/include/halo_structs.h */

    /*TODO HALO: 2. Declare a variable of type SafetyState to hold the last evaluated state
      - SafetyState struct is available from HALO generated code from deps/halo/codegen/riscv64_h3_baremetal/include/halo_structs.h */
      

    /* 3. Define heartbeat_counter as a uint32_t that increments on each loop iteration.*/
    uint32_t heartbeat_counter = 0U;

    /*TODO HALO: 4. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h3_baremetal/src/halo_api.c
    */

    while( 1 )
    {

        /* This is a demo loop to showcase the application running 
        TODO HALO:  delete it when writing the actual implementation */
        if( ( heartbeat_counter % 10U ) == 0U ){
            uart_log( "[APP3] HALO demo loop\n" );
        }

        int recv_rc;

        /*TODO HALO: 5. Receive ChargeCommand message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h3_baremetal/include/halo_api.h and deps/halo/codegen/riscv64_h3_baremetal/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command

                    uart_log( "[APP3] xxxxx");
                }
            */


        /*TODO HALO: 6. Call evaluate_safety( &ChargeCommand, &SafetyState, heartbeat_counter ) */
        // evaluate_safety( &last_command, &state, heartbeat_counter );


        /*TODO HALO: 7. Send the SafetyState state to the peer every using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP3] SafetyState send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h3_baremetal/include/halo_api.h and deps/halo/codegen/riscv64_h3_baremetal/src/halo_channels.c
            */


        /*TODO HALO: 8. Log the Info to the console using uart_log("[APP3] ") which is behaving similar to printf
            -- [APP3] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when safe_mode, breaker_open, or charging_allowed changes, or every N cycles.
            -- Variables are placeholders for the actual variables you will define based on the workshop specification
         */
        // if( state.safe_mode != last_state.safe_mode ||
        //     state.breaker_open != last_state.breaker_open ||
        //     state.charging_allowed != last_state.charging_allowed 
        //      || ( state.heartbeat_counter % 20U ) == 0U 
        // )
        // {
        //     uart_log( "[APP3] safe_mode=%d breaker_closed=%d charging_allowed=%d heartbeat=%d\n",
        //         ( uint32_t ) state.safe_mode,
        //         ( uint32_t ) state.breaker_open ? 0 : 1, // Convert breaker_open to breaker_closed for logging
        //         ( uint32_t ) state.charging_allowed,
        //         ( uint32_t ) state.heartbeat_counter );
        // }

        heartbeat_counter++;
        bm_delay_loop( WORKSHOP_SAFETY_PERIOD_MS );
    }
#endif
}