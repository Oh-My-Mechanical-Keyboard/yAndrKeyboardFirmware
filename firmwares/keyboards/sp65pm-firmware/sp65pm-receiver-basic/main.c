
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "nrf_drv_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "nrf_gzll.h"

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1                           /**< UART RX buffer size. */


#define RX_PIN_NUMBER  30
#define TX_PIN_NUMBER  0
#define CTS_PIN_NUMBER 23
#define RTS_PIN_NUMBER 27
#define HWFC           false


// Define payload length
#define TX_PAYLOAD_LENGTH 5 ///< 5 byte payload length

// ticks for inactive keyboard
#define INACTIVE 100000

// Binary printing
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '#' : '.'), \
  (byte & 0x40 ? '#' : '.'), \
  (byte & 0x20 ? '#' : '.'), \
  (byte & 0x10 ? '#' : '.'), \
  (byte & 0x08 ? '#' : '.'), \
  (byte & 0x04 ? '#' : '.'), \
  (byte & 0x02 ? '#' : '.'), \
  (byte & 0x01 ? '#' : '.')


// Data and acknowledgement payloads
static uint8_t data_payload_left[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host.
static uint8_t data_payload_right[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host.
static uint8_t data_payload_pad[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host.
static uint8_t ack_payload[TX_PAYLOAD_LENGTH];                   ///< Payload to attach to ACK sent to device.
// 发送给qmk芯片列的数据9+9+4+2
static uint8_t data_buffer[29];

// Debug helper variables
extern nrf_gzll_error_code_t nrf_gzll_error_code;   ///< Error code
static bool init_ok, enable_ok, push_ok, pop_ok, packet_received_left, packet_received_right, packet_received_pad;
uint32_t left_active = 0;
uint32_t right_active = 0;
uint32_t pad_active = 0;
uint8_t c;

static uint8_t channel_table[9]={4, 25, 42, 63, 77, 33, 11, 22, 44};

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}


int main(void)
{
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud1M
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);

    // Initialize Gazell
    nrf_gzll_init(NRF_GZLL_MODE_HOST);
    nrf_gzll_set_channel_table(channel_table,9);
    nrf_gzll_set_datarate(NRF_GZLL_DATARATE_1MBIT);
    nrf_gzll_set_timeslot_period(900);

    // Addressing
    nrf_gzll_set_base_address_0(0x01020304);
    nrf_gzll_set_base_address_1(0x05060708);

    // Load data into TX queue
    ack_payload[0] = 0x55;
    nrf_gzll_add_packet_to_tx_fifo(0, data_payload_left, TX_PAYLOAD_LENGTH);
    nrf_gzll_add_packet_to_tx_fifo(1, data_payload_right, TX_PAYLOAD_LENGTH);
    nrf_gzll_add_packet_to_tx_fifo(2, data_payload_pad, TX_PAYLOAD_LENGTH);

    // Enable Gazell to start sending over the air
    nrf_gzll_enable();

    // main loop
    while (true)
    {
        // detecting received packet from interupt, and unpacking
        // 把左边列的数据发出去
        if (packet_received_left)
        {
            packet_received_left = false;

            data_buffer[0] =  data_payload_left[0];
            data_buffer[1] =  data_payload_left[1];
            data_buffer[2] =  data_payload_left[2];
            data_buffer[3] =  data_payload_left[3];
            data_buffer[4] =  data_payload_left[4];
            data_buffer[5] =  data_payload_left[5];
            data_buffer[6] =  data_payload_left[6];
            data_buffer[7] =  data_payload_left[7];
            data_buffer[8] =  data_payload_left[8];

        }

        if (packet_received_right)
        {
            packet_received_right = false;
            data_buffer[9] =  data_payload_right[0];
            data_buffer[10] =  data_payload_right[1];
            data_buffer[11] =  data_payload_right[2];
            data_buffer[12] =  data_payload_right[3];
            data_buffer[13] =  data_payload_right[4];
            data_buffer[14] =  data_payload_right[5];
            data_buffer[15] =  data_payload_right[6];
            data_buffer[16] =  data_payload_right[7];
            data_buffer[17] =  data_payload_right[8];
        }


        if (packet_received_pad)
        {
            packet_received_pad = false;
            data_buffer[18] =  data_payload_pad[0];
            data_buffer[19] =  data_payload_pad[1];
            data_buffer[20] =  data_payload_pad[2];
            data_buffer[21] =  data_payload_pad[3];
            data_buffer[22] =  data_payload_pad[4];
            data_buffer[23] =  data_payload_pad[5];
            data_buffer[24] =  data_payload_pad[6];
            data_buffer[25] =  data_payload_pad[7];
            data_buffer[26] =  data_payload_pad[8];
            data_buffer[27] =  data_payload_pad[9];
            data_buffer[28] =  data_payload_pad[10];
        }

        // checking for a poll request from QMK
        if (app_uart_get(&c) == NRF_SUCCESS && c == 's')
        {
            // sending data to QMK, and an end byte
            // 把列数据发送出去
            nrf_drv_uart_tx(data_buffer, 29);
            // 把结束符号发送出去
            app_uart_put(0xE0);

            // debugging help, for printing keystates to a serial console
            /*
            for (uint8_t i = 0; i < 10; i++)
            {
                app_uart_put(data_buffer[i]);
            }
            printf(BYTE_TO_BINARY_PATTERN " " \
                   BYTE_TO_BINARY_PATTERN " " \
                   BYTE_TO_BINARY_PATTERN " " \
                   BYTE_TO_BINARY_PATTERN " " \
                   BYTE_TO_BINARY_PATTERN "\r\n", \
                   BYTE_TO_BINARY(data_payload_left[0]), \
                   BYTE_TO_BINARY(data_payload_left[1]), \
                   BYTE_TO_BINARY(data_payload_left[2]), \
                   BYTE_TO_BINARY(data_payload_left[3]), \
                   BYTE_TO_BINARY(data_payload_left[4]));
            nrf_delay_us(100);
            */
        }
        // allowing UART buffers to clear
        nrf_delay_us(10);

        // if no packets recieved from keyboards in a few seconds, assume either
        // out of range, or sleeping due to no keys pressed, update keystates to off
        left_active++;
        right_active++;
        if (left_active > INACTIVE)
        {
            data_buffer[0] = 0;
            data_buffer[1] = 0;
            data_buffer[2] = 0;
            data_buffer[3] = 0;
            data_buffer[4] = 0;
            data_buffer[5] = 0;
            data_buffer[6] = 0;
            data_buffer[7] = 0;
            data_buffer[8] = 0;
            left_active = 0;
        }
        if (right_active > INACTIVE)
        {
            data_buffer[9] = 0;
            data_buffer[10] = 0;
            data_buffer[11] = 0;
            data_buffer[12] = 0;
            data_buffer[13] = 0;
            data_buffer[14] = 0;
            data_buffer[15] = 0;
            data_buffer[16] = 0;
            data_buffer[17] = 0;
            right_active = 0;
        }
        if (pad_active > INACTIVE)
        {
            data_buffer[18] =  0;
            data_buffer[19] =  0;
            data_buffer[20] =  0;
            data_buffer[21] =  0;
            data_buffer[22] =  0;
            data_buffer[23] =  0;
            data_buffer[24] =  0;
            data_buffer[25] =  0;
            data_buffer[26] =  0;
            data_buffer[27] =  0;
            data_buffer[28] =  0;
            pad_active = 0;
        }
    }
}


// Callbacks not needed in this example.
void nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info) {}
void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info) {}
void nrf_gzll_disabled() {}

// If a data packet was received, identify half, and throw flag
void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{
    uint32_t data_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

    if (pipe == 0)
    {
        packet_received_left = true;
        left_active = 0;
        // Pop packet and write first byte of the payload to the GPIO port.
        nrf_gzll_fetch_packet_from_rx_fifo(pipe, data_payload_left, &data_payload_length);
    }
    else if (pipe == 1)
    {
        packet_received_right = true;
        right_active = 0;
        // Pop packet and write first byte of the payload to the GPIO port.
        nrf_gzll_fetch_packet_from_rx_fifo(pipe, data_payload_right, &data_payload_length);
    }
    else if (pipe == 2)
    {
        packet_received_pad = true;
        pad_active = 0;
        // Pop packet and write first byte of the payload to the GPIO port.
        nrf_gzll_fetch_packet_from_rx_fifo(pipe, data_payload_pad, &data_payload_length);
    }
    // not sure if required, I guess if enough packets are missed during blocking uart
    nrf_gzll_flush_rx_fifo(pipe);

    //load ACK payload into TX queue
    ack_payload[0] =  0x55;
    nrf_gzll_add_packet_to_tx_fifo(pipe, ack_payload, TX_PAYLOAD_LENGTH);
}
