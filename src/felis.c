#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "felis.h"

felis_config_t __config = {
    .listen_host = "0.0.0.0",
    .listen_port = 8080,
    .threads = 4,
    .daemon = 0
}, *felis_cfg = &__config;

static const struct option options[] = {
    {"host",  2, NULL, 'h'},
    {"port",   2, NULL, 'p'},
    {"threads", 0, NULL, 't'},
    {"daemon", 0, NULL, 'd'},
    {"help",   0, NULL, 'H'},
};

#define fatal(fmt, ...) do {             \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    exit(1);                             \
} while (0)

void usage()
{
    printf("Usage: felis [options]\n"
            "Options:\n"
            "    -r <host>        listen host\n"
            "    -p <port>        port number\n"
            "    -d               daemon mode\n"
            "    -t <threads>     threads count\n"
            "    -d               show help\n");
    exit(0);
}

int felis_init_options(int argc, char **argv)
{
    int opt, i;

    while ((opt = getopt_long(argc, argv,
                                 "h:p:tdH", options, &i)) != -1)
    {
        switch (opt) {
            case 'h':
                felis_cfg->listen_host = strdup(optarg);
                break;
            case 'p':
                felis_cfg->listen_port = atoi(optarg);
                break;
            case 't':
                felis_cfg->threads = atoi(optarg);
            case 'd':
                felis_cfg->daemon = 0;
                break;
            case 'H':
            case '?':
                usage();
        }
    }

    felis_cfg->listen_port = 8888;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
    }

    if (felis_init_options(argc, argv) < 0) {
        fatal("Failed to parse options");
    }
    return 0;
}
