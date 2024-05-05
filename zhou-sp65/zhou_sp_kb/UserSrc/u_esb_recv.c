#include "u_esb_recv.h"
#include "common_cfg.h"

#include "nrf_esb.h"
#include "nrf_log.h"
#include "app_error.h"

// 在别的地方定义过了
//esb_fds_t esb_fds;

static void nrf_esb_event_handler_rx(nrf_esb_evt_t const *p_event);

// 123
static nrf_esb_payload_t tx_payload = NRF_ESB_CREATE_PAYLOAD(2, 0x01, 0x00);
static nrf_esb_payload_t rx_payload;

void process_esb_data(uint8_t *esb_data) {
    //switch (esb_data[0]) {
    //  case REPORT_INDEX_KEYBOARD:
    //    //trp_kb.mods = esb_data[1];
    //    //trp_kb.reserved = esb_data[2];
    //    //memcpy(trp_kb.keys, &esb_data[3], KEYBOARD_REPORT_KEYS);
    //    //usbd_send_keyboard(&trp_kb, false);
    //    break;
    //  case REPORT_INDEX_NKRO:
    //    //trp_kb.nkro.mods = esb_data[1];
    //    //memcpy(trp_kb.nkro.bits, &esb_data[2], KEYBOARD_REPORT_BITS);
    //    //usbd_send_keyboard(&trp_kb, true);
    //    break;
    //  case REPORT_INDEX_CONSUMER:
    //    //trp_extra.usage = esb_data[1] | (esb_data[2] << 8);
    //    //usbd_send_extra(&trp_extra, REPORT_INDEX_CONSUMER);
    //    break;
    //  case REPORT_INDEX_SYSTEM:
    //    //trp_extra.usage = esb_data[1] | (esb_data[2] << 8);
    //    //usbd_send_extra(&trp_extra, REPORT_INDEX_SYSTEM);
    //    break;
    //  case REPORT_INDEX_MOUSE:
    //    //trp_mouse.buttons = esb_data[1];
    //    //trp_mouse.x = esb_data[2];
    //    //trp_mouse.y = esb_data[3];
    //    //trp_mouse.v = esb_data[4];
    //    //trp_mouse.h = esb_data[5];
    //    // usbd_send_mouse(&trp_mouse);
    //    break;
    // case QUESTION_PKG:
    //    //send_led_state_to_esbtx(get_usbd_led_state());
    //    break;
    //  default:
    //    NRF_LOG_INFO("Not handle esb data type: %d", esb_data[0]);
    //}
}

static void nrf_esb_event_handler_rx(nrf_esb_evt_t const *p_event) {
    uint32_t err_code;
    switch (p_event->evt_id) {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_INFO("TX SUCCESS EVENT len %d: %02x %02x %02x ... %02x %02x", tx_payload.length,
                tx_payload.data[0],
                tx_payload.data[1],
                tx_payload.data[2],
                tx_payload.data[3],
                tx_payload.data[31]);
            //NRF_LOG_INFO("TX count %d", return_esb_tx_fifo_count());
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG("TX FAILED EVENT");
            (void)nrf_esb_flush_tx();
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            NRF_LOG_DEBUG("RX RECEIVED EVENT");
            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {
                NRF_LOG_DEBUG("Receiving packet len %d: %02x %02x %02x %02x... rssi: %d", rx_payload.length,
                    rx_payload.data[0],
                    rx_payload.data[1],
                    rx_payload.data[2],
                    rx_payload.data[3],
                    rx_payload.rssi);
                //NRF_LOG_INFO("TX count %d", return_esb_tx_fifo_count());
                //// 处理键盘发过来的数据
                process_esb_data(rx_payload.data);
                //// 回传这次键盘发来的数据, 测试用
                //memcpy(tx_payload.data, rx_payload.data, NRF_ESB_MAX_PAYLOAD_LENGTH);
                //tx_payload.length = rx_payload.length;
                //// NRF_LOG_DEBUG("remain before %d %d", get_fifo_count(), tx_payload.data[1]);
                //err_code = nrf_esb_write_payload(&tx_payload);
                //// NRF_LOG_DEBUG("Resend err %d, remain %d", err_code, get_fifo_count());

            }
            break;
    }
}

void send_led_state_to_esbtx(uint8_t report_val) {
    uint32_t err_code;
    tx_payload.length = 2;
    // tx_payload.data[0] = ESB_LED_STATE; // type: led
    tx_payload.data[1] = report_val;
    err_code = nrf_esb_write_payload(&tx_payload);
    NRF_LOG_INFO("send report val: %d, err %d", report_val, err_code);
}

void esb_init_rx(void) {
    uint32_t err_code;

    nrf_esb_config_t nrf_esb_config = NRF_ESB_DEFAULT_CONFIG;
    // nrf_esb_config.payload_length           = 8;
    nrf_esb_config.protocol = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.bitrate = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.mode = NRF_ESB_MODE_PRX;
    nrf_esb_config.event_handler = nrf_esb_event_handler_rx;
    nrf_esb_config.selective_auto_ack = false;

    err_code = nrf_esb_init(&nrf_esb_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_esb_set_rf_channel(esb_fds.channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_esb_set_base_address_0(esb_fds.addr_0);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_esb_set_base_address_1(esb_fds.addr_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_esb_set_prefixes(esb_fds.addr_perfix, 8);
    APP_ERROR_CHECK(err_code);
}

void esb_enable(void) {
    uint32_t err_code;
    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);
}