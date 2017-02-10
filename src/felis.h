#pragma once

#include <stdint.h>
#include <event.h>

typedef struct {
    uint8_t *listen_host;
    uint16_t listen_port;
    uint8_t threads;
    uint8_t daemon;
    uint8_t *logfile;
    uint32_t logmask;
} felis_config_t;

typedef struct {
    int listenfd;
    struct event_base *evbase;
} felis_ctx_t;

felis_ctx_t *get_felis_ctx();

