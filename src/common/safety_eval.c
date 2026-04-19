#include "safety_eval.h"
#include "workshop_protocol.h"

typedef struct
{
    uint8_t enable_charging;
    uint32_t current_limit_ma;
    uint32_t voltage_limit_mv;
    char charging_mode[7];
} generic_charge_command_t;

typedef struct
{
    uint8_t safe_mode;
    uint8_t breaker_open;
    uint8_t charging_allowed;
    uint32_t heartbeat_counter;
} generic_safety_state_t;

void evaluate_safety( const void * command_struct,
    void * safety_struct,
    uint32_t heartbeat_counter )
{
    const generic_charge_command_t * command = ( const generic_charge_command_t * ) command_struct;
    generic_safety_state_t * state = ( generic_safety_state_t * ) safety_struct;
    uint8_t unsafe_limits = 0U;

    if( command->current_limit_ma > WORKSHOP_SAFETY_MAX_CURRENT_MA )
    {
        unsafe_limits = 1U;
    }

    if( command->voltage_limit_mv > WORKSHOP_SAFETY_MAX_VOLTAGE_MV )
    {
        unsafe_limits = 1U;
    }

    state->safe_mode = unsafe_limits;
    state->breaker_open = ( unsafe_limits != 0U || command->enable_charging == 0U ) ? 1U : 0U;
    state->charging_allowed = ( unsafe_limits != 0U ) ? 0U : command->enable_charging;
    state->heartbeat_counter = heartbeat_counter;
}
