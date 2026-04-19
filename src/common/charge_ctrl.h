#ifndef CHARGE_CTRL_H
#define CHARGE_CTRL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Function prototypes for charge control functions

// Evaluate sensor data and determine faults
void charge_controller_init( void );
uint32_t controller_faults_from_sensor( const void * controller );
void build_charge_outputs( const void * sensor_frame,
                                void * command,
                                void * status );
void apply_operator_command( const void * command );

#endif /* CHARGE_CTRL_H */