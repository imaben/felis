#pragma once

#include <stdint.h>
#include <event.h>
#include "workerqueue.h"

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
    workqueue_t workqueue;
} felis_ctx_t;

typedef struct {
    int fd;
    struct event_base *evbase;
    struct bufferevent *buf_ev;
    struct evbuffer *output_buffer;
} felis_client_t;

felis_config_t *get_config();
felis_ctx_t *get_ctx();

