#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#define MOTOR_CONTROLLER_PIN 22u
#define MOTOR_CONTROLLER_HISTORY_SIZE 10u

typedef struct {
    uint32_t time_us[MOTOR_CONTROLLER_HISTORY_SIZE];
    uint8_t edge_up[MOTOR_CONTROLLER_HISTORY_SIZE];
    uint8_t index;
    uint8_t num_filled;
} motor_controller_history_t;

void motor_controller_init(void);
void motor_controller_reset(void);
void motor_controller_get_history(motor_controller_history_t *history);

#endif
