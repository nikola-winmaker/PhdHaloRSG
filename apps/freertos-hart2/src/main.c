/**
 *                                                                                                                           
 *       ▄▄▄  █                                           ▄▄▄                  ▄                  ▀▀█    ▀▀█                 
 *     ▄▀   ▀ █ ▄▄    ▄▄▄    ▄ ▄▄   ▄▄▄▄   ▄▄▄          ▄▀   ▀  ▄▄▄   ▄ ▄▄   ▄▄█▄▄   ▄ ▄▄   ▄▄▄     █      █     ▄▄▄    ▄ ▄▄ 
 *     █      █▀  █  ▀   █   █▀  ▀ █▀ ▀█  █▀  █         █      █▀ ▀█  █▀  █    █     █▀  ▀ █▀ ▀█    █      █    █▀  █   █▀  ▀
 *     █      █   █  ▄▀▀▀█   █     █   █  █▀▀▀▀         █      █   █  █   █    █     █     █   █    █      █    █▀▀▀▀   █    
 *      ▀▄▄▄▀ █   █  ▀▄▄▀█   █     ▀█▄▀█  ▀█▄▄▀          ▀▄▄▄▀ ▀█▄█▀  █   █    ▀▄▄   █     ▀█▄█▀    ▀▄▄    ▀▄▄  ▀█▄▄▀   █    
 *                                  ▄  █                                                                                     
 *                                   ▀▀                                                                                      
 */

 /************************* INCLUDE SECTION *************************/
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "uart.h"
#include "workshop_protocol.h"
#include "charge_ctrl.h"
#include "memory_layout.h"
#if !defined(USE_HALO) || (USE_HALO == 0)
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

/************************* GLOBAL SECTION *************************/

//TODO Classical: Structure definitions for SensorFrame, ChargeCommand, ChargeStatus, 
// and OperatorCommand based on the workshop specification only for the classical implementation!.

/************************* FUNCTION SECTION *************************/
#if !defined(USE_HALO) || (USE_HALO == 0)

static void charge_ctrl_task( void * parameters )
{
    ( void ) parameters;

/*
    * Workshop steps for implementing the charging loop:
    ______________________________________________________________________________________________________________________________
    !!!!!!!!Shared Memory Note!!!!!!!
    In this application, we are using shared memory to communicate between the charging controller and the peer. 
    This means that both the charging controller and the peer will read and write to the same address in memory to exchange messages.

    MEM Adress Map:
    *   SensorFrame: Written by peer, read by Charging Controller
        Address: SENSOR_FRAME_BASE - ring buffer protocol
        Size: SENSOR_FRAME_SIZE (512 bytes)

    *   ChargeCommand:  Written by Charging Controller, read by peer
        Address: CHARGE_COMMAND_BASE - ring buffer protocol
        Size: CHARGE_COMMAND_SIZE (512 bytes)

    *   ChargeStatus: Written by Charging Controller, read by peer
        Address: CHARGE_STATUS_BASE - blackboard protocol
        Size: CHARGE_STATUS_SIZE (64 bytes)

    *  OperatorCommand: Written by peer, read by Charging Controller
        Address: OPERATOR_COMMAND_BASE - event like protocol
        Size: OPERATOR_COMMAND_SIZE (16 bytes)
    ______________________________________________________________________________________________________________________________

    * 1. Define SensorFrame struct from Workshop specification -> SensorFrameIf and declare a variable of this type
        battery voltage (uint32_t battery_voltage_mv = 0)
        charge current (int32_t charge_current_ma = 0)
        battery temperature (float battery_temp_c = 0.0)
        breaker state (uint8_t breaker_closed = 0)
        fault/status flags (uint32_t fault_flags = 0)

    * 2. Define a OperatorCommand struct from Workshop specification -> OperatorCommandIf and declare a variable of this type
        command id (uint32_t command_id = 0)
        parameter (int32_t command_param = 0)

    * 3. Define ChargeCommand struct from Workshop specification -> ChargeCommandIf and declare a variable of this type
        enable charging (uint8_t enable_charging = 0)
        current limit (uint32_t current_limit_ma = 0)
        voltage limit (uint32_t voltage_limit_mv = 0)
        charging mode (char charging_mode[7] = "normal")

    * 4. Define ChargeStatus struct from Workshop specification -> ChargeStatusIf and declare a variable of this type
        charger state (string charger_state = "idle" / char charger_state[5] = "idle")
        requested current (uint32_t requested_current_ma = 0)
        requested voltage (uint32_t requested_voltage_mv = 0)
        fault state (uint32_t fault_state = 0)

    * 5. Define heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 6. Create a while loop in which the charge controller will run continuously with below steps:

        - Receive SensorFrame message from peer using shared memory access (read from defined memory address for SensorFrame)
            -- It's up to you how you want to implement the shared memory protocol, you can use pointer dereferencing to read from 
            the specific memory address where the SensorFrame is written by the peer. Synchronization is important here, so make sure to implement a simple 
            protocol to check if new data is available before reading.
    
        - Receive OperatorCommand message from peer using shared memory access (read from defined memory address for OperatorCommand)
            -- Similar to SensorFrame, use pointer dereferencing to read the OperatorCommand from the defined memory address. 
            This is an event channel, so you can implement a simple protocol to check for new events/commands.
            
        - Call apply_operator_command(&OperatorCommand ); to apply the received operator command to the charge controller. 

        - Call build_charge_outputs( &SensorFrame, &ChargeCommand, &ChargeStatus );

        - Publish/log/send the ChargeCommand command to the peer using shared memory access (write to defined memory address for ChargeCommand)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.

        - Publish/log/send the ChargeStatus status to the peer using shared memory access (write to defined memory address for ChargeStatus)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.

        - Log the Info to the console using uart_log("[APP2] ") which is behaving similar to printf
            -- [APP2] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.

            -- Example log messages:
                Every N cycles for SensorFrame:
                    uart_log( "[APP2] received mV=%d mA=%d temp=%d breaker_closed=%d faults=%d\n",
                        ( uint32_t ) sensor_frame.battery_voltage_mv,
                        ( uint32_t ) sensor_frame.charge_current_ma,
                        ( uint32_t ) sensor_frame.battery_temp_c,
                        ( uint32_t ) sensor_frame.breaker_closed,
                        ( uint32_t ) sensor_frame.fault_flags );

        - Use vTaskDelay( pdMS_TO_TICKS( WORKSHOP_CHARGE_PERIOD_MS ) ); to create a delay in the loop.
    */



    /*TODO Classical: 1. Declare a variable of type SensorFrame to hold the last received sensor data */

    /*TODO Classical: 2. Declare a variable of type OperatorCommand to hold the last received operator command */

    /*TODO Classical: 3. Declare a variable of type ChargeCommand to hold the charge command */

    /*TODO Classical: 4. Declare a variable of type ChargeStatus to hold the charge status */

    /* 5. Declare heartbeat_counter as a uint32_t that increments on each loop iteration. */
    uint32_t heartbeat_counter = 0U;

    // Init the charge controller state
    charge_controller_init();

    while( 1 )
    {
        
        /* This is a demo loop to showcase the application running */
        // TODO Classical: Delete the demo loop when writing the actual implementation
        if( ( heartbeat_counter % 20U ) == 0U ){
            uart_log( "[APP2] classical demo loop\n" );
        }

        /*TODO Classical: 6. Receive SensorFrame message from peer using shared memory access (read from defined memory address for SensorFrame)
            -- It's up to you how you want to implement the shared memory protocol, you can use pointer dereferencing to read from 
            the specific memory address where the SensorFrame is written by the peer. Synchronization is important here, so make sure to implement a simple 
            protocol to check if new data is available before reading.
        */

        /*TODO Classical: 7. Receive OperatorCommand message from peer using shared memory access (read from defined memory address for OperatorCommand)
            -- Similar to SensorFrame, use pointer dereferencing to read the OperatorCommand from the defined memory address. 
            This is an event channel, so you can implement a simple protocol to check for new events/commands.
        */

        /*TODO Classical: 8. Call apply_operator_command(&OperatorCommand ); to apply the received operator command to the charge controller. 
            Call build_charge_outputs( &SensorFrame, &ChargeCommand, &ChargeStatus );
        */
        // apply_operator_command( &operator_command );
        // build_charge_outputs( &sensor_frame, &charge_command, &charge_status );


        /*TODO Classical: 9. Publish/log/send the ChargeCommand command to the peer using shared memory access (write to defined memory address for ChargeCommand)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.
        */

        /*TODO Classical: 10. Publish/log/send the ChargeStatus status to the peer using shared memory access (write to defined memory address for ChargeStatus)
             -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.
        */


        /*TODO Classical: 11. Log the Info to the console using uart_log("[APP2] ") which is behaving similar to printf
            -- [APP2] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.
            -- Variables are placeholders for the actual variables you will define based on the workshop specification
        */
        // if( ( heartbeat_counter % 10 ) == 0U){
        //     uart_log( "[APP2] received mV=%d mA=%d temp=%f breaker_closed=%d faults=%d\n",
        //         ( uint32_t ) sensor_frame.battery_voltage_mv,
        //         ( uint32_t ) sensor_frame.charge_current_ma,
        //         sensor_frame.battery_temp_c,
        //         ( uint32_t ) sensor_frame.breaker_closed,
        //         ( uint32_t ) sensor_frame.fault_flags );
        // }

        heartbeat_counter++;
        vTaskDelay( pdMS_TO_TICKS( WORKSHOP_CHARGE_PERIOD_MS ) );
    }
}

#else // HALO Implementation

/*
    * Workshop steps for implementing the charging loop:
    * 1. Declare a variable of type SensorFrame to hold the last received sensor data
      - SensorFrame struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h
    
    * 2. Declare a variable of type OperatorCommand to hold the last received operator command
      - OperatorCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h

    * 3. Declare a variable of type ChargeCommand to hold the charge command
      - ChargeCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h

    * 4. Declare a variable of type ChargeStatus to hold the charge status
      - ChargeStatus struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h

    * 5. Declare heartbeat_counter as a uint32_t that increments on each loop iteration.

    * 6. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h2_freertos/src/halo_api.c

    * 7. Create a while loop in which the charge controller will run continuously with below steps:

        - Receive SensorFrame message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command
                }

        - Receive OperatorCommand message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command
                }

        - Call apply_operator_command(&OperatorCommand ); to apply the received operator command to the charge controller. 

        - Call build_charge_outputs( &SensorFrame, &ChargeCommand, &ChargeStatus );

        - Send the ChargeCommand to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP2] ChargeCommand send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c

        - Send the ChargeStatus to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP2] ChargeStatus send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c

        - Log the Info to the console using uart_log("[APP2] ") which is behaving similar to printf
            -- [APP2] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.

            -- Example log messages:
                Every N cycles for SensorFrame:
                    uart_log( "[APP2] received mV=%d mA=%d temp=%d breaker_closed=%d faults=%d\n",
                        ( uint32_t ) sensor_frame.battery_voltage_mv,
                        ( uint32_t ) sensor_frame.charge_current_ma,
                        ( uint32_t ) sensor_frame.battery_temp_c,
                        ( uint32_t ) sensor_frame.breaker_closed,
                        ( uint32_t ) sensor_frame.fault_flags );

        - Use vTaskDelay( pdMS_TO_TICKS( WORKSHOP_SENSOR_PERIOD_MS ) ); to create a delay in the loop.
    */


static void charge_ctrl_task( void * parameters )
{
    ( void ) parameters;

    /*TODO HALO: 1. Declare a variable of type SensorFrame to hold the last received sensor data
      - SensorFrame struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h*/
    
    /* TODO HALO: 2. Declare a variable of type OperatorCommand to hold the last received operator command
      - OperatorCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h */

    /* TODO HALO: 3. Declare a variable of type ChargeCommand to hold the charge command
      - ChargeCommand struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h*/

    /* TODO HALO: 4. Declare a variable of type ChargeStatus to hold the charge status
      - ChargeStatus struct is available from HALO generated code from deps/halo/codegen/riscv64_h2_freertos/include/halo_structs.h */

    /* 5. Declare heartbeat_counter as a uint32_t that increments on each loop iteration
    */
    uint32_t heartbeat_counter = 0U;


    // Init the charge controller state
    charge_controller_init();

    /*TODO HALO: 6. Initialize the HALO channels for communication with the peer using init function defined in halo_api.c
         - This will set up the necessary channels for sending and receiving messages with the peer
            -- Full path to init function is in .deps/halo/codegen/riscv64_h2_freertos/src/halo_api.c
    */

    while( 1 )
    {

        int recv_rc;

        /* This is a demo loop to showcase the application running */
        // TODO HALO: Delete the demo loop when writing the actual implementation
        if( ( heartbeat_counter % 10 ) == 0U){
            uart_log( "[APP2] HALO demo loop\n" );
        }

        /*TODO HALO: 7. Receive SensorFrame message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command
                }
        */


        /*TODO HALO: 8. Receive OperatorCommand message for peer in the loop using halo_recv_ API functions defined in halo_api.h
            -- Read all values while they are available in the channel using while loop and checking the return value of halo_recv_ function to check if new data is available
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
            Example:
                while( ( recv_rc = halo_recv_XXXX ) > 0 )
                {
                    // Process the received command
                }
        */


        /*TODO HALO: 9. Call apply_operator_command(&OperatorCommand ); to apply the received operator command to the charge controller. 
            Call build_charge_outputs( &SensorFrame, &ChargeCommand, &ChargeStatus );
        */
        // apply_operator_command(&operator_command );
        // build_charge_outputs( &sensor_frame, &charge_command, &charge_status );


        /*TODO HALO: 10. Send the ChargeCommand to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP2] ChargeCommand send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
        */

        /*TODO HALO: 11. Send the ChargeStatus to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                uart_log( "[APP2] ChargeStatus send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h2_freertos/include/halo_api.h and deps/halo/codegen/riscv64_h2_freertos/src/halo_channels.c
        */

        /*TODO HALO: 12. Log the Info to the console using uart_log("[APP2] ") which is behaving similar to printf
            -- [APP2] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.
            -- Variables are placeholders for the actual variables you will define based on the workshop specification
        */
        // if( ( heartbeat_counter % 10 ) == 0U){
        //     uart_log( "[APP2] received mV=%d mA=%d temp=%f breaker_closed=%d faults=%d\n",
        //         ( uint32_t ) sensor_frame.battery_voltage_mv,
        //         ( uint32_t ) sensor_frame.charge_current_ma,
        //         sensor_frame.battery_temp_c,
        //         ( uint32_t ) sensor_frame.breaker_closed,
        //         ( uint32_t ) sensor_frame.fault_flags );
        // }


        heartbeat_counter++;
        vTaskDelay( pdMS_TO_TICKS( WORKSHOP_CHARGE_PERIOD_MS ) );
    }
}

#endif

int main( void )
{
    uart_init();

    if( xTaskCreate( charge_ctrl_task, "charge_ctrl", configMINIMAL_STACK_SIZE, NULL, 1, NULL ) != pdPASS )
    {
        uart_log( "[APP2] task create failed\n" );

        for( ;; )
        {
            __asm__ volatile ( "wfi" );
        }
    }

    vTaskStartScheduler();
    uart_log( "[APP2] scheduler exited unexpectedly\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}
