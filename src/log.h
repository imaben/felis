#pragma once

#include <stdio.h>

#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_NOTICE  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_ERROR   3

int log_init(char *file, int mark);
void log_write(int level, char *fmt, ...);
void log_destroy();

#define log_debug(fmt, ...) \
    log_write(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define log_notice(fmt, ...) \
    log_write(LOG_LEVEL_NOTICE, fmt, ##__VA_ARGS__)

#define log_warn(fmt, ...) \
    log_write(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)

#define log_error(fmt, ...) \
    log_write(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
