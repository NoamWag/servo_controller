#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    kMotor1 = 0,
    kMotor2 = 1,
    kMotorCount = 2
} eMotorId;

typedef enum {
    kMotorDirectionForward = 0,
    kMotorDirectionReverse = 1
} eMotorDirection;

void motor_manager_init(void);
void motor_manager_process(void);
void start_motor(eMotorId motor, eMotorDirection direction, uint16_t speed);
void stop_motor(eMotorId motor);

#endif
