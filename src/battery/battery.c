#include "battery.h"
#include <string.h>
#include <math.h>

#include "battery_config.h"

static battery_state_t battery_state;

void battery_init(void) {
    memset(&battery_state, 0, sizeof(battery_state));
    battery_state.voltage_mv = 12000.0f;
    battery_state.current_ma = 5000.0f;
    battery_state.temperature_c = 30.0f;
}

void battery_tick(void) {
    // Simulate temperature rise with current
    battery_state.temperature_c += battery_state.current_ma * BATTERY_TEMP_RISE_PER_A;
    if (battery_state.temperature_c > BATTERY_TEMP_MAX)
        battery_state.temperature_c = BATTERY_TEMP_MAX;
    // Cool down if not charging
    if (battery_state.current_ma == 0.0f && battery_state.temperature_c > 25.0f) {
        battery_state.temperature_c -= BATTERY_TEMP_COOL_PER_TICK;
        if (battery_state.temperature_c < 25.0f) battery_state.temperature_c = 25.0f;
    }
}

void battery_get_state(battery_state_t *state) {
    if (state) {
        memcpy(state, &battery_state, sizeof(battery_state_t));
    }
}

void battery_set_state(const battery_state_t *state) {
    if (state) {
        memcpy(&battery_state, state, sizeof(battery_state_t));
    }
}

// APIs to adjust battery temperature
void battery_increase_temp(float delta) {
    battery_state.temperature_c += delta;
}

void battery_decrease_temp(float delta) {
    battery_state.temperature_c -= delta;
}
