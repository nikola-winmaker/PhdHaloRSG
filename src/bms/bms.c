#include "bms.h"
#include <string.h>
#include "config_bms.h"
#include <stdbool.h>

static bms_state_t bms_state = {0};

void bms_init(void) {
    breaker_init();
    memset(&bms_state, 0, sizeof(bms_state));
    bms_state.battery_voltage_mv = BATTERY_VOLTAGE_MIN;
    bms_state.charge_current_ma = 0.0f;
    bms_state.battery_temp_c = 25.0f;
    bms_state.breaker_closed = 0;
    bms_state.fault_flags = 0;
    bms_state.soc_percent = 0.0f;
}

void bms_tick(int charging_enabled) {
    // Simulate faults
    bms_state.fault_flags = 0;
    float temp_rise = 0.0f;
    breaker_model_t brk;
    int breaker_closed;

    // Simulate breaker logic for this tick
    if (charging_enabled) {
        breaker_tick((bms_state.current_limit_ma < BATTERY_OVERCURRENT) ? bms_state.current_limit_ma : BATTERY_OVERCURRENT);
    } else {
        breaker_tick(0.0f);
    }
    breaker_get_state(&brk);
    breaker_closed = (brk.state == BREAKER_CLOSED);
    if (charging_enabled && breaker_closed) {
        // Simulate current (avoid fminf)
        bms_state.charge_current_ma = (bms_state.current_limit_ma < BATTERY_OVERCURRENT) ? bms_state.current_limit_ma : BATTERY_OVERCURRENT;
        // Simulate voltage rise
        float dv = (bms_state.charge_current_ma / BATTERY_CAPACITY_MAH) * (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 0.001f;
        bms_state.battery_voltage_mv += dv;
        if (bms_state.battery_voltage_mv > bms_state.voltage_limit_mv) {
            bms_state.battery_voltage_mv = bms_state.voltage_limit_mv;
        }
        if (bms_state.battery_voltage_mv > BATTERY_VOLTAGE_MAX) {
            bms_state.battery_voltage_mv = BATTERY_VOLTAGE_MAX;
        }
        // Simulate SoC
        bms_state.soc_percent = (bms_state.battery_voltage_mv - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100.0f;
        if (bms_state.soc_percent > 100.0f) bms_state.soc_percent = 100.0f;
        // Simulate temperature rise
        temp_rise = bms_state.charge_current_ma * BATTERY_TEMP_RISE_PER_A;
        bms_state.battery_temp_c += temp_rise;
    } else {
        //bms_state.charge_current_ma = 0.0f;
        // Cool down
        if (bms_state.battery_temp_c > 25.0f) {
            bms_state.battery_temp_c -= BATTERY_TEMP_COOL_PER_TICK;
            if (bms_state.battery_temp_c < 25.0f) bms_state.battery_temp_c = 25.0f;
        }
    }
    // Faults
    if (bms_state.battery_temp_c > BATTERY_TEMP_MAX) {
        bms_state.fault_flags |= 0x01; // over-temp
        breaker_trip();
    }
    if (bms_state.charge_current_ma > BATTERY_OVERCURRENT) {
        bms_state.fault_flags |= 0x02; // over-current
        breaker_trip();
    }
    if (bms_state.battery_voltage_mv > BATTERY_OVERVOLTAGE) {
        bms_state.fault_flags |= 0x04; // over-voltage
        breaker_trip();
    }
    if (bms_state.battery_voltage_mv < BATTERY_UNDERVOLTAGE) {
        bms_state.fault_flags |= 0x08; // undervoltage
        breaker_trip();
    }
    // Simulate breaker
    breaker_get_state(&brk);
    bms_state.breaker_closed = (brk.state == BREAKER_CLOSED);
    if (!bms_state.breaker_closed) {
        bms_state.charge_current_ma = 0.0f;
    }
    // Update battery model with simulated values (if battery_set_state exists)
    battery_state_t bat;
    bat.voltage_mv = bms_state.battery_voltage_mv;
    bat.current_ma = bms_state.charge_current_ma;
    bat.temperature_c = bms_state.battery_temp_c;
    battery_set_state(&bat);
    // If you have similar setters for other models, call them here as needed.
}


void bms_get_state(bms_state_t *state) {
    if (state) {
        memcpy(state, &bms_state, sizeof(bms_state_t));
    }
}

void bms_inject_fault(unsigned int fault_mask) {
    bms_state.fault_flags |= fault_mask;
    if (fault_mask & 0x01) bms_state.breaker_closed = 0; // over-temp
    if (fault_mask & 0x02) bms_state.breaker_closed = 0; // over-current
    if (fault_mask & 0x04) bms_state.breaker_closed = 0; // over-voltage
    if (fault_mask & 0x08) bms_state.breaker_closed = 0; // undervoltage
    if (fault_mask & 0x10) bms_state.breaker_closed = 0; // breaker open
}

void bms_reset_faults(void) {
    bms_state.fault_flags = 0;
    bms_state.breaker_closed = 1;
}

void bms_set_state(const bms_state_t *state) {
    if (state) {
        memcpy(&bms_state, state, sizeof(bms_state_t));
    }
}

float bms_get_voltage(void) {
    return bms_state.battery_voltage_mv;
}

float bms_get_current(void) {
    return bms_state.charge_current_ma;
}

float bms_get_temperature(void) {
    return bms_state.battery_temp_c;
}

unsigned int bms_get_fault_flags(void) {
    return bms_state.fault_flags;
}

int bms_get_breaker_closed(void) {
    return bms_state.breaker_closed;
}

float bms_get_soc(void) {
    return bms_state.soc_percent;
}

float bms_voltage_to_soc(float voltage_mv) {
    float soc = (voltage_mv - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100.0f;
    if (soc < 0.0f) soc = 0.0f;
    if (soc > 100.0f) soc = 100.0f;
    return soc;
}

float bms_soc_to_voltage(float soc_percent) {
    if (soc_percent < 0.0f) soc_percent = 0.0f;
    if (soc_percent > 100.0f) soc_percent = 100.0f;
    return BATTERY_VOLTAGE_MIN + (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * (soc_percent / 100.0f);
}

void write_to_bms_command(unsigned char * g_halo_region, uint32_t command_id, uint32_t command_param){
    // Address the command to the BMS command handler
    *(uint32_t *)(g_halo_region + 0x5000) = command_id;
    *(uint32_t *)(g_halo_region + 0x5004) = command_param;
}

void bms_get_operator_command(uint32_t *command_id, uint32_t *command_param){
    // Read the latest command from the shared memory
    *command_id = *(uint32_t *)0x80345000;
    *command_param = *(uint32_t *)0x80345004;
}

void command_bms(uint32_t command_id, uint32_t command_param){
    // For simplicity, we directly apply the command here. In a real implementation, you might want to queue this or handle it in the main tick loop.
    switch (command_id) {
        case WORKSHOP_CMD_NOOP:
            break;
        case WORKSHOP_CMD_START:
            // Start charging with default limits
            if (bms_state.current_limit_ma == 0.0f) {
                bms_state.current_limit_ma = BMS_DEFAULT_CURRENT_LIMIT_MA;
            }
            if (bms_state.voltage_limit_mv == 0.0f) {
                bms_state.voltage_limit_mv = BMS_DEFAULT_VOLTAGE_LIMIT_MV;
            }
            bms_state.charging_enabled = true;
            bms_tick(bms_state.charging_enabled);
            break;
        case WORKSHOP_CMD_STOP:
            // Stop charging
            bms_state.charging_enabled = false;
            bms_tick(bms_state.charging_enabled);
            break;
        case WORKSHOP_CMD_RESET:
            bms_reset_faults();
            bms_state.battery_voltage_mv = BATTERY_VOLTAGE_MIN;
            bms_state.current_limit_ma = BMS_DEFAULT_CURRENT_LIMIT_MA;
            break;
        case WORKSHOP_CMD_MODE_NORMAL:
            // Set normal limits
            bms_state.current_limit_ma = WORKSHOP_NORMAL_CURRENT_LIMIT_MA;
            bms_state.voltage_limit_mv = WORKSHOP_NORMAL_VOLTAGE_LIMIT_MV;
            bms_tick(bms_state.charging_enabled);
            break;
        case WORKSHOP_CMD_MODE_FAST:
            // Set fast limits
            bms_state.current_limit_ma = WORKSHOP_FAST_CURRENT_LIMIT_MA;
            bms_state.voltage_limit_mv = WORKSHOP_FAST_VOLTAGE_LIMIT_MV;
            bms_tick(bms_state.charging_enabled);
            break;
        case WORKSHOP_CMD_SET_CURRENT:
            bms_state.current_limit_ma = (float)command_param;
            bms_tick(bms_state.charging_enabled);
            break;
        case WORKSHOP_CMD_SET_VOLTAGE:
            bms_state.voltage_limit_mv = (float)command_param;
            bms_tick(bms_state.charging_enabled);
            break;
        default:
            // Unknown command
            break;
    }
}
