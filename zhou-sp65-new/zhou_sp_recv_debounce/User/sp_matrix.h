#pragma once
#include <stdint.h>
#include "nrf_esb.h"

#define COLUMNS 9
#define ROWS 5
#define DOBCOLUMNS 18

#define QMK_POLL_READY_FLAG 0x73 // 0x73

#define ESB_MAX_PAYLOAD_LENGTH 32

// Define payload length
#define TX_PAYLOAD_LENGTH ROWS // row nums

#define KEYBOARD_REPORT_INTERVAL APP_TIMER_TICKS(1) // 1ms=1KHZ

// ticks for inactive keyboard
#define INACTIVE 100000

#define SPZ_RECV_TX 9
#define SPZ_RECV_RX 10


void process_esb_matirx_data(nrf_esb_payload_t rxd);

void sp_matrix_init(void);

void qmk_poll_data_ready(uint8_t flag);
