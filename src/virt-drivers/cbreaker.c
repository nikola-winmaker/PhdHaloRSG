#include "cbreaker.h"
#include <string.h>

/**
 * @brief Static circuit breaker instance
 */
static struct {
    cbreaker_t data;
    int enabled;
    float current_threshold;
    float current_offset;
} cbreaker_state = {
    .data = {
        .state = CBREAKER_CLOSED,
        .current = 0.0f,
        .trip_count = 0
    },
    .enabled = 1,
    .current_threshold = 100.0f,  /* Threshold in Amperes */
    .current_offset = 0.0f
};

int cbreaker_init(void)
{
    memset(&cbreaker_state.data, 0, sizeof(cbreaker_t));
    cbreaker_state.data.state = CBREAKER_CLOSED;
    cbreaker_state.data.current = 0.0f;
    cbreaker_state.data.trip_count = 0;
    cbreaker_state.enabled = 1;
    return 0;
}

int cbreaker_read(cbreaker_t *data)
{
    if (data == NULL) {
        return -1;
    }
    
    if (!cbreaker_state.enabled) {
        return -1;
    }
    
    /* Simulate current variation */
    static unsigned int counter = 0;
    float variation = ((counter % 80) - 40) * 0.5f;  /* -20 to +20A swing */
    cbreaker_state.data.current = 50.0f + variation + cbreaker_state.current_offset;
    
    /* Check for overcurrent condition */
    if (cbreaker_state.data.current > cbreaker_state.current_threshold) {
        if (cbreaker_state.data.state == CBREAKER_CLOSED) {
            cbreaker_state.data.state = CBREAKER_FAULT;
            cbreaker_state.data.trip_count++;
        }
    }
    
    counter++;
    
    memcpy(data, &cbreaker_state.data, sizeof(cbreaker_t));
    return 0;
}

int cbreaker_close(void)
{
    if (cbreaker_state.data.state != CBREAKER_FAULT) {
        cbreaker_state.data.state = CBREAKER_CLOSED;
        return 0;
    }
    return -1;  /* Cannot close if in fault state */
}

int cbreaker_open(void)
{
    cbreaker_state.data.state = CBREAKER_OPEN;
    cbreaker_state.data.current = 0.0f;
    return 0;
}

int cbreaker_reset(void)
{
    cbreaker_state.data.state = CBREAKER_CLOSED;
    cbreaker_state.data.current = 0.0f;
    cbreaker_state.data.trip_count = 0;
    return 0;
}

void cbreaker_increase(float delta)
{
    cbreaker_state.current_offset += delta;
}

void cbreaker_decrease(float delta)
{
    cbreaker_state.current_offset -= delta;
}