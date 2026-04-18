#include "breaker.h"
#include <string.h>

#define BREAKER_TRIP_CURRENT 12000.0f

static breaker_model_t breaker_state;

void breaker_init(void) {
    memset(&breaker_state, 0, sizeof(breaker_state));
    breaker_state.state = BREAKER_OPEN;
    breaker_state.current_ma = 0.0f;
    breaker_state.trip_count = 0;
}

void breaker_tick(float current_ma) {
    breaker_state.current_ma = current_ma;
    if (current_ma > BREAKER_TRIP_CURRENT) {
        breaker_state.state = BREAKER_OPEN;
        breaker_state.trip_count++;
    } else if(current_ma == 0.0f) {
        breaker_state.state = BREAKER_OPEN; // Stay open if no current
    } else {
        breaker_state.state = BREAKER_CLOSED;
    }
}

void breaker_get_state(breaker_model_t *state) {
    if (state) {
        memcpy(state, &breaker_state, sizeof(breaker_model_t));
    }
}

void breaker_trip(void) {
    breaker_state.state = BREAKER_OPEN;
    breaker_state.trip_count++;
}

void breaker_reset(void) {
    breaker_state.state = BREAKER_OPEN;
    breaker_state.trip_count = 0;
}
