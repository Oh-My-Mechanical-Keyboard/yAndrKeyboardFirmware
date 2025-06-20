#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_FILE     (0x8010)
#define CONFIG_REC_KEY  (0x7010)



void fds_init_my(void);
void fds_update_esb_data(void);
