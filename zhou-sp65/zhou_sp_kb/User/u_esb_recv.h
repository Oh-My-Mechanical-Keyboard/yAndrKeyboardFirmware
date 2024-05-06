#pragma once

#include <stdint.h>

void send_led_state_to_esbtx(uint8_t report_val);

void esb_enable(void);
void esb_init_rx(void);
