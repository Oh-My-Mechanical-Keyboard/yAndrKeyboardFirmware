#include "sp_matrix.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "nrf_log.h"
#include "app_error.h"
#include "nrfx_gpiote.h"
#include "u_adc.h"
#include "nrf_delay.h"

APP_TIMER_DEF(m_scan_timer_id); /**< Scan timer. */
APP_TIMER_DEF(m_keep_timer_id); /**< Keep timer. */

nrf_esb_payload_t tx_payload = NRF_ESB_CREATE_PAYLOAD(PIPE_NUMBER, 0x01, 0x00);

const uint32_t ROW_PINS[ROWS] = MATRIX_ROW_PINS;
const unsigned short REMAINING_POSITIONS = 8 - ROWS;
const uint8_t mask_of_each_595[NUM_OF_74HC595][COLUMNS] = MATRIX_OF_74HC595;

// Data and acknowledgement payloads
static uint8_t data_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to Host.
static uint8_t ack_payload[ESB_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from Host.

// Key buffers
static uint8_t keys_buffer[COLUMNS+2];
static uint32_t activity_ticks;

static void send_mx_data(void) {
    tx_payload.length = COLUMNS+2;
    memcpy(tx_payload.data, keys_buffer, COLUMNS+2);
    nrf_esb_write_payload(&tx_payload);
}

static void shift_out_single(uint8_t value) {
    for (uint8_t i = 0; i < 8; i++) {
        nrf_gpio_pin_clear(HC595_SCK);
        if ((value>>i)&1) {
            nrf_gpio_pin_set(HC595_SER);
        } else {
            nrf_gpio_pin_clear(HC595_SER);
        }
        nrf_delay_us(5);
        nrf_gpio_pin_set(HC595_SCK);
    }
}

static void select_all_col(void) {
#ifdef COMPILE_RIGHT
        nrf_gpio_pin_set(R_COL8_PIN);
#endif
    nrf_gpio_pin_clear(HC595_RCK);
    for (uint8_t i = 0; i < NUM_OF_74HC595; ++i) {
        shift_out_single(0xff);
    }
    nrf_gpio_pin_set(HC595_RCK);

}


static void select_col(uint8_t col) {
#ifdef COMPILE_RIGHT
    if (col == 8) {
        nrf_gpio_pin_set(R_COL8_PIN);
        return;
    }
#endif
    nrf_gpio_pin_clear(HC595_RCK);
    for (uint8_t i = 0; i < NUM_OF_74HC595; ++i) {
        shift_out_single(mask_of_each_595[i][col]);
    }
    nrf_gpio_pin_set(HC595_RCK);

}

static void unselect_col(uint8_t col) {
#ifdef COMPILE_RIGHT
    if (col == 8) {
        nrf_gpio_pin_clear(R_COL8_PIN);
    }
#endif
    nrf_gpio_pin_clear(HC595_RCK);
    for (uint8_t i = 0; i < NUM_OF_74HC595; ++i) {
        shift_out_single(0x00);
    }
    nrf_gpio_pin_set(HC595_RCK);
}

#define setbit(x, y)  (x=(x)|(1<<y))
#define clrbit(x, y)  (x=(x)&(~(1<<y)))
static void key_scan_once(void) {
    uint32_t input = 0;
    for (uint8_t i = 0; i < COLUMNS; ++i) {
        keys_buffer[i] = 0; // 每列的buffer
        select_col(i);
        nrf_delay_us(30); // 实际测试1us也行，防止特别垃圾的轴有问题
        for (uint8_t j = 0; j < ROWS; ++j) {
            if (nrf_gpio_pin_read(ROW_PINS[j]) == 1) {
                keys_buffer[i] = keys_buffer[i] | (1<<j);
            } else {
                keys_buffer[i] = keys_buffer[i] & (~(1<<j));
            }
        }
        unselect_col(i);
    }
}

static bool empty_keys(void) {
    for(int i=0; i < COLUMNS; i++) {
        if (keys_buffer[i]) {
          return false;
        }
    }
    return true;
}

/**
  GPIOTE中断处理
 */
void in_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_INFO("in_pin_handler %d\n", pin);
    for (uint8_t i = 0; i < ROWS; ++i) {
        nrfx_gpiote_in_event_disable(ROW_PINS[i]);
    }
    ////enable rtc interupt triggers
    app_timer_start(m_scan_timer_id, KEYBOARD_SCAN_INTERVAL, NULL);
    //app_timer_start(m_keep_timer_id, KEYBOARD_KEEP_INTERVAL, NULL);

    //// 和二极管方向有关，配置唤醒后的输出状态
    for (uint8_t i = 0; i < ROWS; ++i) {
        nrf_gpio_cfg_input(ROW_PINS[i], NRF_GPIO_PIN_PULLDOWN);
    }
    for (uint8_t i = 0; i < COLUMNS; ++i) {
        unselect_col(i);
    }
    activity_ticks = 0;
}

static void sp_matrix_scan_task(void) {
    // 键扫一次
    key_scan_once();
    // 去抖在键盘端用，直接发送数据
    send_mx_data();
    // looking for 500 ticks of no keys pressed, to go back to deep sleep
    if (empty_keys()) {
        activity_ticks++;
        if (activity_ticks > ACTIVITY) {
            activity_ticks = 0;
            if (nrf_gpio_pin_read(VBUS_READ) != 1) {
              // 休眠
               app_timer_stop(m_scan_timer_id);
               //app_timer_stop(m_keep_timer_id);
              // 和二极管方向有关，配置输出引脚，用来进行触发唤醒
              select_all_col();
              nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
              in_config.pull = NRF_GPIO_PIN_PULLDOWN;
              for (uint8_t i = 0; i < ROWS; ++i) {
                   nrfx_gpiote_in_uninit(ROW_PINS[i]);
                   ret_code_t err_code = nrfx_gpiote_in_init(ROW_PINS[i], &in_config, in_pin_handler);
                   NRF_LOG_INFO("nrfx_gpiote_in_init ROW_PINS[%d]:%d\n", i, err_code);
                   // APP_ERROR_CHECK(err_code);
                  nrfx_gpiote_in_event_enable(ROW_PINS[i], true);
              }            
            }
        }
    } else {
        activity_ticks = 0;
    }
}

static void sp_matrix_keep_task(void) {
    uint16_t bat_v = bat_percent();
    NRF_LOG_INFO("bat_percent: %d\n", bat_v);
    keys_buffer[COLUMNS] = (uint8_t)(bat_v >> 8);
    keys_buffer[COLUMNS+1] = (uint8_t)(bat_v & 0xff);
    send_mx_data();
}


/* 记录超时的次数 */
static void keyboard_scan_timeout_handler(void *p_context) {
    UNUSED_PARAMETER(p_context);
    sp_matrix_scan_task();
}

/* 记录超时的次数 */
static void keyboard_keep_timeout_handler(void *p_context) {
    UNUSED_PARAMETER(p_context);
    sp_matrix_keep_task();
}

void sp_matrix_init(void) {
    // init vbus
    nrf_gpio_cfg_input(VBUS_READ, NRF_GPIO_PIN_PULLDOWN);
    // init row
    for (uint8_t i = 0; i < ROWS; ++i) {
        nrf_gpio_cfg_input(ROW_PINS[i], NRF_GPIO_PIN_PULLDOWN);
    }
    // init col
    nrf_gpio_cfg_output(HC595_SER);
    nrf_gpio_pin_clear(HC595_SER);
    nrf_gpio_cfg_output(HC595_RCK);
    nrf_gpio_pin_clear(HC595_RCK);
    nrf_gpio_cfg_output(HC595_SCK);
    nrf_gpio_pin_clear(HC595_SCK);
#ifdef COMPILE_RIGHT
    nrf_gpio_cfg_output(R_COL8_PIN);
    nrf_gpio_pin_clear(R_COL8_PIN);
#endif
    for (uint8_t i = 0; i < COLUMNS; ++i) {
        unselect_col(i);
    }
    // init args
    activity_ticks = 0;
    // init scan timer
    ret_code_t err_code = app_timer_create(&m_scan_timer_id, APP_TIMER_MODE_REPEATED, keyboard_scan_timeout_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(m_scan_timer_id, KEYBOARD_SCAN_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
    //// init keep timer
    err_code = app_timer_create(&m_keep_timer_id, APP_TIMER_MODE_REPEATED, keyboard_keep_timeout_handler);
    APP_ERROR_CHECK(err_code);
    //err_code = app_timer_start(m_keep_timer_id, KEYBOARD_KEEP_INTERVAL, NULL);
    //APP_ERROR_CHECK(err_code);

    // init int
    err_code = nrfx_gpiote_init();
    APP_ERROR_CHECK(err_code);
}
