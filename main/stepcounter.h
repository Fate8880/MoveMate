// stepcounter.h
#ifndef STEPCOUNTER_H
#define STEPCOUNTER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"

// States
typedef enum {
    STATE_WALKING,
    STATE_WEAK,
    STATE_STRONG,
    STATE_IDLE,
    STATE_POTENTIAL_STEP,
    STATE_RUNNING
} movement_state_t;

// Prototype for the task
void step_counter_task(void *pvParameters);

#endif // STEPCOUNTER_H
