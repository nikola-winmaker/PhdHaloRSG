#ifndef SAFETY_EVAL_H
#define SAFETY_EVAL_H

#include <stdint.h>

void evaluate_safety( const void * command_struct,
    void * safety_struct,
    uint32_t heartbeat_counter );

#endif
