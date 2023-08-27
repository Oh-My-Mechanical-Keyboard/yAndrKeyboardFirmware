/*
 * Either COMPILE_RIGHT or COMPILE_LEFT has to be defined from the make call to allow proper functionality
 */
#include "sp65pm.h"
#include "nrf_drv_config.h"
#include "nrf_gzll.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "nrf51_bitfields.h"
#include "nrf51.h"
#include "nrf_drv_adc.h"
#include "nrf_log.h"
#include "app_util_platform.h"





/*****************************************************************************/
/** Configuration */
/*****************************************************************************/

const nrf_drv_rtc_t rtc_maint = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
const nrf_drv_rtc_t rtc_deb = NRF_DRV_RTC_INSTANCE(1); /**< Declaring an instance of nrf_drv_rtc for RTC1. */

const uint32_t COL_PINS[COLUMNS] = { C01, C02, C03, C04, C05, C06, C07, C08, C09 };
const unsigned short REMAINING_POSITIONS = 8 - ROWS;

#define ADC_BUFFER_SIZE 10                                /**< Size of buffer for ADC samples.  */
static nrf_adc_value_t       adc_buffer[ADC_BUFFER_SIZE]; /**< ADC buffer. */
static nrf_drv_adc_channel_t m_channel_config =  {{{                                                       \
    .resolution = NRF_ADC_CONFIG_RES_10BIT,                \
    .input      = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD, \
    .reference  = NRF_ADC_CONFIG_REF_SUPPLY_ONE_THIRD,                  \
    .ain        = (NRF_ADC_CONFIG_INPUT_5)                           \
 }}, NULL};


// Define payload length
#define TX_PAYLOAD_LENGTH COLUMNS ///< 5 byte payload length when transmitting

// Data and acknowledgement payloads
#ifdef COMPILE_PAD
static uint8_t data_payload[TX_PAYLOAD_LENGTH+2];                ///< Payload to send to Host.
#else
static uint8_t data_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to Host.
#endif
static uint8_t ack_payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from Host.

// Debounce time (dependent on tick frequency)
#define DEBOUNCE 5
#define ACTIVITY 500

// Key buffers
static uint8_t keys[COLUMNS], keys_snapshot[COLUMNS], keys_buffer[COLUMNS];
static uint32_t debounce_ticks, activity_ticks;
static volatile bool debouncing = false;

// Debug helper variables
static volatile bool init_ok, enable_ok, push_ok, pop_ok, tx_success;

#ifdef COMPILE_LEFT
static uint8_t channel_table[3]={4, 42, 77};
#endif
#ifdef COMPILE_RIGHT
static uint8_t channel_table[3]={25, 63, 33};
#endif
#ifdef COMPILE_PAD
static uint8_t channel_table[3]={11, 22, 44};
#endif

#ifdef COMPILE_PAD
static int16_t adc_val = 0;
/**
 * @brief ADC interrupt handler.
 */
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
    uint32_t t_sum_adc = 0;
    if (p_event->type == NRF_DRV_ADC_EVT_DONE)
    {
        uint32_t i;
        for (i = 0; i < p_event->data.done.size; i++)
        {
            t_sum_adc += p_event->data.done.p_buffer[i];
            NRF_LOG_PRINTF("Current sample value: %d\r\n", p_event->data.done.p_buffer[i]);
        }
    }
    adc_val = t_sum_adc/p_event->data.done.size;
}

/**
 * @brief ADC initialization.
 */
static void adc_config(void)
{
    nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;

    nrf_drv_adc_init(&config, adc_event_handler);

    nrf_drv_adc_channel_enable(&m_channel_config);
}
#endif

// Setup switch pins with pullups
static void gpio_config(void)
{
    nrf_gpio_cfg_sense_input(R01, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(R02, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(R03, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(R04, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(R05, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);

    nrf_gpio_cfg_output(C01);
    nrf_gpio_cfg_output(C02);
    nrf_gpio_cfg_output(C03);
    nrf_gpio_cfg_output(C04);
    nrf_gpio_cfg_output(C05);
    nrf_gpio_cfg_output(C06);
    nrf_gpio_cfg_output(C07);
    nrf_gpio_cfg_output(C08);
    nrf_gpio_cfg_output(C09);
}

// Return the key states
#define setbit(x, y)  (x=(x)|(1<<y))
#define clrbit(x, y)  (x=(x)&(~(1<<y)))
uint8_t cols_pin[COLUMNS] = {C01, C02, C03, C04, C05, C06, C07, C08, C09};
static void read_keys_by_col(void) {
    // keys_buffer = read_col();
    uint8_t buff[COLUMNS] = {0};
    uint32_t input = 0;
    for (uint8_t i = 0; i < COLUMNS; ++i) {
        nrf_gpio_pin_set(cols_pin[i]); // 把第i col拉高
        input = 0;
        input = NRF_GPIO->IN;  // 获取到所有row目前的状态
        // buff[0] 取出目前的状态
        // ((input >> R01) & 1) 判断第1 row是不是被拉高了
        ((input >> R01) & 1) ? setbit(buff[i], 0) : clrbit(buff[i], 0); // 设置第1（C01）列数据（buff[0]）的第（i）行
        ((input >> R02) & 1) ? setbit(buff[i], 1) : clrbit(buff[i], 1);
        ((input >> R03) & 1) ? setbit(buff[i], 2) : clrbit(buff[i], 2);
        ((input >> R04) & 1) ? setbit(buff[i], 3) : clrbit(buff[i], 3);
        ((input >> R05) & 1) ? setbit(buff[i], 4) : clrbit(buff[i], 4);
        nrf_gpio_pin_clear(cols_pin[i]);
    }
    keys_buffer[0] = buff[0];
    keys_buffer[1] = buff[1];
    keys_buffer[2] = buff[2];
    keys_buffer[3] = buff[3];
    keys_buffer[4] = buff[4];
    keys_buffer[5] = buff[5];
    keys_buffer[6] = buff[6];
    keys_buffer[7] = buff[7];
    keys_buffer[8] = buff[8];
    return;
}




static bool compare_keys(uint8_t* first, uint8_t* second, uint32_t size)
{
    for(int i=0; i < size; i++)
    {
        if (first[i] != second[i])
        {
          return false;
        }
    }
    return true;
}

static bool empty_keys(void)
{
    for(int i=0; i < COLUMNS; i++)
    {
        if (keys_buffer[i])
        {
          return false;
        }
    }
    return true;
}

// Assemble packet and send to receiver
static void send_data(void)
{
    for(int i=0; i < COLUMNS; i++)
    {
        data_payload[i] = keys[i];
    }
#ifdef COMPILE_PAD
    data_payload[COLUMNS] = (adc_val & 0x0f);
    data_payload[COLUMNS+1] = (adc_val>>8) & 0x0f;
    nrf_gzll_add_packet_to_tx_fifo(PIPE_NUMBER, data_payload, TX_PAYLOAD_LENGTH+2);
#else
    nrf_gzll_add_packet_to_tx_fifo(PIPE_NUMBER, data_payload, TX_PAYLOAD_LENGTH);
#endif
}
// 8Hz held key maintenance, keeping the reciever keystates valid
static void handler_maintenance(nrf_drv_rtc_int_type_t int_type)
{
    send_data();
}

// 1000Hz debounce sampling
static void handler_debounce(nrf_drv_rtc_int_type_t int_type)
{
#ifdef COMPILE_PAD
    nrf_drv_adc_buffer_convert(adc_buffer,ADC_BUFFER_SIZE);
    nrf_drv_adc_sample();
#endif
    read_keys_by_col();

    // debouncing, waits until there have been no transitions in 5ms (assuming five 1ms ticks)
    if (debouncing)
    {
        // if debouncing, check if current keystates equal to the snapshot
        if (compare_keys(keys_snapshot, keys_buffer, COLUMNS))
        {
            // DEBOUNCE ticks of stable sampling needed before sending data
            debounce_ticks++;
            if (debounce_ticks == DEBOUNCE)
            {
                for(int j=0; j < COLUMNS; j++)
                {
                    keys[j] = keys_snapshot[j];
                }
                send_data();
            }
        }
        else
        {
            // if keys change, start period again
            debouncing = false;
        }
    }
    else
    {
        // if the keystate is different from the last data
        // sent to the receiver, start debouncing
        if (!compare_keys(keys, keys_buffer, COLUMNS))
        {
            for(int k=0; k < COLUMNS; k++)
            {
                keys_snapshot[k] = keys_buffer[k];
            }
            debouncing = true;
            debounce_ticks = 0;
        }
    }

    // looking for 500 ticks of no keys pressed, to go back to deep sleep
    if (empty_keys())
    {
        activity_ticks++;
        if (activity_ticks > ACTIVITY)
        {
            nrf_drv_rtc_disable(&rtc_maint);
            nrf_drv_rtc_disable(&rtc_deb);
            nrf_gpio_pin_set(C01);
            nrf_gpio_pin_set(C02);
            nrf_gpio_pin_set(C03);
            nrf_gpio_pin_set(C04);
            nrf_gpio_pin_set(C05);
            nrf_gpio_pin_set(C06);
            nrf_gpio_pin_set(C07);
            nrf_gpio_pin_set(C08);
            nrf_gpio_pin_set(C09);
        }

    }
    else
    {
        activity_ticks = 0;
    }

}

// Low frequency clock configuration
static void lfclk_config(void)
{
    nrf_drv_clock_init();

    nrf_drv_clock_lfclk_request(NULL);
}

// RTC peripheral configuration
static void rtc_config(void)
{
    //Initialize RTC instance
    nrf_drv_rtc_init(&rtc_maint, NULL, handler_maintenance);
    nrf_drv_rtc_init(&rtc_deb, NULL, handler_debounce);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&rtc_maint,true);
    nrf_drv_rtc_tick_enable(&rtc_deb,true);

    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc_maint);
    nrf_drv_rtc_enable(&rtc_deb);
}

int main()
{
    // Initialize Gazell
    nrf_gzll_init(NRF_GZLL_MODE_DEVICE);

    // Attempt sending every packet up to 100 times
    nrf_gzll_set_max_tx_attempts(100);
    nrf_gzll_set_timeslots_per_channel(4);
    nrf_gzll_set_channel_table(channel_table,3);
    nrf_gzll_set_datarate(NRF_GZLL_DATARATE_1MBIT);
    nrf_gzll_set_timeslot_period(900);

    // Addressing
    nrf_gzll_set_base_address_0(0x01020304);
    nrf_gzll_set_base_address_1(0x05060708);

    // Enable Gazell to start sending over the air
    nrf_gzll_enable();

    // Configure 32kHz xtal oscillator
    lfclk_config();

    // Configure RTC peripherals with ticks
    rtc_config();

    // Configure all keys as inputs with pullups
    gpio_config();

    // Set the GPIOTE PORT event as interrupt source, and enable interrupts for GPIOTE
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    NVIC_EnableIRQ(GPIOTE_IRQn);

#ifdef COMPILE_PAD
    adc_config();
    // NRF_POWER->RESET = POWER_RESET_RESET_Enabled;
#endif

    // Main loop, constantly sleep, waiting for RTC and gpio IRQs
    while(1)
    {
        __SEV();
        __WFE();
        __WFE();
    }
}

// This handler will be run after wakeup from system ON (GPIO wakeup)
void GPIOTE_IRQHandler(void)
{
    if(NRF_GPIOTE->EVENTS_PORT)
    {
        //clear wakeup event
        NRF_GPIOTE->EVENTS_PORT = 0;

        //enable rtc interupt triggers
        nrf_drv_rtc_enable(&rtc_maint);
        nrf_drv_rtc_enable(&rtc_deb);

        nrf_gpio_pin_clear(C01);
        nrf_gpio_pin_clear(C02);
        nrf_gpio_pin_clear(C03);
        nrf_gpio_pin_clear(C04);
        nrf_gpio_pin_clear(C05);
        nrf_gpio_pin_clear(C06);
        nrf_gpio_pin_clear(C07);
        nrf_gpio_pin_clear(C08);
        nrf_gpio_pin_clear(C09);

        //TODO: proper interrupt handling to avoid fake interrupts because of matrix scanning
        //debouncing = false;
        //debounce_ticks = 0;
        activity_ticks = 0;
    }
}



/*****************************************************************************/
/** Gazell callback function definitions  */
/*****************************************************************************/

void  nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
    uint32_t ack_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

    if (tx_info.payload_received_in_ack)
    {
        // Pop packet and write first byte of the payload to the GPIO port.
        nrf_gzll_fetch_packet_from_rx_fifo(pipe, ack_payload, &ack_payload_length);
    }
}

// no action is taken when a packet fails to send, this might need to change
void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{

}

// Callbacks not needed
void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{}
void nrf_gzll_disabled()
{}
