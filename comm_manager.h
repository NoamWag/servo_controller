#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define COMM_MSG_SIZE 50
#define COMM_TX_PREFIX_BYTES 2
#define COMM_TX_PAYLOAD_BYTES (COMM_MSG_SIZE - COMM_TX_PREFIX_BYTES)

typedef enum {
    kReset = 0,
    kWaitRx,
    kFrameReady,
    kProcess,
    kInTx,
    kTxDone
} comm_manager_state_t;

void comm_manager_process(void);
void comm_manager_on_rx(uint8_t ch);

#endif