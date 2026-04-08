#ifndef CURRENT_ACQ_H
#define CURRENT_ACQ_H

#include <stdint.h>

/**
 * @file current_acq.h
 * @brief Current acquisition virtual driver for QEMU
 * @brief Simulates a current sensor (ammeter)
 */

/**
 * @brief Current sensor data structure
 */
typedef struct {
    float current_rms;      /**< RMS current in Amperes */
    float current_peak;     /**< Peak current in Amperes */
    uint32_t sample_count;  /**< Number of samples acquired */
    uint32_t status;        /**< Sensor status flags */
} current_acq_t;

/**
 * @brief Initialize current acquisition sensor
 * @return 0 on success, negative on error
 */
int current_acq_init(void);

/**
 * @brief Read current measurement
 * @param[out] data Pointer to current acquisition structure
 * @return 0 on success, negative on error
 */
int current_acq_read(current_acq_t *data);

/**
 * @brief Enable current acquisition
 */
void current_acq_enable(void);

/**
 * @brief Disable current acquisition
 */
void current_acq_disable(void);

/**
 * @brief Reset sample counter
 */
void current_acq_reset(void);

/**
 * @brief Increase current offset
 * @param[in] delta Current increase in Amperes
 */
void current_acq_increase(float delta);

/**
 * @brief Decrease current offset
 * @param[in] delta Current decrease in Amperes
 */
void current_acq_decrease(float delta);

#endif /* CURRENT_ACQ_H */
