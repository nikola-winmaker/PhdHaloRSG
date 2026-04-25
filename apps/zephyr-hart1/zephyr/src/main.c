/**
 *                                                                                               
 *      ▄▄▄▄                                            ▄▄▄▄▄▄                 ▀                 
 *     █▀   ▀  ▄▄▄   ▄ ▄▄    ▄▄▄    ▄▄▄    ▄ ▄▄         █      ▄   ▄   ▄▄▄   ▄▄▄     ▄▄▄   ▄ ▄▄  
 *     ▀█▄▄▄  █▀  █  █▀  █  █   ▀  █▀ ▀█   █▀  ▀        █▄▄▄▄▄ █   █  █   ▀    █    █▀ ▀█  █▀  █ 
 *         ▀█ █▀▀▀▀  █   █   ▀▀▀▄  █   █   █            █      █   █   ▀▀▀▄    █    █   █  █   █ 
 *     ▀▄▄▄█▀ ▀█▄▄▀  █   █  ▀▄▄▄▀  ▀█▄█▀   █            █      ▀▄▄▀█  ▀▄▄▄▀  ▄▄█▄▄  ▀█▄█▀  █   █ 
 *                                                                                               
 *                                                                                               
 */

/************************* INCLUDE SECTION *************************/
#include <zephyr/kernel.h>
#include "workshop_protocol.h"
#include "console_lock.h"
#include "bms.h"
#if !defined(USE_HALO) || (USE_HALO == 0)
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

/************************* GLOBAL SECTION *************************/
#if !defined(USE_HALO) || (USE_HALO == 0)
    typedef struct SensorFrame {
        uint32_t battery_voltage_mv;
        int32_t charge_current_ma;
        float battery_temp_c;
        uint8_t breaker_closed;
        uint32_t fault_flags;
    } SensorFrame;
#endif

/************************* FUNCTION SECTION *************************/
int main( void )
{
#if !defined(USE_HALO) || (USE_HALO == 0)

/*
    * Workshop steps for implementing the safety monitor loop:
    ______________________________________________________________________________________________________________________________
    !!!!!!!!Shared Memory Note!!!!!!!
    In this application, we are using shared memory to communicate between the safety monitor and the peer. 
    This means that both the safety monitor and the peer will read and write to the same address in memory to exchange messages.

    MEM Adress Map:
    *   SensorFrame: Written by Sensor Fusion, read by Charging Controller
        Address: SENSOR_FRAME_BASE - ring buffer protocol
        Size: SENSOR_FRAME_SIZE (512 bytes) - ring buffer protocol
    ______________________________________________________________________________________________________________________________

    * 1. Define SensorFrame struct from Workshop specification -> SensorFrameIf and declare a variable of this type
        battery voltage (uint32_t battery_voltage_mv = 0)
        charge current (int32_t charge_current_ma = 0)
        battery temperature (float battery_temp_c = 0.0)
        breaker state (uint8_t breaker_closed = 0)
        fault/status flags (uint32_t fault_flags = 0)

    * 4. In a while loop in which the sensor fusion will run continuously:

        - Call BMS APIs to get the sensor data and fill the SensorFrame struct with the data. 
            -- Example:
                SensorFrame frame;
                frame.battery_voltage_mv = (uint32_t)(bms_get_voltage());
                frame.charge_current_ma = (int32_t)(bms_get_current());
                frame.battery_temp_c = bms_get_temperature();
                frame.breaker_closed = bms_get_breaker_closed();
                frame.fault_flags = bms_get_fault_flags();

        - Publish/log/send the SensorFrame to the peer using shared memory access (write to defined memory address for SensorFrame)
            -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.

        - Log the Info to the console using printk("[APP1] ") which is behaving similar to printf
            -- [APP1] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.

            -- Example log messages:
                Received command if mode changes:
                    printk( "[APP1] Charging mode=%s\n",
                            command.charging_mode );

                Every N cycles:
                    printk( "[APP1] sent mV=%u mA=%d temp=%f breaker_closed=%u faults=%d\n",
                            ( unsigned int ) frame.battery_voltage_mv,
                            frame.charge_current_ma,
                            frame.battery_temp_c,
                            ( unsigned int ) frame.breaker_closed,
                            ( unsigned int ) frame.fault_flags );

        - Use k_msleep( ms ) to create a delay in the loop
    */


    // Variables to read user command to BMS
    uint32_t bms_cmd_id = 0U;
    uint32_t bms_cmd_param = 0U;

    /* Declare a variable of type SensorFrame */
    SensorFrame sensor_frame;
    /* Declare a variable of type SensorFrame to hold the last sent sensor data for logging on change */
    uint32_t heartbeat_counter = 0U;

    // Init BMS controller
    bms_init();

    while( 1 )
    {
        // Read command for BMS from console and apply it to BMS
        bms_get_operator_command( &bms_cmd_id, &bms_cmd_param );
        // Apply operator command to BMS
        command_bms( bms_cmd_id, bms_cmd_param ); 

        /* This is a demo loop to showcase the application running */
        /* NOTE: This is for demonstration purposes only and should be deleted when workshop code is written */
        //classical_hello();


        /* Call BMS APIs to get the sensor data and fill the SensorFrame struct with the data. 
            -- Example:
                SensorFrame frame;
                frame.battery_voltage_mv = (unsigned int)(bms_get_voltage());
                frame.charge_current_ma = (int)(bms_get_current());
                frame.battery_temp_c = bms_get_temperature();
                frame.breaker_closed = bms_get_breaker_closed();
                frame.fault_flags = bms_get_fault_flags();
        */
        sensor_frame.battery_voltage_mv = (uint32_t)(bms_get_voltage());
        sensor_frame.charge_current_ma = (int32_t)(bms_get_current());
        sensor_frame.battery_temp_c = bms_get_temperature();
        sensor_frame.breaker_closed = bms_get_breaker_closed();
        sensor_frame.fault_flags = bms_get_fault_flags();


        /* Publish/log/send the SensorFrame to the peer using shared memory access (write to defined memory address for SensorFrame)
            -- Synchronization is important, so make sure to implement a simple protocol to signal when new data is available for the peer to read.
        */
        *( ( SensorFrame * ) SENSOR_FRAME_BASE ) = sensor_frame;

        /* Log the Info to the console using printk("[APP1] ") which is behaving similar to printf */
        if( ( heartbeat_counter % 10U ) == 0U || sensor_frame.fault_flags != 0U )
        {
            printk( "[APP1] CC sent mV=%u mA=%d temp=%f breaker_closed=%u faults=%d\n",
                       ( unsigned int ) sensor_frame.battery_voltage_mv,
                       sensor_frame.charge_current_ma,
                       sensor_frame.battery_temp_c,
                       ( unsigned int ) sensor_frame.breaker_closed,
                       ( unsigned int ) sensor_frame.fault_flags );
        }

        k_msleep( WORKSHOP_SENSOR_PERIOD_MS );
    }
#else


/*
    * Workshop steps for implementing the safety monitor loop:
    * 1. Declare a variable of type SensorFrame generated from HALO
      - SensorFrame struct is available from HALO generated code from deps/halo/codegen/riscv64_h1_zephyr/include/halo_structs.h

    * 2. In a while loop in which the sensor fusion will run continuously:

        - Call BMS APIs to get the sensor data and fill the SensorFrame struct with the data. 
            -- Example:
                SensorFrame frame;
                frame.battery_voltage_mv = (unsigned int)(bms_get_voltage());
                frame.charge_current_ma = (int)(bms_get_current());
                frame.battery_temp_c = bms_get_temperature();
                frame.breaker_closed = bms_get_breaker_closed();
                frame.fault_flags = bms_get_fault_flags();

        - Send the SensorFrame to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                printk( "[APP1] SensorFrame send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h1_zephyr/include/halo_api.h and deps/halo/codegen/riscv64_h1_zephyr/src/halo_channels.c

        - Log the Info to the console using printk("[APP1] ") which is behaving similar to printf
            -- [APP1] needs to be included in the log message to differentiate logs from other applications running on different harts
            -- Use logging on change to avoid flooding the console with repeated messages. 
            -- For example, only log when data changes, or every N cycles.

            -- Example log messages:
                Received command if mode changes:
                    printk( "[APP1] Charging mode=%s\n",
                            command.charging_mode );

                Every N cycles:
                    printk( "[APP1] sent mV=%u mA=%d temp=%f breaker_closed=%u faults=%d\n",
                            ( unsigned int ) frame.battery_voltage_mv,
                            frame.charge_current_ma,
                            frame.battery_temp_c,
                            ( unsigned int ) frame.breaker_closed,
                            ( unsigned int ) frame.fault_flags );

        - Use k_msleep( ms ) to create a delay in the loop
    */


    printk( "[APP1] SensorFusion HALO\n" );
    halo_zephyr_h1_init_riscv64_h1_zephyr();

    // Declare hearbeat_counter that increments on each loop iteration.
    uint32_t heartbeat_counter = 0U;
    // Variables to read user command to BMS
    uint32_t bms_cmd_id = 0U;
    uint32_t bms_cmd_param = 0U;

    /* 1. Declare a variable of type SensorFrame generated from HALO
      - SensorFrame struct is available from HALO generated code from deps/halo/codegen/riscv64_h1_zephyr/include/halo_structs.h
    */
    SensorFrame frame;

    // Init BMS controller
    bms_init();

    while( 1 )
    {

        // Read command for BMS from console and apply it to BMS
        bms_get_operator_command( &bms_cmd_id, &bms_cmd_param );
        // Apply operator command to BMS
        command_bms( bms_cmd_id, bms_cmd_param ); 

        /* Call BMS APIs to get the sensor data and fill the SensorFrame struct with the data. 
            -- Example:
                SensorFrame frame;
                frame.battery_voltage_mv = (unsigned int)(bms_get_voltage());
                frame.charge_current_ma = (int)(bms_get_current());
                frame.battery_temp_c = bms_get_temperature();
                frame.breaker_closed = bms_get_breaker_closed();
                frame.fault_flags = bms_get_fault_flags();
        */

        frame.battery_voltage_mv = (unsigned int)(bms_get_voltage());
        frame.charge_current_ma = (int)(bms_get_current());
        frame.battery_temp_c = bms_get_temperature();
        frame.breaker_closed = bms_get_breaker_closed();
        frame.fault_flags = bms_get_fault_flags();
 
        /* Send the SensorFrame to the peer using halo_send_ API functions defined in halo_api.h
            -- Check the return value of halo_send_ function to ensure the message was sent successfully if not sent log an error message    
                printk( "[APP1] SensorFrame send failed\n" );
            -- Full path is in .deps/halo/codegen/riscv64_h1_zephyr/include/halo_api.h and deps/halo/codegen/riscv64_h1_zephyr/src/halo_channels.c
        */
        if( halo_send_SensorFrameIf_SensorFrame( &frame ) != 0 )
        {
            printk( "[APP1] SensorTelemetry send failed\n" );
        }
        /* Log the Info to the console using printk("[APP1] ") which is behaving similar to printf */
        else if( ( heartbeat_counter % 10 ) == 0U)// || frame.fault_flags != 0U )
        {
            printk( "[APP1] sent mV=%u mA=%d temp=%f breaker_closed=%u faults=%d\n",
                       ( unsigned int ) frame.battery_voltage_mv,
                       frame.charge_current_ma,
                       frame.battery_temp_c,
                       ( unsigned int ) frame.breaker_closed,
                       ( unsigned int ) frame.fault_flags );
        }

        heartbeat_counter++;
        k_msleep( WORKSHOP_SENSOR_PERIOD_MS );
    }
#endif

    return 0;
}
