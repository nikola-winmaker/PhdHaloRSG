#include "voltage_acq.h"
#include <string.h>
#include <math.h>

/**
 * @brief Static voltage acquisition sensor instance
 */
static struct {
    voltage_acq_t data;
    int enabled;
    unsigned int sample_counter;
    float voltage_offset;
} voltage_acq_state = {
    .data = {
        .voltage_rms = 0.0f,
        .voltage_dc = 0.0f,
        .voltage_peak = 0.0f,
        .sample_count = 0,
        .status = 0
    },
    .enabled = 0,
    .sample_counter = 0,
    .voltage_offset = 0.0f
};

int voltage_acq_init(void)
{
    memset(&voltage_acq_state.data, 0, sizeof(voltage_acq_t));
    voltage_acq_state.data.voltage_dc = 400.0f;  /* 400V DC bus */
    voltage_acq_state.data.status = 1;  /* Sensor ready */
    voltage_acq_state.enabled = 1;
    voltage_acq_state.sample_counter = 0;
    return 0;
}

int voltage_acq_read(voltage_acq_t *data)
{
    if (data == NULL) {
        return -1;
    }
    
    if (!voltage_acq_state.enabled) {
        return -1;
    }
    
    /* Simulate three-phase AC voltage waveform */
    static float phase = 0.0f;
    float peak_voltage = 230.0f * 1.414f;  /* Peak from RMS */
    float rms_voltage = 230.0f;
    
    /* Simulate AC voltage with ripple */
    float instantaneous = peak_voltage * sinf(phase);
    float ripple = ((voltage_acq_state.sample_counter % 20) - 10) * 2.0f;  /* ±10V ripple */
    
    voltage_acq_state.data.voltage_rms = rms_voltage + (ripple * 0.05f) + voltage_acq_state.voltage_offset;
    voltage_acq_state.data.voltage_peak = peak_voltage + voltage_acq_state.voltage_offset;
    voltage_acq_state.data.voltage_dc = 400.0f + voltage_acq_state.voltage_offset;  /* Regulated DC bus */
    voltage_acq_state.data.sample_count = voltage_acq_state.sample_counter;
    
    phase += 0.1f;  /* Advance phase */
    if (phase > 6.28f) {  /* 2*pi */
        phase = 0.0f;
    }
    
    voltage_acq_state.sample_counter++;
    
    memcpy(data, &voltage_acq_state.data, sizeof(voltage_acq_t));
    return 0;
}

void voltage_acq_enable(void)
{
    voltage_acq_state.enabled = 1;
    voltage_acq_state.data.status = 1;
}

void voltage_acq_disable(void)
{
    voltage_acq_state.enabled = 0;
    voltage_acq_state.data.status = 0;
}

void voltage_acq_reset(void)
{
    voltage_acq_state.sample_counter = 0;
    voltage_acq_state.data.sample_count = 0;
}

void voltage_acq_increase(float delta)
{
    voltage_acq_state.voltage_offset += delta;
}

void voltage_acq_decrease(float delta)
{
    voltage_acq_state.voltage_offset -= delta;
}