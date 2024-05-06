#include "common_cfg.h"
#include "fds_my.h"

#include "uart_distributor.h"
#include "app_fifo.h"
#include "app_uart.h"
#include "nrf_assert.h"
#include "nrf_uart.h"
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "boards.h"

#define UART_TX_BUF_SIZE 128 /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256 /**< UART RX buffer size. */

#define MAX_DATA_PAYLOAD 64 /**< UART Transimit Max data len */

uint8_t rx_data[MAX_DATA_PAYLOAD];
volatile uint8_t rx_pkg_s;
volatile uint8_t rx_pkg_i;
volatile uint8_t rx_pkg_len;
volatile uint8_t cmd_i;

/*
cmd_i  name               cmd_len
0      get add0           0
1      set add0           4
2      get add1           0
3      set add1           4
4      get prefix         0
5      set prefix         8
6      get channel        0
7      set channel        1
8      test               0
*/
#define CMD_MAX_I 8

#define CMD_NUM (CMD_MAX_I+1)

enum rx_cmd {
    get_add0 = 0,
    set_add0 = 1,
    get_add1 = 2,
    set_add1 = 3,
    get_perfix = 4,
    set_perfix = 5,
    get_channel = 6,
    set_channel = 7,
    test_cmd = 8
};

uint8_t cmd_lens[CMD_NUM] = {0,4,0,4,0,8,0,1,0};

static void return_add0_to_ch552(void) {
    app_uart_flush();
    app_uart_put(0xff);  // head
    app_uart_put(5);     // len

    app_uart_put(0x00);                // cmd:0
    app_uart_put(esb_fds.addr_0[0]);  // data:0
    app_uart_put(esb_fds.addr_0[1]);  // data:1
    app_uart_put(esb_fds.addr_0[2]);  // data:2
    app_uart_put(esb_fds.addr_0[3]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void set_add0_from_ch552(void) {
    app_uart_flush();
    uint8_t need_update_fds = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        NRF_LOG_INFO("set_add0_from_ch552 d[%d] = %d", i, rx_data[i]);
        if (rx_data[i] != esb_fds.addr_0[i]) {
            esb_fds.addr_0[i] = rx_data[i];
            need_update_fds = 1;
        }
    }
    if (need_update_fds) {
        fds_update_esb_data();
    }

    app_uart_put(0xff);  // head
    app_uart_put(5);     // len

    app_uart_put(0x01);                // cmd:1
    app_uart_put(esb_fds.addr_0[0]);  // data:0
    app_uart_put(esb_fds.addr_0[1]);  // data:1
    app_uart_put(esb_fds.addr_0[2]);  // data:2
    app_uart_put(esb_fds.addr_0[3]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void return_add1_to_ch552(void) {
    app_uart_flush();
    app_uart_put(0xff);  // head
    app_uart_put(5);     // len

    app_uart_put(0x02);                // cmd:2
    app_uart_put(esb_fds.addr_1[0]);  // data:0
    app_uart_put(esb_fds.addr_1[1]);  // data:1
    app_uart_put(esb_fds.addr_1[2]);  // data:2
    app_uart_put(esb_fds.addr_1[3]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void set_add1_from_ch552(void) {
    app_uart_flush();
    uint8_t need_update_fds = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        NRF_LOG_INFO("set_add1_from_ch552 d[%d] = %d", i, rx_data[i]);
        if (rx_data[i] != esb_fds.addr_1[i]) {
            esb_fds.addr_1[i] = rx_data[i];
            need_update_fds = 1;
        }
    }
    if (need_update_fds) {
        fds_update_esb_data();
    }

    app_uart_put(0xff);  // head
    app_uart_put(5);     // len

    app_uart_put(0x03);                // cmd:3
    app_uart_put(esb_fds.addr_1[0]);  // data:0
    app_uart_put(esb_fds.addr_1[1]);  // data:1
    app_uart_put(esb_fds.addr_1[2]);  // data:2
    app_uart_put(esb_fds.addr_1[3]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void return_perfix_to_ch552(void) {
    app_uart_flush();
    app_uart_put(0xff);  // head
    app_uart_put(9);  // len

    app_uart_put(0x04);                    // cmd:4
    app_uart_put(esb_fds.addr_perfix[0]);  // data:0
    app_uart_put(esb_fds.addr_perfix[1]);  // data:1
    app_uart_put(esb_fds.addr_perfix[2]);  // data:2
    app_uart_put(esb_fds.addr_perfix[3]);  // data:3
    app_uart_put(esb_fds.addr_perfix[4]);  // data:3
    app_uart_put(esb_fds.addr_perfix[5]);  // data:3
    app_uart_put(esb_fds.addr_perfix[6]);  // data:3
    app_uart_put(esb_fds.addr_perfix[7]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void set_perfix_from_ch552(void) {
    app_uart_flush();
    uint8_t need_update_fds = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        NRF_LOG_INFO("set_perfix_from_ch552 d[%d] = %d", i, rx_data[i]);
        if (rx_data[i] != esb_fds.addr_perfix[i]) {
            esb_fds.addr_perfix[i] = rx_data[i];
            need_update_fds = 1;
        }
    }
    if (need_update_fds) {
        fds_update_esb_data();
    }

    app_uart_put(0xff);  // head
    app_uart_put(9);  // len

    app_uart_put(0x05);                    // cmd:5
    app_uart_put(esb_fds.addr_perfix[0]);  // data:0
    app_uart_put(esb_fds.addr_perfix[1]);  // data:1
    app_uart_put(esb_fds.addr_perfix[2]);  // data:2
    app_uart_put(esb_fds.addr_perfix[3]);  // data:3
    app_uart_put(esb_fds.addr_perfix[4]);  // data:3
    app_uart_put(esb_fds.addr_perfix[5]);  // data:3
    app_uart_put(esb_fds.addr_perfix[6]);  // data:3
    app_uart_put(esb_fds.addr_perfix[7]);  // data:3

    app_uart_put(0xfe);  // tail
}

static void return_channel_to_ch552(void) {
    app_uart_flush();
    app_uart_put(0xff);  // head
    app_uart_put(2);  // len

    app_uart_put(0x06);             // cmd:6
    app_uart_put(esb_fds.channel);  // data:0

    app_uart_put(0xfe);  // tail
}

static void set_channel_from_ch552(void) {
    app_uart_flush();
    uint8_t need_update_fds = 0;
    NRF_LOG_INFO("set_channel_from_ch552 d[0] = %d", rx_data[0]);
    if (rx_data[0] != esb_fds.channel) {
        esb_fds.channel = rx_data[0];
        need_update_fds = 1;
    }
    if (need_update_fds) {
        fds_update_esb_data();
    }

    app_uart_put(0xff);  // head
    app_uart_put(2);  // len

    app_uart_put(0x07);             // cmd:7
    app_uart_put(esb_fds.channel);  // data:0

    app_uart_put(0xfe);  // tail
}


static void uart_event_handle_by_header_buffer(app_uart_evt_t *p_event) {
    static uint8_t temp_buff = 0xff;
    uint32_t err_code;
    switch (p_event->evt_type) {
    case APP_UART_DATA_READY:
        UNUSED_VARIABLE(app_uart_get(&temp_buff));
        if (rx_pkg_s == 0) {
            if (temp_buff == 0xff) {
                rx_pkg_s = 1;
                rx_pkg_i = 0;
                rx_pkg_len = 0;
                memset(rx_data, 0, sizeof(rx_data));
                NRF_LOG_INFO("Star Catch CMD");
            } else {
                rx_pkg_i = 0;
                rx_pkg_len = 0;
            }
        } else {
            if (rx_pkg_i == 0) {
                cmd_i = temp_buff;
                if (cmd_i > CMD_MAX_I) {
                    rx_pkg_s = 0;
                    rx_pkg_i = 0;
                }
                rx_pkg_len = cmd_lens[cmd_i];
                NRF_LOG_INFO("Catch CMD %d, len %d", cmd_i, rx_pkg_len);
            } else if (rx_pkg_i-1 >= rx_pkg_len) {
                if (temp_buff == 0xfe) {
                    // 执行命令
                    NRF_LOG_INFO("RUN CMD %d", cmd_i);
                    switch (cmd_i) {
                        case 0:
                            return_add0_to_ch552();
                            break;
                        case 1:
                            set_add0_from_ch552();
                            break;
                        case 2:
                            return_add1_to_ch552();
                            break;
                        case 3:
                            set_add1_from_ch552();
                            break;
                        case 4:
                            return_perfix_to_ch552();
                            break;
                        case 5:
                            set_perfix_from_ch552();
                            break;
                        case 6:
                            return_channel_to_ch552();
                            break;
                        case 7:
                            set_channel_from_ch552();
                            break;
                        case 8:
                        break;
                    }
                }
                rx_pkg_s = 0;
                rx_pkg_i = 0;
            } else {
                rx_data[rx_pkg_i-1] = temp_buff;
            }
            rx_pkg_i++;
        }
        break;
    case APP_UART_COMMUNICATION_ERROR:
        NRF_LOG_INFO("ERROR: COMMUNICATION ERROR");
        // APP_ERROR_HANDLER(p_event->data.error_communication);
        break;
    case APP_UART_FIFO_ERROR:
        NRF_LOG_INFO("ERROR: FIFO ERROR");
        // APP_ERROR_HANDLER(p_event->data.error_code);
        break;
    default:
        break;
    }
}

/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
// #undef  UART_PRESENT
void uart_init(void) {
    uint32_t err_code;
    app_uart_comm_params_t const comm_params =
        {
            .rx_pin_no = RX_PIN_NUMBER,
            .tx_pin_no = TX_PIN_NUMBER,
            .rts_pin_no = RTS_PIN_NUMBER,
            .cts_pin_no = CTS_PIN_NUMBER,
            .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
            .use_parity = false,
            .baud_rate = NRF_UART_BAUDRATE_38400};
    APP_UART_FIFO_INIT(&comm_params,
        UART_RX_BUF_SIZE,
        UART_TX_BUF_SIZE,
        uart_event_handle_by_header_buffer,
        APP_IRQ_PRIORITY_LOWEST,
        err_code);
    nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(TX_PIN_NUMBER, NRF_GPIO_PIN_PULLUP);
    APP_ERROR_CHECK(err_code);
}