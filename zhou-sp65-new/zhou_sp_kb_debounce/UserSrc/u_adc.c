#include "u_adc.h"
#include "sp_matrix.h"
#include "nrf_log.h"
#include "app_error.h"
#include "nrf_drv_saadc.h"


#define SAMPLES_IN_BUFFER 5
static uint32_t              m_adc_evt_counter;

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        int i;
        NRF_LOG_INFO("ADC event number: %d", (int)m_adc_evt_counter);

        for (i = 0; i < SAMPLES_IN_BUFFER; i++)
        {
            NRF_LOG_INFO("%d", p_event->data.done.p_buffer[i]);
        }
        m_adc_evt_counter++;
    }
}

void my_adc_init(void) {
    ret_code_t err_code;
    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(VBAT_AIN);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);


}

uint16_t bat_percent(void) {
    ret_code_t err_code;
    nrf_saadc_value_t saadc_val;
    nrf_drv_saadc_sample_convert(0,&saadc_val);
    return (saadc_val*3600)>>9;
}
