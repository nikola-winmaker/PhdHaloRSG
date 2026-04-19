#include "charge_ctrl.h"
#include "workshop_protocol.h"

static char mode_normal[] = "normal";
static char mode_fast[] = "fast";
static char state_idle[] = "idle";
static char state_charging[] = "charging";
static char state_fault[] = "fault";

typedef struct {
    uint32_t battery_voltage_mv;
    int32_t charge_current_ma;
    float battery_temp_c;
    uint8_t breaker_closed;
    uint32_t fault_flags;
} SensFrame;

typedef struct {
    uint32_t command_id;
    int32_t command_param;
} OpCommand;

typedef struct {
    uint8_t enable_charging;
    uint32_t current_limit_ma;
    uint32_t voltage_limit_mv;
    char charging_mode[7];
} ChargeCMD;

typedef struct {
    char charger_state[5];
    uint32_t requested_current_ma;
    uint32_t requested_voltage_mv;
    uint32_t fault_state;
} ChargeStat;

typedef struct
{
    uint8_t charging_requested;
    uint8_t fast_mode;
    uint32_t current_limit_ma;
    uint32_t voltage_limit_mv;
    uint32_t extra_faults;
    uint8_t have_sensor_frame;
} controller_state_t;

// Global controller state
static controller_state_t controller = {0};

static void memcpy_internal( void * dest, const void * src, size_t n )
{
    uint8_t * d = ( uint8_t * ) dest;
    const uint8_t * s = ( const uint8_t * ) src;

    for( size_t i = 0; i < n; i++ )
    {
        d[i] = s[i];
    }
}

// static void memset_internal( void * dest, uint8_t value, size_t n )
// {
//     uint8_t * d = ( uint8_t * ) dest;

//     for( size_t i = 0; i < n; i++ )
//     {
//         d[i] = value;
//     }
// }

void charge_controller_init( void )
{
    controller.current_limit_ma = WORKSHOP_NORMAL_CURRENT_LIMIT_MA;
    controller.voltage_limit_mv = WORKSHOP_NORMAL_VOLTAGE_LIMIT_MV;
}

uint32_t controller_faults_from_sensor( const void * SensorFrame )
{

    const SensFrame * sensorics = ( const SensFrame * ) SensorFrame;
    uint32_t faults = controller.extra_faults;
    int32_t current_ma;


    faults |= sensorics->fault_flags;

    if( sensorics->battery_temp_c > WORKSHOP_MAX_TEMP_C )
    {
        faults |= WORKSHOP_FAULT_OVER_TEMP;
    }

    current_ma = sensorics->charge_current_ma;
    if( current_ma < 0 )
    {
        current_ma = -current_ma;
    }

    if( ( uint32_t ) current_ma > WORKSHOP_MAX_CURRENT_MA )
    {
        faults |= WORKSHOP_FAULT_OVER_CURRENT;
    }

    if( sensorics->battery_voltage_mv > WORKSHOP_MAX_VOLTAGE_MV )
    {
        faults |= WORKSHOP_FAULT_OVER_VOLTAGE;
    }

    if( sensorics->battery_voltage_mv < WORKSHOP_MIN_VOLTAGE_MV )
    {
        faults |= WORKSHOP_FAULT_UNDER_VOLTAGE;
    }

    return faults;
}



void build_charge_outputs( const void * sensor_frame,
                                void * command,
                                void * status )
{

    const SensFrame * sensorics = ( const SensFrame * ) sensor_frame;
    ChargeCMD * charge_cmd = ( ChargeCMD * ) command;
    ChargeStat * charge_status = ( ChargeStat * ) status;

    uint32_t faults = controller_faults_from_sensor( sensorics );
    uint8_t enable_charging = 0U;

    if( controller.charging_requested != 0U && faults == 0U )
    {
        enable_charging = 1U;
    }

    charge_cmd->enable_charging = enable_charging;
    charge_cmd->current_limit_ma = enable_charging ? controller.current_limit_ma : 0U;
    charge_cmd->voltage_limit_mv = enable_charging ? controller.voltage_limit_mv : 0U;

    // charge_cmd->charging_mode = controller->fast_mode != 0U ? mode_fast : mode_normal;
    memcpy_internal( charge_cmd->charging_mode, controller.fast_mode != 0U ? mode_fast : mode_normal, sizeof( charge_cmd->charging_mode ) );

    // status->charger_state = faults != 0U ? state_fault :
    //     ( enable_charging != 0U ? state_charging : state_idle );
    memcpy_internal( charge_status->charger_state, faults != 0U ? state_fault :
        ( enable_charging != 0U ? state_charging : state_idle ), sizeof( charge_status->charger_state ) );

    charge_status->requested_current_ma = charge_cmd->current_limit_ma;
    charge_status->requested_voltage_mv = charge_cmd->voltage_limit_mv;
    charge_status->fault_state = faults;
}



void apply_operator_command(const void * command )
{

    const OpCommand * op_cmd = ( const OpCommand * ) command;

    switch( op_cmd->command_id )
    {
        case WORKSHOP_CMD_NOOP:
            break;

        case WORKSHOP_CMD_START:
            controller.charging_requested = 1U;
            break;

        case WORKSHOP_CMD_STOP:
            controller.charging_requested = 0U;
            break;

        case WORKSHOP_CMD_RESET:
            controller.charging_requested = 0U;
            controller.fast_mode = 0U;
            controller.current_limit_ma = WORKSHOP_NORMAL_CURRENT_LIMIT_MA;
            controller.voltage_limit_mv = WORKSHOP_NORMAL_VOLTAGE_LIMIT_MV;
            controller.extra_faults = 0U;
            break;

        case WORKSHOP_CMD_MODE_NORMAL:
            controller.fast_mode = 0U;
            controller.current_limit_ma = WORKSHOP_NORMAL_CURRENT_LIMIT_MA;
            controller.voltage_limit_mv = WORKSHOP_NORMAL_VOLTAGE_LIMIT_MV;
            break;

        case WORKSHOP_CMD_MODE_FAST:
            controller.fast_mode = 1U;
            controller.current_limit_ma = WORKSHOP_FAST_CURRENT_LIMIT_MA;
            controller.voltage_limit_mv = WORKSHOP_FAST_VOLTAGE_LIMIT_MV;
            break;

        case WORKSHOP_CMD_SET_CURRENT:
            if( op_cmd->command_param > 0 )
            {
                controller.current_limit_ma = ( uint32_t ) op_cmd->command_param;
            }
            break;

        case WORKSHOP_CMD_SET_VOLTAGE:
            if( op_cmd->command_param > 0 )
            {
                controller.voltage_limit_mv = ( uint32_t ) op_cmd->command_param;
            }
            break;

        default:
            controller.extra_faults |= WORKSHOP_STATUS_UNKNOWN_COMMAND;    
            break;
    }
}
