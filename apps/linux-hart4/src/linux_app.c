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

#define SHARED_MEM_BASE 0x80340000UL
#define SHARED_MEM_SIZE 0x1000UL
#define SHARED_MAGIC 0x48414c4fUL
#define SLOT_MAGIC 0x534c4f54UL
#define MAX_HARTS 5U

typedef struct
{
    uint32_t magic;
    uint32_t hart_id;
    uint32_t heartbeat;
    uint32_t flags;
    char name[ 16 ];
} shared_slot_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t ready_mask;
    uint32_t reserved;
    shared_slot_t slots[ MAX_HARTS ];
} shared_state_t;

static volatile sig_atomic_t keep_running = 1;

static void on_signal( int sig )
{
    ( void ) sig;
    keep_running = 0;
}

static void set_name( volatile shared_slot_t * slot, const char * name )
{
    size_t i = 0U;

    while( i < sizeof( slot->name ) - 1U && name[ i ] != '\0' )
    {
        slot->name[ i ] = name[ i ];
        i++;
    }

    while( i < sizeof( slot->name ) )
    {
        slot->name[ i++ ] = '\0';
    }
}

int main( void )
{
    int fd;
    void * map;
    volatile shared_state_t * state;
    uint32_t seen[ MAX_HARTS ] = { 0U };
    uint32_t heartbeat = 0U;

    signal( SIGINT, on_signal );
    signal( SIGTERM, on_signal );

    fd = open( "/dev/mem", O_RDWR | O_SYNC );
    if( fd < 0 )
    {
        fprintf( stderr, "[APP4] open(/dev/mem) failed: %s\n", strerror( errno ) );
        return 1;
    }

    map = mmap( NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SHARED_MEM_BASE );
    if( map == MAP_FAILED )
    {
        fprintf( stderr, "[APP4] mmap failed: %s\n", strerror( errno ) );
        close( fd );
        return 1;
    }

    state = ( volatile shared_state_t * ) map;
    printf( "[APP4] userspace app started\n" );
    fflush( stdout );

    while( keep_running )
    {
        volatile shared_slot_t * slot = &state->slots[ 0 ];

        state->magic = SHARED_MAGIC;
        state->version = 1U;
        slot->magic = SLOT_MAGIC;
        slot->hart_id = 0U;
        slot->heartbeat = heartbeat;
        set_name( slot, "APP4" );
        state->ready_mask |= 1U; 

        printf( "[APP4] heartbeat %u\n", heartbeat++ );

        for( uint32_t hart = 1U; hart < MAX_HARTS; ++hart )
        {
            volatile shared_slot_t * peer = &state->slots[ hart ];

            if( peer->magic == SLOT_MAGIC && peer->heartbeat != seen[ hart ] )
            {
                seen[ hart ] = peer->heartbeat;
                printf( "[APP4] saw hart%u (%s) heartbeat %u\n",
                        hart, peer->name, peer->heartbeat );
            }
        }

        fflush( stdout );
        sleep( 1 );
    }

    munmap( map, SHARED_MEM_SIZE );
    close( fd );
    return 0;
}
