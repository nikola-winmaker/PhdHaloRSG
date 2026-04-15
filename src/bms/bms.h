#ifndef BMS_H
#define BMS_H

#include "breaker.h"
#include "../battery/battery.h" // Battery model parameters shared with BMS

#ifdef __cplusplus
extern "C" {
#endif

// BMS state structure
typedef struct {
    float battery_voltage_mv;
    float charge_current_ma;
    float battery_temp_c;
    int breaker_closed;
    unsigned int fault_flags;
    float soc_percent; // State of charge
} bms_state_t;


// Initialize BMS and all virtual drivers
void bms_init(void);
// Simulate one tick (e.g., 100ms)
void bms_tick(const int charging_enabled, const float current_limit_ma, const float voltage_limit_mv);
// Get current BMS state
void bms_get_state(bms_state_t *state);

// Fault injection (bitmask: see system_specification.html)
void bms_inject_fault(unsigned int fault_mask);
// Reset all faults and breaker
void bms_reset_faults(void);
// Set battery state (for test/scenario control)
void bms_set_state(const bms_state_t *state);
// Get individual sensor values
float bms_get_voltage(void);
float bms_get_current(void);
float bms_get_temperature(void);
unsigned int bms_get_fault_flags(void);
int bms_get_breaker_closed(void);
float bms_get_soc(void);
// Map voltage to SoC and vice versa
float bms_voltage_to_soc(float voltage_mv);
float bms_soc_to_voltage(float soc_percent);

#ifdef __cplusplus
}
#endif

#endif // BMS_H
