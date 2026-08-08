#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define COMM_MSG_SIZE 50
#define COMM_TX_PREFIX_BYTES 2
#define COMM_TX_PAYLOAD_BYTES (COMM_MSG_SIZE - COMM_TX_PREFIX_BYTES)

/* Operation codes for communication from PC to DSP */
typedef enum {
    kGetVer  = 0,
    kGetTime = 1,
    kSetTime = 257,
    kEcho    = 258,
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
    uint16_t op;
    union {
        uint8_t raw[COMM_MSG_SIZE - sizeof(uint16_t)];
        tPcToDspMessageGetVer  msg_get_ver;
        tPcToDspMessageGetTime msg_get_time;
        tPcToDspMessageSetTime msg_set_time;
        tPcToDspMessageEcho    msg_echo;
    } payload;
} tPcToDspMessage;

/* Operation codes for messages from DSP to PC */
typedef enum {
    kAck = 0,
    kString = 1,
} eDspToPcCommOpCodes;

/* Message structure for messages from DSP to PC */
typedef struct {
    uint8_t ver_major;
    uint8_t ver_minor;
} tDspToPcMessageVer;

typedef struct {
    uint8_t hr;
    uint8_t min;
    uint8_t secs;
    uint16_t msecs;
} tDspToPcMessageTime;

typedef struct {
    char text[COMM_MSG_SIZE - sizeof(uint16_t)];
} tDspToPcMessageString;

typedef struct {
    uint16_t op;
    union {
        uint8_t raw[COMM_MSG_SIZE - sizeof(uint16_t)];
        tDspToPcMessageVer    msg_ver;
        tDspToPcMessageTime   msg_time;
        tDspToPcMessageString msg_string;
    } payload;
} tDspToPcMessage;

void comm_manager_process(void);
void comm_manager_on_rx(uint8_t ch);

#endif