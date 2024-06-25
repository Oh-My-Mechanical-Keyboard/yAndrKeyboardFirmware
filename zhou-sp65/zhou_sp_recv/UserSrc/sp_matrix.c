#include "sp_matrix.h"

#include "nrf_esb.h"
#include "nrf_log.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "app_fifo.h"
#include "app_uart.h"
#include "nrf_assert.h"
#include "nrf_uart.h"

APP_TIMER_DEF(m_report_timer_id); /**< Report timer. */

// Data and acknowledgement payloads
static uint8_t data_payload_left[ESB_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host.
static uint8_t data_payload_right[ESB_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host.
static uint8_t ack_payload[TX_PAYLOAD_LENGTH];                   ///< Payload to attach to ACK sent to device.
// 发送给qmk芯片列的数据
static uint8_t data_buffer[DOBCOLUMNS+4];
// 状态记录
static bool qmk_poll_ok, packet_received_left, packet_received_right;
uint32_t left_active = 0;
uint32_t right_active = 0;

void process_esb_matirx_data(nrf_esb_payload_t rxd) {
    NRF_LOG_INFO("RXD: len=%d, pipe=%d, rssi=%d, noack=%d, pid=%d", rxd.length, rxd.pipe, rxd.rssi, rxd.noack, rxd.pid);
    switch (rxd.pipe) {
        case 0:
            packet_received_left = true;
            left_active = 0;
            memcpy(data_payload_left, rxd.data, rxd.length);
            break;
        case 1:
            packet_received_right = true;
            right_active = 0;
            memcpy(data_payload_right, rxd.data, rxd.length);
        default:
            
            break;
    }
}

void send_mx_data(void) {
    app_uart_flush();
    app_uart_put(0xff);  // head
    app_uart_put(DOBCOLUMNS+4+1);  // len

    app_uart_put(0x08);             // cmd:8
    // 把列数据发送出去
    for (uint8_t i = 0; i < DOBCOLUMNS+4; ++i) {
        app_uart_put(data_buffer[i]); // data:i
    }
    NRF_LOG_INFO("LADC: %d\n", data_buffer[COLUMNS]<<8|data_buffer[COLUMNS+1]);
    NRF_LOG_INFO("RADC: %d\n", data_buffer[DOBCOLUMNS+2]<<8|data_buffer[DOBCOLUMNS+3]);
    app_uart_put(0xfe);  // tail
}

void sp_matrix_task(void) {
    // detecting received packet from interupt, and unpacking
    // 把左边列的数据发出去
    if (packet_received_left)
    {
        packet_received_left = false;
        for (uint8_t i = 0; i < COLUMNS+2; ++i) {
            data_buffer[i] = data_payload_left[i];
        }
    }

    if (packet_received_right)
    {
        packet_received_right = false;
        for (uint8_t i = 0; i < COLUMNS+2; ++i) {
            data_buffer[i+COLUMNS+2] = data_payload_right[i];
        }
    }

    // checking for a poll request from QMK
    if (qmk_poll_ok)
    {
        qmk_poll_ok = false;
        // NRF_LOG_INFO("qmk_poll_ok\n");
        nrf_delay_us(10);
        send_mx_data();
        // allowing UART buffers to clear
        nrf_delay_us(10);

    }

    // if no packets recieved from keyboards in a few seconds, assume either
    // out of range, or sleeping due to no keys pressed, update keystates to off
    left_active++;
    right_active++;
    if (left_active > INACTIVE)
    {
        for (uint8_t i = 0; i < COLUMNS; ++i) {
            data_buffer[i] = 0;
        }
        left_active = 0;
    }
    if (right_active > INACTIVE)
    {
        for (uint8_t i = 0; i < COLUMNS; ++i) {
            data_buffer[i+COLUMNS] = 0;
        }
        right_active = 0;
    }
}

void qmk_poll_data_ready(uint8_t flag) {
    if (flag == QMK_POLL_READY_FLAG) {
        qmk_poll_ok = true;
    } else {
        qmk_poll_ok = false;
    }
}

/* 记录超时的次数 */
static void keyboard_report_timeout_handler(void *p_context) {
    UNUSED_PARAMETER(p_context);
    sp_matrix_task();
}

void sp_matrix_init(void) {
    // init report timer
    ret_code_t err_code = app_timer_create(&m_report_timer_id, APP_TIMER_MODE_REPEATED, keyboard_report_timeout_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(m_report_timer_id, KEYBOARD_REPORT_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}