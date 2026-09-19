#include "motor_manager.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define MOTOR1_PWM_PIN 16
#define MOTOR1_DIR_A_PIN 17
#define MOTOR1_DIR_B_PIN 18

#define MOTOR2_PWM_PIN 19
#define MOTOR2_DIR_A_PIN 20
#define MOTOR2_DIR_B_PIN 21

#define MOTOR_PWM_WRAP 1000u

typedef enum {
    kMotorStateIdle = 0,
    kMotorStateInMove = 1
} eMotorState;

typedef struct {
    eMotorState state;
    eMotorDirection direction;
    uint16_t speed;
    bool needs_update;
} tMotorStateSlot;

static tMotorStateSlot s_motors[kMotorCount];

static uint16_t clamp_speed(uint16_t speed)
{
    if (speed > MOTOR_PWM_WRAP) {
        return MOTOR_PWM_WRAP;
    }
    return speed;
}

static void motor_apply_hw(eMotorId motor)
{
    if (motor >= kMotorCount) {
        return;
    }

    tMotorStateSlot *slot = &s_motors[motor];
    uint pwm_pin = (motor == kMotor1) ? MOTOR1_PWM_PIN : MOTOR2_PWM_PIN;
    uint dir_a_pin = (motor == kMotor1) ? MOTOR1_DIR_A_PIN : MOTOR2_DIR_A_PIN;
    uint dir_b_pin = (motor == kMotor1) ? MOTOR1_DIR_B_PIN : MOTOR2_DIR_B_PIN;

    if (slot->state == kMotorStateIdle || slot->speed == 0U) {
        gpio_put(dir_a_pin, 0);
        gpio_put(dir_b_pin, 0);
        pwm_set_gpio_level(pwm_pin, 0U);
        slot->speed = 0U;
        return;
    }

    if (slot->direction == kMotorDirectionForward) {
        gpio_put(dir_a_pin, 1);
        gpio_put(dir_b_pin, 0);
    } else {
        gpio_put(dir_a_pin, 0);
        gpio_put(dir_b_pin, 1);
    }

    pwm_set_gpio_level(pwm_pin, clamp_speed(slot->speed));
    slot->needs_update = false;
}

void motor_manager_init(void)
{
    gpio_set_function(MOTOR1_PWM_PIN, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR1_DIR_A_PIN, GPIO_FUNC_SIO);
    gpio_set_function(MOTOR1_DIR_B_PIN, GPIO_FUNC_SIO);
    gpio_set_function(MOTOR2_PWM_PIN, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR2_DIR_A_PIN, GPIO_FUNC_SIO);
    gpio_set_function(MOTOR2_DIR_B_PIN, GPIO_FUNC_SIO);

    gpio_set_dir(MOTOR1_DIR_A_PIN, GPIO_OUT);
    gpio_set_dir(MOTOR1_DIR_B_PIN, GPIO_OUT);
    gpio_set_dir(MOTOR2_DIR_A_PIN, GPIO_OUT);
    gpio_set_dir(MOTOR2_DIR_B_PIN, GPIO_OUT);

    gpio_put(MOTOR1_DIR_A_PIN, 0);
    gpio_put(MOTOR1_DIR_B_PIN, 0);
    gpio_put(MOTOR2_DIR_A_PIN, 0);
    gpio_put(MOTOR2_DIR_B_PIN, 0);

    uint slice1 = pwm_gpio_to_slice_num(MOTOR1_PWM_PIN);
    uint slice2 = pwm_gpio_to_slice_num(MOTOR2_PWM_PIN);

    pwm_config config1 = pwm_get_default_config();
    pwm_config config2 = pwm_get_default_config();

    pwm_config_set_clkdiv_int(&config1, 1);
    pwm_config_set_wrap(&config1, MOTOR_PWM_WRAP);
    pwm_config_set_clkdiv_int(&config2, 1);
    pwm_config_set_wrap(&config2, MOTOR_PWM_WRAP);

    pwm_init(slice1, &config1, true);
    pwm_init(slice2, &config2, true);

    pwm_set_gpio_level(MOTOR1_PWM_PIN, 0U);
    pwm_set_gpio_level(MOTOR2_PWM_PIN, 0U);

    for (int index = 0; index < kMotorCount; index++) {
        s_motors[index].state = kMotorStateIdle;
        s_motors[index].direction = kMotorDirectionForward;
        s_motors[index].speed = 0U;
        s_motors[index].needs_update = false;
    }
}

void motor_manager_process(void)
{
    for (int index = 0; index < kMotorCount; index++) {
        if (s_motors[index].needs_update) {
            motor_apply_hw((eMotorId)index);
        }
    }
}

void start_motor(eMotorId motor, eMotorDirection direction, uint16_t speed)
{
    if (motor >= kMotorCount) {
        return;
    }

    tMotorStateSlot *slot = &s_motors[motor];
    slot->direction = direction;
    slot->speed = clamp_speed(speed);

    if (slot->state != kMotorStateInMove) {
        slot->state = kMotorStateInMove;
    }

    slot->needs_update = true;
    motor_apply_hw(motor);
}

void stop_motor(eMotorId motor)
{
    if (motor >= kMotorCount) {
        return;
    }

    tMotorStateSlot *slot = &s_motors[motor];
    slot->state = kMotorStateIdle;
    slot->speed = 0U;
    slot->needs_update = true;
    motor_apply_hw(motor);
}
