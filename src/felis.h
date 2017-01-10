#ifndef __FELIS_H__
#define __FELIS_H__

#include <stdint.h>

typedef struct {
    uint8_t *listen_host;
    uint16_t listen_port;
    uint8_t threads;
    uint8_t daemon;
} felis_config_t;

#endif
