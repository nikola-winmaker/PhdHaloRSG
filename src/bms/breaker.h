#ifndef BREAKER_H
#define BREAKER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BREAKER_CLOSED = 0,
    BREAKER_OPEN = 1
} breaker_state_t;

// Breaker state structure
typedef struct {
    breaker_state_t state;
    float current_ma;
    uint32_t trip_count;
} breaker_model_t;

// Initialize breaker
void breaker_init(void);
// Simulate one tick (update state based on current)
void breaker_tick(float current_ma);
// Get current breaker state
void breaker_get_state(breaker_model_t *state);
// Trip the breaker
void breaker_trip(void);
// Reset the breaker
void breaker_reset(void);

#ifdef __cplusplus
}
#endif

#endif // BREAKER_H
