#pragma once
#include "nrf_esb.h"

#define COMPILE_LEFT
// #define COMPILE_RIGHT

#define COLUMNS 9
#define DOBCOLUMNS 18
#define ROWS 5

#define KEYBOARD_SCAN_INTERVAL APP_TIMER_TICKS(1) // 1ms=1KHZ
#define KEYBOARD_KEEP_INTERVAL APP_TIMER_TICKS(300) // 300ms=3.3HZ

#define TX_PAYLOAD_LENGTH COLUMNS
#define ESB_MAX_PAYLOAD_LENGTH 32

#define DEBOUNCE 5
#define ACTIVITY 8000 // 降频扫描 8000ms

#define NUM_OF_74HC595 1

//                              a    b     c     d     e     f     g     h
#define SR_74HC595_LINE_MASK { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }
//                          c0    c1    c2    c3    c4    c5    c6    c7    c8
#define MATRIX_OF_74HC595 {{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00}}

#ifdef COMPILE_LEFT

#define PIPE_NUMBER 0

#define MATRIX_ROW_PINS {16, 6, 5, 18, 25}
#define HC595_SER 15
#define HC595_RCK 14
#define HC595_SCK 9

#define SPZ_RX_PIN 30
#define SPZ_TX_PIN 28

#define VBUS_READ 1
#define VBAT_READ 4

#define VBAT_AIN NRF_SAADC_INPUT_AIN2

#else

#define PIPE_NUMBER 1

#define MATRIX_ROW_PINS {28, 15, 14, 16, 25}
#define HC595_SER 6
#define HC595_RCK 4
#define HC595_SCK 1
#define R_COL8_PIN 30

#define SPZ_RX_PIN 18
#define SPZ_TX_PIN 20

#define VBUS_READ 9
#define VBAT_READ 5
#define VBAT_AIN NRF_SAADC_INPUT_AIN3

#endif

extern nrf_esb_payload_t tx_payload;

void sp_matrix_init(void);

