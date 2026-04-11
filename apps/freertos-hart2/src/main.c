#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "../../../src/shared_memory.h"

#if !defined(USE_HALO) || (USE_HALO == 0)
    #include "classical_api.h"
#else
    #include "halo_api.h"
#endif

static void uart_write_uint( uint32_t value )
{
    char buf[ 11 ];
    int i = 0;

    if( value == 0U )
    {
        uart_write_char( '0' );
        return;
    }

    while( value > 0U )
    {
        buf[ i++ ] = ( char ) ( '0' + ( value % 10U ) );
        value /= 10U;
    }

    while( i > 0 )
    {
        uart_write_char( buf[ --i ] );
    }
}

static void log_peer_heartbeat( uint32_t hart, const volatile halo_shared_slot_t * slot )
{
    uart_write_string( "[APP2] saw Hart" );
    uart_write_uint( hart );
    uart_write_string( " (" );
    uart_write_string( ( const char * ) slot->name );
    uart_write_string( ") heartbeat " );
    uart_write_uint( slot->heartbeat );
    uart_write_string( "\n" );
}

static void hello_task( void * parameters )
{
    uint32_t heartbeat = 0U;
    uint32_t seen[ HALO_MAX_HARTS ] = { 0 };

    ( void ) parameters;

    uart_write_string( "\n[APP2] FreeRTOS hello from Hart 2\n" );
    halo_shared_publish( 2U, "freertos", 0U );

    for( ;; )
    {
        halo_shared_publish( 2U, "freertos", heartbeat );
        uart_write_string( "[APP2] FreeRTOS tick " );
        uart_write_uint( heartbeat++ );
        uart_write_string( "\n" );

        for( uint32_t hart = 1U; hart < HALO_MAX_HARTS; ++hart )
        {
            volatile halo_shared_slot_t * slot;

            if( hart == 2U )
            {
                continue;
            }

            slot = halo_shared_slot( hart );

            if( slot->magic == HALO_SLOT_MAGIC && slot->heartbeat != seen[ hart ] )
            {
                seen[ hart ] = slot->heartbeat;
                log_peer_heartbeat( hart, slot );
            }
        }

        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
}

void vAssertCalled( const char * file, int line )
{
    ( void ) file;
    ( void ) line;

    uart_write_string( "[APP2] assert failed\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}

void vApplicationMallocFailedHook( void )
{
    uart_write_string( "[APP2] malloc failed\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}

int main( void )
{
    uart_init();
#if !defined( USE_HALO ) || ( USE_HALO == 0 )
    classical_hello();
#else
    uart_write_string( "\n[APP2] Starting FreeRTOS scheduler HALO\n" );

    // Initialize HALO protocol (if enabled) before starting scheduler
    halo_freertos_h2_init_riscv64_h2_freertos();
#endif


    if( xTaskCreate( hello_task, "hello", configMINIMAL_STACK_SIZE, NULL, 1, NULL ) != pdPASS )
    {
        uart_write_string( "[APP2] task create failed\n" );

        for( ;; )
        {
            __asm__ volatile ( "wfi" );
        }
    }

    vTaskStartScheduler();
    uart_write_string( "[APP2] scheduler exited unexpectedly\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}
