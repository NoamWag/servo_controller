#include <string.h>
#include "comm_manager.h"
#include "hardware/uart.h"
#include <string.h>

#define UART_ID uart1

typedef enum {
    kReset = 0,
    kWaitRx,
    kFrameReady,
    kProcess,
    kInTx,
    kTxDone
} comm_manager_state_t;

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

static void comm_manager_prepare_tx(const void *msg, uint8_t len)
{
    if (len > COMM_MSG_SIZE) {
        len = COMM_MSG_SIZE;
    }

    memcpy((void *)s_tx_buffer, msg, len);
    s_tx_len = len;

    // Do not set b_is_new_message here.
    // b_is_new_message is for RX, not TX.
}


static void process_new_messsage(void)
{
    // first 2 bytes as little-endian uint16_t
    tPcToDspMessage message_rx = *(tPcToDspMessage *)s_rx_buffer;
    ePcToDspCommOpCodes op_rx = (ePcToDspCommOpCodes)message_rx.op;
    tDspToPcMessage message_tx = *(tDspToPcMessage *)s_tx_buffer;

    switch (op_rx) {
        case kGetVer: {
            message_tx.op = kString;
            message_tx.payload.msg_ver.ver_major = 1;
            message_tx.payload.msg_ver.ver_minor = 0;
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kGetTime: {
            message_tx.op = kString;
            message_tx.payload.msg_time.hr = 0;
            message_tx.payload.msg_time.min = 0;
            message_tx.payload.msg_time.secs = 0;
            message_tx.payload.msg_time.msecs = 0;
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kSetTime: {
            message_tx.op = kAck;
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kEcho: {
            message_tx.op = kString;
            memcpy(message_tx.payload.msg_string.text,
                   message_rx.payload.msg_echo.text,
                   sizeof(message_rx.payload.msg_echo.text));
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        default:
            break;
    }
}


void comm_manager_process(void)
{
    switch (s_state) {
        case kReset:
            s_state = kWaitRx;
            break;

        case kWaitRx:
            if (b_is_new_message) {
                s_state = kProcess;             
                b_is_new_message = false;
            }
            break;
        
        case kProcess:
            process_new_messsage();
            s_state = kInTx; 
            break;

        case kInTx:
            uart_write_blocking(UART_ID, (const uint8_t *)s_tx_buffer, s_tx_len);
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

