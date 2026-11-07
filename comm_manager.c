#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "comm_manager.h"
#include "motor_manager.h"
#include "motor_controller.h"
#include "Version.h"
#include "pico/time.h"

// UART defines
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

static volatile uint32_t s_set_time_clock_count = 0;
static volatile uint8_t s_set_time_hr = 0;
static volatile uint8_t s_set_time_min = 0;
static volatile uint8_t s_set_time_secs = 0;
static volatile uint16_t s_set_time_msecs = 0;
static volatile uint32_t s_rx_clock_count = 0;

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

void on_uart1_rx_irq(void)
{
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = (uint8_t)uart_getc(UART_ID);
        comm_manager_on_rx(ch);
    }
}

void comm_manager_init(void)
{
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART1_IRQ, on_uart1_rx_irq);
    uart_set_irq_enables(UART_ID, true, false);
    irq_set_enabled(UART1_IRQ, true);
}

void comm_manager_on_rx(uint8_t ch)
{
    if (s_state != kWaitRx) {
        return;
    }

    s_rx_clock_count = time_us_32();
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


static void comm_manager_get_current_time(uint8_t *out_hr,
                                         uint8_t *out_min,
                                         uint8_t *out_secs,
                                         uint16_t *out_msecs)
{
    uint32_t current_clock_count = time_us_32();
    uint32_t elapsed_us = 0;

    if (current_clock_count < s_set_time_clock_count) {
        elapsed_us = (UINT32_MAX - s_set_time_clock_count) + current_clock_count + 1U;
    } else {
        elapsed_us = current_clock_count - s_set_time_clock_count;
    }

    uint32_t total_msecs = (uint32_t)s_set_time_msecs + (elapsed_us / 1000U);
    uint32_t total_secs = (uint32_t)s_set_time_secs + (total_msecs / 1000U);
    uint32_t total_mins = (uint32_t)s_set_time_min + (total_secs / 60U);
    uint32_t total_hours = (uint32_t)s_set_time_hr + (total_mins / 60U);

    *out_msecs = (uint16_t)(total_msecs % 1000U);
    *out_secs = (uint8_t)(total_secs % 60U);
    *out_min = (uint8_t)(total_mins % 60U);
    *out_hr = (uint8_t)(total_hours % 24U);
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
            snprintf(message_tx.payload.msg_string.text,
                     sizeof(message_tx.payload.msg_string.text),
                     "DSP version %u.%u",
                     VER_MAJOR,
                     VER_MINOR);
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kGetTime: {
            uint8_t get_time_hr = 0;
            uint8_t get_time_min = 0;
            uint8_t get_time_secs = 0;
            uint16_t get_time_msec = 0;

            if (s_set_time_clock_count != 0U) {
                comm_manager_get_current_time(&get_time_hr,
                                             &get_time_min,
                                             &get_time_secs,
                                             &get_time_msec);
            }

            memset(&message_tx, 0, sizeof(message_tx));
            message_tx.op = kString;
            snprintf(message_tx.payload.msg_string.text,
                     sizeof(message_tx.payload.msg_string.text),
                     "DSP time %02u:%02u:%02u.%03u",
                     get_time_hr,
                     get_time_min,
                     get_time_secs,
                     get_time_msec);
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kSetTime: {
            const tPcToDspMessageSetTime *msg_set_time =
                (const tPcToDspMessageSetTime *)&message_rx.payload.msg_set_time;

            s_set_time_clock_count = s_rx_clock_count;
            s_set_time_hr = msg_set_time->hr;
            s_set_time_min = msg_set_time->min;
            s_set_time_secs = msg_set_time->secs;
            s_set_time_msecs = msg_set_time->msecs;

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

        case kStartMotor: {
            const tPcToDspMessageStartMotor *msg_start_motor =
                (const tPcToDspMessageStartMotor *)&message_rx.payload.msg_start_motor;

            start_motor((eMotorId)msg_start_motor->motor,
                        (eMotorDirection)msg_start_motor->direction,
                        msg_start_motor->speed);
            message_tx.op = kAck;
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kStopMotor: {
            const tPcToDspMessageStopMotor *msg_stop_motor =
                (const tPcToDspMessageStopMotor *)&message_rx.payload.msg_stop_motor;

            stop_motor((eMotorId)msg_stop_motor->motor);
            message_tx.op = kAck;
            comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
            break;
        }

        case kGetMotorSensor: {
            const tPcToDspMessageGetMotorSensor *msg_get_motor_sensor =
                (const tPcToDspMessageGetMotorSensor *)&message_rx.payload.msg_get_motor_sensor;

            memset(&message_tx, 0, sizeof(message_tx));
            message_tx.op = kMotorSensor;

            if (msg_get_motor_sensor->motor != 0U) {
                message_tx.payload.msg_motor_sensor.count = 0U;
                comm_manager_prepare_tx(&message_tx, sizeof(message_tx));
                break;
            }

            motor_controller_history_t history = {0};
            motor_controller_get_history(&history);

            uint8_t count = (history.num_filled > MOTOR_CONTROLLER_HISTORY_SIZE)
                ? MOTOR_CONTROLLER_HISTORY_SIZE
                : history.num_filled;
            message_tx.payload.msg_motor_sensor.count = count;

            for (uint8_t i = 0U; i < count; ++i) {
                uint8_t src_index = (uint8_t)((history.index + MOTOR_CONTROLLER_HISTORY_SIZE - 1U - i)
                                            % MOTOR_CONTROLLER_HISTORY_SIZE);
                uint32_t sample_time = history.time_us[src_index];
                uint8_t sample_dir = history.edge_up[src_index];

                message_tx.payload.msg_motor_sensor.times[i] = sample_time;

                if (sample_dir != 0U) {
                    uint8_t bit_index = i % 8U;
                    uint8_t byte_index = i / 8U;
                    message_tx.payload.msg_motor_sensor.direction_bits[byte_index] |=
                        (uint8_t)(1U << bit_index);
                }
            }

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

