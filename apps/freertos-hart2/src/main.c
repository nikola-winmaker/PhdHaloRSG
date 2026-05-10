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
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t battery_voltage_mv;
    int32_t charge_current_ma;
    float battery_temp_c;
    bool breaker_closed;
    uint32_t fault_flags;

} SensorFrame;

typedef struct
{
    bool enable_charging;
    uint32_t current_limit_ma;
    uint32_t voltage_limit_mv;
    char charging_mode[7];

} ChargeCommand;

typedef struct
{
    char charger_state[5];
    uint32_t requested_current_ma;
    uint32_t requested_voltage_mv;
    uint32_t fault_state;

} ChargeStatus;

typedef struct
{
    uint32_t command_id;
    int32_t command_param;

} OperatorCommand;
/************************* FUNCTION SECTION *************************/
#if !defined(USE_HALO) || (USE_HALO == 0)

typedef enum
{
    IPC_IDLE = 0,
    IPC_WRITING = 1,
    IPC_READING = 2

} IpcStatus;

#include <string.h>

typedef struct
{
    volatile uint32_t status;

} MailboxHeader;

#define SENSOR_STATUS \
    (*(volatile uint32_t*)SENSOR_FRAME_BASE)

#define SENSOR_DATA \
    ((volatile SensorFrame*)(SENSOR_FRAME_BASE + sizeof(uint32_t)))


#define CHARGE_CMD_STATUS \
    (*(volatile uint32_t*)CHARGE_COMMAND_BASE)

#define CHARGE_CMD_DATA \
    ((volatile ChargeCommand*)(CHARGE_COMMAND_BASE + sizeof(uint32_t)))


#define CHARGE_STATUS_FLAG \
    (*(volatile uint32_t*)CHARGE_STATUS_BASE)

#define CHARGE_STATUS_DATA \
    ((volatile ChargeStatus*)(CHARGE_STATUS_BASE + sizeof(uint32_t)))


#define OPERATOR_STATUS \
    (*(volatile uint32_t*)OPERATOR_COMMAND_BASE)

#define OPERATOR_DATA \
    ((volatile OperatorCommand*)(OPERATOR_COMMAND_BASE + sizeof(uint32_t)))

static bool ipc_write(volatile uint32_t* status,
                      volatile void* destination,
                      const void* source,
                      uint32_t size)
{
    if (*status != IPC_IDLE)
    {
        return false;
    }

    *status = IPC_WRITING;

    memcpy((void*)destination,
           source,
           size);

    *status = IPC_IDLE;

    return true;
}

static bool ipc_read(volatile uint32_t* status,
                     const volatile void* source,
                     void* destination,
                     uint32_t size)
{
    if (*status != IPC_IDLE)
    {
        return false;
    }

    *status = IPC_READING;

    memcpy(destination,
           (const void*)source,
           size);

    *status = IPC_IDLE;

    return true;
}

bool write_sensor_frame(const SensorFrame* data)
{
    return ipc_write(
        &SENSOR_STATUS,
        SENSOR_DATA,
        data,
        sizeof(SensorFrame));
}

bool read_sensor_frame(SensorFrame* data)
{
    return ipc_read(
        &SENSOR_STATUS,
        SENSOR_DATA,
        data,
        sizeof(SensorFrame));
}

bool write_charge_command(const ChargeCommand* data)
{
    return ipc_write(
        &CHARGE_CMD_STATUS,
        CHARGE_CMD_DATA,
        data,
        sizeof(ChargeCommand));
}

bool read_charge_command(ChargeCommand* data)
{
    return ipc_read(
        &CHARGE_CMD_STATUS,
        CHARGE_CMD_DATA,
        data,
        sizeof(ChargeCommand));
}

bool write_charge_status(const ChargeStatus* data)
{
    return ipc_write(
        &CHARGE_STATUS_FLAG,
        CHARGE_STATUS_DATA,
        data,
        sizeof(ChargeStatus));
}

bool read_charge_status(ChargeStatus* data)
{
    return ipc_read(
        &CHARGE_STATUS_FLAG,
        CHARGE_STATUS_DATA,
        data,
        sizeof(ChargeStatus));
}

bool write_operator_command(const OperatorCommand* data)
{
    return ipc_write(
        &OPERATOR_STATUS,
        OPERATOR_DATA,
        data,
        sizeof(OperatorCommand));
}

bool read_operator_command(OperatorCommand* data)
{
    return ipc_read(
        &OPERATOR_STATUS,
        OPERATOR_DATA,
        data,
        sizeof(OperatorCommand));
}
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



    /*variable of type SensorFrame to hold the last received sensor data */
    SensorFrame sensor_frame;

    /*variable of type OperatorCommand to hold the last received operator command */
    OperatorCommand operator_command;

    /*variable of type ChargeCommand to hold the charge command */
    ChargeCommand charge_command;

    /*variable of type ChargeStatus to hold the charge status */
    ChargeStatus charge_status;

    /* 5. Declare heartbeat_counter as a uint32_t that increments on each loop iteration. */
    uint32_t heartbeat_counter = 0U;

    // Init the charge controller state
    charge_controller_init();

    while( 1 )
    {
        volatile uint32_t* sensor_status =
            (volatile uint32_t*)SENSOR_FRAME_BASE;

        volatile SensorFrame* shared_sensor =
            (volatile SensorFrame*)(SENSOR_FRAME_BASE + sizeof(uint32_t));

        /* Only read if writer is not currently writing */
        if (*sensor_status == IPC_IDLE)
        {
            /* Claim ownership for reading */
            *sensor_status = IPC_READING;

            /* Copy data from shared memory */
            sensor_frame = *shared_sensor;

            /* Release mailbox */
            *sensor_status = IPC_IDLE;

        }

        /*This is an event channel, so you can implement a simple protocol to check for new events/commands.*/

        volatile uint32_t* operator_status =
            (volatile uint32_t*)OPERATOR_COMMAND_BASE;

        volatile OperatorCommand* shared_command =
            (volatile OperatorCommand*)(OPERATOR_COMMAND_BASE + sizeof(uint32_t));

        /* Check if peer finished writing a new command */
        if (*operator_status == IPC_IDLE)
        {
            /* Claim mailbox for reading */
            *operator_status = IPC_READING;

            /* Read command from shared memory */
            operator_command = *shared_command;

            /* Release mailbox */
            *operator_status = IPC_IDLE;
        }
        apply_operator_command( &operator_command );
        build_charge_outputs( &sensor_frame, &charge_command, &charge_status );

        volatile uint32_t* charge_command_flag =
            (volatile uint32_t*)CHARGE_COMMAND_BASE;
        volatile ChargeCommand* shared_charge_cmd =
            (volatile ChargeCommand*)(CHARGE_COMMAND_BASE + sizeof(uint32_t));

        /* Example command to publish */
        charge_command.enable_charging = true;
        charge_command.current_limit_ma = 5000;
        charge_command.voltage_limit_mv = 42000;

        strcpy(charge_command.charging_mode, "FAST");

        /* Only write if mailbox is free */
        if (*charge_command_flag == IPC_IDLE)
        {
            /* Claim mailbox for writing */
            *charge_command_flag = IPC_WRITING;

            /* Write payload to shared memory */
            *shared_charge_cmd = charge_command;

            /* Release mailbox for peer to read */
            *charge_command_flag = IPC_IDLE;
        }
        /* Classical: 10. Publish ChargeStatus to peer using shared memory */

        volatile uint32_t* charge_status_flag =
            (volatile uint32_t*)CHARGE_STATUS_BASE;

        volatile ChargeStatus* shared_status =
            (volatile ChargeStatus*)(CHARGE_STATUS_BASE + sizeof(uint32_t));

        /* Only write if mailbox is free */
        if (*charge_status_flag == IPC_IDLE)
        {
            /* Claim mailbox for writing */
            *charge_status_flag = IPC_WRITING;

            /* Write status to shared memory */
            *shared_status = charge_status;

            /* Release mailbox (peer can now read) */
            *charge_status_flag = IPC_IDLE;
        }
         if( ( heartbeat_counter % 10 ) == 0U){
             uart_log( "[APP2] received mV=%d mA=%d temp=%f breaker_closed=%d faults=%d\n",
                 ( uint32_t ) sensor_frame.battery_voltage_mv,
                 ( uint32_t ) sensor_frame.charge_current_ma,
                 sensor_frame.battery_temp_c,
                 ( uint32_t ) sensor_frame.breaker_closed,
                 ( uint32_t ) sensor_frame.fault_flags );
         }

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
