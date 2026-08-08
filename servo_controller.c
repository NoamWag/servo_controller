#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "comm_manager.h"

// UART defines
#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_TX_PIN 4
#define UART_RX_PIN 5

void on_uart1_rx_irq(void)
{
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = (uint8_t)uart_getc(UART_ID);
        comm_manager_on_rx(ch);
    }
}

int main(void)
{
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART1_IRQ, on_uart1_rx_irq);
    uart_set_irq_enables(UART_ID, true, false);
    irq_set_enabled(UART1_IRQ, true);

    while (true) {
        comm_manager_process();
        tight_loop_contents();
    }
}