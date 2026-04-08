#include "inverter_temp.h"
#include <string.h>

/**
 * @brief Static inverter temperature sensor instance
 */
static struct {
    inverter_temp_t data;
    int enabled;
    float base_temp;
    float temp_offset;
} inverter_temp_state = {
    .data = {
        .temperature = 30.0f,
        .temp_max = 30.0f,
        .temp_min = 30.0f,
        .status = 0
    },
    .enabled = 0,
    .base_temp = 30.0f,
    .temp_offset = 0.0f
};

int inverter_temp_init(void)
{
    memset(&inverter_temp_state.data, 0, sizeof(inverter_temp_t));
    inverter_temp_state.data.temperature = 30.0f;
    inverter_temp_state.data.temp_max = 30.0f;
    inverter_temp_state.data.temp_min = 30.0f;
    inverter_temp_state.data.status = 1;  /* Sensor ready */
    inverter_temp_state.base_temp = 30.0f;
    inverter_temp_state.enabled = 1;
    return 0;
}

int inverter_temp_read(inverter_temp_t *data)
{
    if (data == NULL) {
        return -1;
    }
    
    if (!inverter_temp_state.enabled) {
        return -1;
    }
    
    /* Simulate temperature variation with thermal inertia */
    static unsigned int counter = 0;
    float load_factor = ((counter % 150) - 75) * 0.05f;  /* Simulates load cycles */
    float temp_rise = load_factor * 20.0f;  /* Up to 20 degree rise from load */
    
    inverter_temp_state.data.temperature = inverter_temp_state.base_temp + temp_rise + inverter_temp_state.temp_offset;
    
    /* Track min/max */
    if (inverter_temp_state.data.temperature > inverter_temp_state.data.temp_max) {
        inverter_temp_state.data.temp_max = inverter_temp_state.data.temperature;
    }
    if (inverter_temp_state.data.temperature < inverter_temp_state.data.temp_min) {
        inverter_temp_state.data.temp_min = inverter_temp_state.data.temperature;
    }
    
    counter++;
    
    memcpy(data, &inverter_temp_state.data, sizeof(inverter_temp_t));
    return 0;
}

void inverter_temp_enable(void)
{
    inverter_temp_state.enabled = 1;
    inverter_temp_state.data.status = 1;
}

void inverter_temp_disable(void)
{
    inverter_temp_state.enabled = 0;
    inverter_temp_state.data.status = 0;
}

void inverter_temp_reset_minmax(void)
{
    inverter_temp_state.data.temp_max = inverter_temp_state.data.temperature;
    inverter_temp_state.data.temp_min = inverter_temp_state.data.temperature;
}

void inverter_temp_increase(float delta)
{
    inverter_temp_state.temp_offset += delta;
}

void inverter_temp_decrease(float delta)
{
    inverter_temp_state.temp_offset -= delta;
}