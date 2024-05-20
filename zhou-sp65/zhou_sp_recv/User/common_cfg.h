#pragma once

#include <stdint.h>

#define USER_ESB_BASE_ADDR0 {0x96, 0x96, 0x96, 0x96}
#define USER_ESB_BASE_ADDR1 {0x97, 0x97, 0x97, 0x97}
#define USER_ESB_ADDR_PREFIX {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8}
#define USER_ESB_CHANNEL 0x4D

/* A dummy structure to save in flash. */
typedef struct
{
    uint8_t     addr_0[4];
    uint8_t     addr_1[4];
    uint8_t     addr_perfix[8];
    uint8_t     channel;
} esb_fds_t;

extern esb_fds_t esb_fds;