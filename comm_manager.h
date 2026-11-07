#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include "motor_controller.h"

#pragma pack(push, 1)

#define COMM_MSG_SIZE 50
#define COMM_TX_PREFIX_BYTES 2
#define COMM_PAYLOAD_MAX_BYTES (COMM_MSG_SIZE - sizeof(uint16_t))
#define COMM_TX_PAYLOAD_BYTES COMM_PAYLOAD_MAX_BYTES

/* Operation codes for communication from PC to DSP */
typedef enum {
    kGetVer          = 0,
    kGetTime         = 1,
    kSetTime         = 257,
    kEcho            = 258,
    kStartMotor      = 259,
    kStopMotor       = 260,
    kGetMotorSensor  = 261,
} ePcToDspCommOpCodes;

/* Message structure for messages from PC to DSP */
typedef struct {
    uint8_t stub;
} tPcToDspMessageGetVer;

typedef struct {
    uint8_t stub;
} tPcToDspMessageGetTime;

typedef struct {
    uint8_t hr;
    uint8_t min;
    uint8_t secs;
    uint16_t msecs;
} tPcToDspMessageSetTime;

typedef struct {
    char text[COMM_MSG_SIZE - sizeof(uint16_t)];
} tPcToDspMessageEcho;

typedef struct {
    uint8_t motor;
    uint8_t direction;
    uint16_t speed;
} tPcToDspMessageStartMotor;

typedef struct {
    uint8_t motor;
} tPcToDspMessageStopMotor;

typedef struct {
    uint8_t motor;
} tPcToDspMessageGetMotorSensor;

typedef struct {
    uint16_t op;
    union {
        uint8_t raw[COMM_MSG_SIZE - sizeof(uint16_t)];
        tPcToDspMessageGetVer          msg_get_ver;
        tPcToDspMessageGetTime         msg_get_time;
        tPcToDspMessageSetTime         msg_set_time;
        tPcToDspMessageEcho            msg_echo;
        tPcToDspMessageStartMotor      msg_start_motor;
        tPcToDspMessageStopMotor       msg_stop_motor;
        tPcToDspMessageGetMotorSensor  msg_get_motor_sensor;
    } payload;
} tPcToDspMessage;

/* Operation codes for messages from DSP to PC */
typedef enum {
    kAck = 0,
    kString = 1,
    kMotorSensor = 2,
} eDspToPcCommOpCodes;

typedef struct {
    char text[COMM_MSG_SIZE - sizeof(uint16_t)];
} tDspToPcMessageString;

typedef struct {
    uint8_t count;
    uint32_t times[MOTOR_CONTROLLER_HISTORY_SIZE];
    uint8_t direction_bits[(MOTOR_CONTROLLER_HISTORY_SIZE + 7U) / 8U];
} tDspToPcMessageMotorSensor;

typedef struct {
    uint16_t op;
    union {
        uint8_t raw[COMM_MSG_SIZE - sizeof(uint16_t)];
        tDspToPcMessageString      msg_string;
        tDspToPcMessageMotorSensor msg_motor_sensor;
    } payload;
} tDspToPcMessage;

static_assert(COMM_PAYLOAD_MAX_BYTES > 0U, "COMM payload must be positive");
static_assert(sizeof(tPcToDspMessageGetVer) <= COMM_PAYLOAD_MAX_BYTES, "GetVer payload too large");
static_assert(sizeof(tPcToDspMessageGetTime) <= COMM_PAYLOAD_MAX_BYTES, "GetTime payload too large");
static_assert(sizeof(tPcToDspMessageSetTime) <= COMM_PAYLOAD_MAX_BYTES, "SetTime payload too large");
static_assert(sizeof(tPcToDspMessageEcho) <= COMM_PAYLOAD_MAX_BYTES, "Echo payload too large");
static_assert(sizeof(tPcToDspMessageStartMotor) <= COMM_PAYLOAD_MAX_BYTES, "StartMotor payload too large");
static_assert(sizeof(tPcToDspMessageStopMotor) <= COMM_PAYLOAD_MAX_BYTES, "StopMotor payload too large");
static_assert(sizeof(tPcToDspMessageGetMotorSensor) <= COMM_PAYLOAD_MAX_BYTES, "GetMotorSensor payload too large");
static_assert(sizeof(tDspToPcMessageString) <= COMM_PAYLOAD_MAX_BYTES, "String payload too large");
static_assert(sizeof(tDspToPcMessageMotorSensor) <= COMM_PAYLOAD_MAX_BYTES, "MotorSensor payload too large");

void comm_manager_init(void);
void comm_manager_process(void);
void comm_manager_on_rx(uint8_t ch);

#pragma pack(pop)

#endif