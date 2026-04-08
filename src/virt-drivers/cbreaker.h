#ifndef CBREAKER_H
#define CBREAKER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file cbreaker.h
 * @brief Circuit breaker virtual driver for QEMU
 * @brief Simulates a circuit breaker device
 */

enum cbreaker_state {
    CBREAKER_CLOSED = 0,    /**< Circuit breaker closed */
    CBREAKER_OPEN = 1,      /**< Circuit breaker open */
    CBREAKER_FAULT = 2      /**< Circuit breaker in fault state */
};

/**
 * @brief Circuit breaker data structure
 */
typedef struct {
    enum cbreaker_state state;  /**< Current breaker state */
    float current;              /**< Current flowing through breaker (A) */
    uint32_t trip_count;        /**< Number of trips */
} cbreaker_t;

/**
 * @brief Initialize circuit breaker device
 * @return 0 on success, negative on error
 */
int cbreaker_init(void);

/**
 * @brief Read circuit breaker status
 * @param[out] data Pointer to circuit breaker structure
 * @return 0 on success, negative on error
 */
int cbreaker_read(cbreaker_t *data);

/**
 * @brief Close the circuit breaker
 * @return 0 on success, negative on error
 */
int cbreaker_close(void);

/**
 * @brief Open the circuit breaker
 * @return 0 on success, negative on error
 */
int cbreaker_open(void);

/**
 * @brief Reset circuit breaker after trip
 * @return 0 on success, negative on error
 */
int cbreaker_reset(void);

/**
 * @brief Increase circuit breaker current
 * @param[in] delta Current increase in Amperes
 */
void cbreaker_increase(float delta);

/**
 * @brief Decrease circuit breaker current
 * @param[in] delta Current decrease in Amperes
 */
void cbreaker_decrease(float delta);

#endif /* CBREAKER_H */
