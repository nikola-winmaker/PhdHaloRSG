#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdint.h>

#ifndef SHARED_MEM_BASE
    #define SHARED_MEM_BASE 0x80340000UL
#endif

#define HALO_SHARED_MAGIC 0x48414c4fUL
#define HALO_SLOT_MAGIC   0x534c4f54UL
#define HALO_SHARED_VERSION 1UL
#define HALO_MAX_HARTS    5U

typedef struct
{
    uint32_t magic;
    uint32_t hart_id;
    uint32_t heartbeat;
    uint32_t flags;
    char name[ 16 ];
} halo_shared_slot_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t ready_mask;
    uint32_t reserved;
    halo_shared_slot_t slots[ HALO_MAX_HARTS ];
} halo_shared_state_t;

#define HALO_SHARED_STATE ( ( volatile halo_shared_state_t * ) SHARED_MEM_BASE )

static inline volatile halo_shared_slot_t * halo_shared_slot( uint32_t hart_id )
{
    return &HALO_SHARED_STATE->slots[ hart_id ];
}

static inline void halo_shared_init_header( void )
{
    HALO_SHARED_STATE->magic = HALO_SHARED_MAGIC;
    HALO_SHARED_STATE->version = HALO_SHARED_VERSION;
}

static inline void halo_shared_set_name( volatile halo_shared_slot_t * slot, const char * name )
{
    uint32_t i = 0U;

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

static inline void halo_shared_publish( uint32_t hart_id, const char * name, uint32_t heartbeat )
{
    volatile halo_shared_slot_t * slot = halo_shared_slot( hart_id );

    halo_shared_init_header();
    slot->magic = HALO_SLOT_MAGIC;
    slot->hart_id = hart_id;
    slot->heartbeat = heartbeat;
    halo_shared_set_name( slot, name );
    HALO_SHARED_STATE->ready_mask |= ( 1UL << hart_id );
}

#endif
