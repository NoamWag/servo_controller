#include "motor_controller.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

static motor_controller_history_t s_history = {0};

static void motor_controller_store_transition(bool went_up, uint32_t now_us)
{
    s_history.time_us[s_history.index] = now_us;
    s_history.edge_up[s_history.index] = went_up ? 1u : 0u;

    s_history.index = (s_history.index + 1u) % MOTOR_CONTROLLER_HISTORY_SIZE;

    if (s_history.num_filled < MOTOR_CONTROLLER_HISTORY_SIZE) {
        s_history.num_filled++;
    }
}

void motor_controller_gpio_isr(uint gpio, uint32_t events)
{
    uint32_t now_us = time_us_32();
    bool went_up = gpio_get(gpio);

    motor_controller_store_transition(went_up, now_us);
    gpio_acknowledge_irq(gpio, events);
}

void motor_controller_init(void)
{
    gpio_init(MOTOR_CONTROLLER_PIN);
    gpio_set_dir(MOTOR_CONTROLLER_PIN, GPIO_IN);
    gpio_disable_pulls(MOTOR_CONTROLLER_PIN);

    s_history.index = 0u;
    s_history.num_filled = 0u;
    for (uint8_t i = 0u; i < MOTOR_CONTROLLER_HISTORY_SIZE; i++) {
        s_history.time_us[i] = 0u;
        s_history.edge_up[i] = 0u;
    }

    gpio_set_irq_enabled_with_callback(MOTOR_CONTROLLER_PIN,
                                      GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                      true,
                                      &motor_controller_gpio_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void motor_controller_reset(void)
{
    s_history.index = 0u;
    s_history.num_filled = 0u;
    for (uint8_t i = 0u; i < MOTOR_CONTROLLER_HISTORY_SIZE; i++) {
        s_history.time_us[i] = 0u;
        s_history.edge_up[i] = 0u;
    }
}

void motor_controller_get_history(motor_controller_history_t *history)
{
    if (history == NULL) {
        return;
    }

    *history = s_history;
}
