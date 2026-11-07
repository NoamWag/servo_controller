#include <stdint.h>
#include "pico/stdlib.h"
#include "comm_manager.h"
#include "motor_manager.h"
#include "motor_controller.h"

int main(void)
{
    stdio_init_all();

    comm_manager_init();
    motor_manager_init();
    motor_controller_init();

    while (true) {
        comm_manager_process();
        motor_manager_process();
        tight_loop_contents();
    }
}