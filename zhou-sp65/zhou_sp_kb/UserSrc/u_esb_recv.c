#include "u_esb_recv.h"
#include "common_cfg.h"

#include "nrf_esb.h"
#include "nrf_log.h"
#include "app_error.h"
#include "sp_matrix.h"

// 在别的地方定义过了
//esb_fds_t esb_fds;

static void nrf_esb_event_handler_rx(nrf_esb_evt_t const *p_event);

// 123
//extern nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;

void process_esb_data(uint8_t *esb_data) {

}

static void nrf_esb_event_handler_tx(nrf_esb_evt_t const *p_event) {
    uint32_t err_code;
    switch (p_event->evt_id) {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_INFO("TXD: len=%d, pipe=%d, rssi=%d, noack=%d, pid=%d", tx_payload.length, tx_payload.pipe, tx_payload.rssi, tx_payload.noack, tx_payload.pid);
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG("TX FAILED EVENT");
            (void) nrf_esb_flush_tx();
            (void) nrf_esb_start_tx();
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            NRF_LOG_DEBUG("RX RECEIVED EVENT");
            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {
                NRF_LOG_INFO("RXD: len=%d, pipe=%d, rssi=%d, noack=%d, pid=%d", rx_payload.length, rx_payload.pipe, rx_payload.rssi, rx_payload.noack, rx_payload.pid);
            }
            break;
    }
}


void esb_init_tx(void) {
    uint32_t err_code;

    nrf_esb_config_t nrf_esb_config = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.payload_length           = 32;
    nrf_esb_config.protocol = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.bitrate = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.mode = NRF_ESB_MODE_PTX;
    nrf_esb_config.event_handler = nrf_esb_event_handler_tx;
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
