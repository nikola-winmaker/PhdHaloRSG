#ifndef BATTERY_TEMP_H
#define BATTERY_TEMP_H

#include <stdint.h>

/**
 * @file battery_temp.h
 * @brief Battery temperature virtual driver for QEMU
 * @brief Simulates a battery temperature sensor
 */

/**
 * @brief Battery temperature sensor data structure
 */
typedef struct {
    float temperature;      /**< Current temperature in Celsius */
    uint32_t status;        /**< Sensor status flags */
} battery_temp_t;

/**
 * @brief Initialize battery temperature sensor
 * @return 0 on success, negative on error
 */
int battery_temp_init(void);

/**
 * @brief Read battery temperature
 * @param[out] data Pointer to battery temperature structure
 * @return 0 on success, negative on error
 */
int battery_temp_read(battery_temp_t *data);

/**
 * @brief Enable battery temperature monitoring
 */
void battery_temp_enable(void);

/**
 * @brief Disable battery temperature monitoring
 */
void battery_temp_disable(void);

/**
 * @brief Increase battery temperature
 * @param[in] delta Temperature increase in Celsius
 */
void battery_temp_increase(float delta);

/**
 * @brief Decrease battery temperature
 * @param[in] delta Temperature decrease in Celsius
 */
void battery_temp_decrease(float delta);

#endif /* BATTERY_TEMP_H */
