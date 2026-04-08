#ifndef VOLTAGE_ACQ_H
#define VOLTAGE_ACQ_H

#include <stdint.h>

/**
 * @file voltage_acq.h
 * @brief Voltage acquisition virtual driver for QEMU
 * @brief Simulates a voltage sensor (voltmeter)
 */

/**
 * @brief Voltage sensor data structure
 */
typedef struct {
    float voltage_rms;      /**< RMS voltage in Volts */
    float voltage_dc;       /**< DC component in Volts */
    float voltage_peak;     /**< Peak voltage in Volts */
    uint32_t sample_count;  /**< Number of samples acquired */
    uint32_t status;        /**< Sensor status flags */
} voltage_acq_t;

/**
 * @brief Initialize voltage acquisition sensor
 * @return 0 on success, negative on error
 */
int voltage_acq_init(void);

/**
 * @brief Read voltage measurement
 * @param[out] data Pointer to voltage acquisition structure
 * @return 0 on success, negative on error
 */
int voltage_acq_read(voltage_acq_t *data);

/**
 * @brief Enable voltage acquisition
 */
void voltage_acq_enable(void);

/**
 * @brief Disable voltage acquisition
 */
void voltage_acq_disable(void);

/**
 * @brief Reset sample counter
 */
void voltage_acq_reset(void);

/**
 * @brief Increase voltage offset
 * @param[in] delta Voltage increase in Volts
 */
void voltage_acq_increase(float delta);

/**
 * @brief Decrease voltage offset
 * @param[in] delta Voltage decrease in Volts
 */
void voltage_acq_decrease(float delta);

#endif /* VOLTAGE_ACQ_H */
