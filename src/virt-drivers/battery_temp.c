#include "battery_temp.h"
#include <string.h>

/**
 * @brief Static battery temperature sensor instance
 */
static struct {
    battery_temp_t data;
    int enabled;
    float base_temp;
    float temp_offset;
} battery_temp_state = {
    .data = {
        .temperature = 25.0f,
        .status = 0
    },
    .enabled = 0,
    .base_temp = 25.0f,
    .temp_offset = 0.0f
};

int battery_temp_init(void)
{
    memset(&battery_temp_state.data, 0, sizeof(battery_temp_t));
    battery_temp_state.data.temperature = 25.0f;
    battery_temp_state.data.status = 1;  /* Sensor ready */
    battery_temp_state.base_temp = 25.0f;
    battery_temp_state.enabled = 1;
    return 0;
}

int battery_temp_read(battery_temp_t *data)
{
    if (data == NULL) {
        return -1;
    }
    
    if (!battery_temp_state.enabled) {
        return -1;
    }
    
    /* Simulate temperature variation */
    static unsigned int counter = 0;
    float variation = ((counter % 100) - 50) * 0.1f;  /* -5 to +5 degree swing */
    battery_temp_state.data.temperature = battery_temp_state.base_temp + variation + battery_temp_state.temp_offset;
    counter++;

    memcpy(data, &battery_temp_state.data, sizeof(battery_temp_t));
    return 0;
}

void battery_temp_enable(void)
{
    battery_temp_state.enabled = 1;
    battery_temp_state.data.status = 1;
}

void battery_temp_disable(void)
{
    battery_temp_state.enabled = 0;
    battery_temp_state.data.status = 0;
}

void battery_temp_increase(float delta)
{
    battery_temp_state.temp_offset += delta;
}

void battery_temp_decrease(float delta)
{
    battery_temp_state.temp_offset -= delta;
}