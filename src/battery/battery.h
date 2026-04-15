#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include "battery_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Battery state structure
typedef struct {
    float voltage_mv;
    float current_ma;
    float temperature_c;
} battery_state_t;

// Initialize battery model
void battery_init(void);
// Simulate one tick (update state)
void battery_tick(void);
// Get current battery state
void battery_get_state(battery_state_t *state);
void battery_set_state(const battery_state_t *state);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_H
