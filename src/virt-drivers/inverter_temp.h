#ifndef INVERTER_TEMP_H
#define INVERTER_TEMP_H

#include <stdint.h>

/**
 * @file inverter_temp.h
 * @brief Inverter temperature virtual driver for QEMU
 * @brief Simulates an inverter temperature sensor
 */

/**
 * @brief Inverter temperature sensor data structure
 */
typedef struct {
    float temperature;      /**< Current temperature in Celsius */
    float temp_max;         /**< Maximum recorded temperature */
    float temp_min;         /**< Minimum recorded temperature */
    uint32_t status;        /**< Sensor status flags */
} inverter_temp_t;

/**
 * @brief Initialize inverter temperature sensor
 * @return 0 on success, negative on error
 */
int inverter_temp_init(void);

/**
 * @brief Read inverter temperature
 * @param[out] data Pointer to inverter temperature structure
 * @return 0 on success, negative on error
 */
int inverter_temp_read(inverter_temp_t *data);

/**
 * @brief Enable temperature monitoring
 */
void inverter_temp_enable(void);

/**
 * @brief Disable temperature monitoring
 */
void inverter_temp_disable(void);

/**
 * @brief Reset min/max temperature values
 */
void inverter_temp_reset_minmax(void);

/**
 * @brief Increase inverter temperature
 * @param[in] delta Temperature increase in Celsius
 */
void inverter_temp_increase(float delta);

/**
 * @brief Decrease inverter temperature
 * @param[in] delta Temperature decrease in Celsius
 */
void inverter_temp_decrease(float delta);

#endif /* INVERTER_TEMP_H */
