#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"

void vAssertCalled( const char * file, int line )
{
    ( void ) file;
    ( void ) line;

    uart_log( "[APP2] assert failed\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}

void vApplicationMallocFailedHook( void )
{
    uart_log( "[APP2] malloc failed\n" );

    for( ;; )
    {
        __asm__ volatile ( "wfi" );
    }
}