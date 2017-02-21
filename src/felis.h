#pragma once

#include <stdint.h>
#include <event.h>
#include <pthread.h>
#include "dict.h"

typedef struct {
    char*   listen_host;
    int     listen_port;
    int     timeout;
    int     threads;
    short   daemon;
    char*   logfile;
    short   logmask;
} felis_config_t;

typedef struct {
    pthread_t thread;
    struct event_base *evbase;
    struct evhttp *httpd;
} felis_thread_t;

typedef struct {
    int listenfd;
    felis_config_t* cfg;
    felis_thread_t* threads;
    dict_t* dicts;
} felis_ctx_t;

felis_ctx_t *get_ctx();

#define DEFAULT_HTTP_TIMEOUT 30
#define VERSION "0.0.1"
