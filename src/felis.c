#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "felis.h"
#include "log.h"
#include "daemon.h"

felis_config_t __config = {
    .listen_host = "0.0.0.0",
    .listen_port = 8080,
    .threads = 4,
    .daemon = 0,
    .logfile = "/tmp/felis.log",
    .logmask = LOG_LEVEL_DEBUG
}, *felis_cfg = &__config;

static const struct option options[] = {
    {"host",  2, NULL, 'h'},
    {"port",   2, NULL, 'p'},
    {"threads", 0, NULL, 't'},
    {"daemon", 0, NULL, 'd'},
    {"log",    0, NULL, 'l'},
    {"logmask", 0, NULL, 'm'},
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
            case 'l':
                felis_cfg->logfile = strdup(optarg);
                break;
            case 'm':
                if (strcasecmp(optarg, "debug") == 0) {
                    felis_cfg->logmask = LOG_LEVEL_DEBUG;
                } else if (strcasecmp(optarg, "notice") == 0) {
                    felis_cfg->logmask = LOG_LEVEL_NOTICE;
                } else if (strcasecmp(optarg, "warn") == 0) {
                    felis_cfg->logmask = LOG_LEVEL_WARN;
                } else if (strcasecmp(optarg, "error") == 0) {
                    felis_cfg->logmask = LOG_LEVEL_ERROR;
                } else {
                    fatal("Invalid logmask argument\n");
                }
                break;
            case 'H':
            case '?':
                usage();
        }
    }

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

    if (log_init(felis_cfg->logfile, felis_cfg->logmask) == -1) {
        fatal("Failed to initialize log file:%s\n", felis_cfg->logfile);
    }

    if (felis_cfg->daemon) {
        daemonize();
    }
    return 0;
}
