#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

#define configCPU_CLOCK_HZ                  ( ( uint64_t ) 10000000 )
#define configTICK_RATE_HZ                  ( ( TickType_t ) 100 )
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE             0
#define configMAX_PRIORITIES                5
#define configMINIMAL_STACK_SIZE            256
#define configMAX_TASK_NAME_LEN             16
#define configTICK_TYPE_WIDTH_IN_BITS       TICK_TYPE_WIDTH_64_BITS
#define configIDLE_SHOULD_YIELD             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configQUEUE_REGISTRY_SIZE           0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configUSE_MINI_LIST_ITEM            1
#define configSTACK_DEPTH_TYPE              size_t
#define configMESSAGE_BUFFER_LENGTH_TYPE    size_t
#define configHEAP_CLEAR_MEMORY_ON_FREE     1
#define configUSE_NEWLIB_REENTRANT          0

#define configUSE_TIMERS                    0
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         0
#define configUSE_COUNTING_SEMAPHORES       0
#define configUSE_TASK_NOTIFICATIONS        1

#define configSUPPORT_STATIC_ALLOCATION     0
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configTOTAL_HEAP_SIZE               ( ( size_t ) ( 64 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP    0

#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0
#define configCHECK_FOR_STACK_OVERFLOW      0
#define configUSE_MALLOC_FAILED_HOOK        1
#define configGENERATE_RUN_TIME_STATS       0

#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xTaskGetSchedulerState      1

#define configISR_STACK_SIZE_WORDS          256
#define configASSERT_DEFINED                1
#define configASSERT( x )                   do { if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ ); } while( 0 )

#define configENABLE_FPU                    0
#define configENABLE_VPU                    0

#define configMTIME_BASE_ADDRESS            0x0200BFF8ULL
#define configMTIMECMP_BASE_ADDRESS         0x02004000ULL

void vAssertCalled( const char * file, int line );

#endif
