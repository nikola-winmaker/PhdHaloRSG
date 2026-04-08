#include "current_acq.h"
#include <string.h>
#include <math.h>

/**
 * @brief Static current acquisition sensor instance
 */
static struct {
    current_acq_t data;
    int enabled;
    unsigned int sample_counter;
    float current_offset;
} current_acq_state = {
    .data = {
        .current_rms = 0.0f,
        .current_peak = 0.0f,
        .sample_count = 0,
        .status = 0
    },
    .enabled = 0,
    .sample_counter = 0,
    .current_offset = 0.0f
};

int current_acq_init(void)
{
    memset(&current_acq_state.data, 0, sizeof(current_acq_t));
    current_acq_state.data.status = 1;  /* Sensor ready */
    current_acq_state.enabled = 1;
    current_acq_state.sample_counter = 0;
    return 0;
}

int current_acq_read(current_acq_t *data)
{
    if (data == NULL) {
        return -1;
    }
    
    if (!current_acq_state.enabled) {
        return -1;
    }
    
    /* Simulate sinusoidal current waveform */
    static float phase = 0.0f;
    float peak_current = 50.0f;  /* 50A peak */
    float rms_current = peak_current / 1.414f;  /* RMS = Peak / sqrt(2) */
    
    /* Simulate AC current with noise */
    float instantaneous = peak_current * sinf(phase);
    float noise = ((current_acq_state.sample_counter % 10) - 5) * 0.5f;
    
    current_acq_state.data.current_rms = rms_current + (noise * 0.1f) + current_acq_state.current_offset;
    current_acq_state.data.current_peak = peak_current + current_acq_state.current_offset;
    current_acq_state.data.sample_count = current_acq_state.sample_counter;
    
    phase += 0.1f;  /* Advance phase */
    if (phase > 6.28f) {  /* 2*pi */
        phase = 0.0f;
    }
    
    current_acq_state.sample_counter++;
    
    memcpy(data, &current_acq_state.data, sizeof(current_acq_t));
    return 0;
}

void current_acq_enable(void)
{
    current_acq_state.enabled = 1;
    current_acq_state.data.status = 1;
}

void current_acq_disable(void)
{
    current_acq_state.enabled = 0;
    current_acq_state.data.status = 0;
}

void current_acq_reset(void)
{
    current_acq_state.sample_counter = 0;
    current_acq_state.data.sample_count = 0;
}

void current_acq_increase(float delta)
{
    current_acq_state.current_offset += delta;
}

void current_acq_decrease(float delta)
{
    current_acq_state.current_offset -= delta;
}