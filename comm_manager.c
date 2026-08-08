#include "comm_manager.h"
#include "hardware/uart.h"

#define UART_ID uart1

static comm_manager_state_t s_state = kReset;

static volatile uint8_t s_rx_buffer[COMM_MSG_SIZE];
static volatile uint8_t s_rx_index = 0;
static volatile bool b_is_new_message = false;

static volatile uint8_t s_tx_buffer[COMM_MSG_SIZE];
static volatile uint8_t s_tx_len = 0;

void comm_manager_on_rx(uint8_t ch)
{
    if (s_state != kWaitRx) {
        return;
    }

    s_rx_buffer[s_rx_index++] = ch;

    if (s_rx_index >= COMM_MSG_SIZE) {
        s_rx_index = 0;
        b_is_new_message = true;
    }
}

static void comm_manager_prepare_tx_from_rx(void)
{
    s_tx_buffer[0] = 0;
    s_tx_buffer[1] = 0;

    for (uint8_t i = 0; i < COMM_TX_PAYLOAD_BYTES; i++) {
        s_tx_buffer[COMM_TX_PREFIX_BYTES + i] = s_rx_buffer[i];
    }

    s_tx_len = COMM_MSG_SIZE;
}

void comm_manager_process(void)
{
    switch (s_state) {
        case kReset:
            s_state = kWaitRx;
            break;

        case kWaitRx:
            if (b_is_new_message) {
                b_is_new_message = false;
                comm_manager_prepare_tx_from_rx();
                s_state = kInTx;
            }
            break;

        case kInTx:
            uart_write_blocking(UART_ID, s_tx_buffer, s_tx_len);
            s_state = kTxDone;
            break;

        case kTxDone:
            s_state = kWaitRx;
            break;

        default:
            s_state = kReset;
            break;
    }
}