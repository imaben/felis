#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "log.h"

static const char *error_titles[LOG_LEVEL_ERROR + 1] = {
    "DEBUG", "NOTICE", "WARN", "ERROR"
};

static int  log_fd         = 0;
static int  log_mark       = LOG_LEVEL_DEBUG;
static int  log_initialize = 0;
static char log_buffer[4096];


int log_init(char *file, int mark)
{
    if (log_initialize) {
        return 0;
    }

    if (mark < LOG_LEVEL_DEBUG || mark > LOG_LEVEL_ERROR)
    {
        return -1;
    }

    if (file) {
        log_fd = open(file, O_WRONLY | O_CREAT | O_APPEND , 0666);
        if (!log_fd) {
            return -1;
        }

    } else {
        dup2(log_fd, STDERR_FILENO);
    }

    log_mark = mark;
    log_initialize = 1;

    return 0;
}


void log_write(int level, char *fmt, ...)
{
    va_list al;
    time_t current;
    struct tm *dt;
    int off1, off2;

    if (!log_initialize
            || level < log_mark
            || level > LOG_LEVEL_ERROR)
    {
        return;
    }

    /* Get current date and time */
    time(&current);
    dt = localtime(&current);

    off1 = sprintf(log_buffer,
            "[%04d-%02d-%02d %02d:%02d:%02d] %s: ",
            dt->tm_year + 1900,
            dt->tm_mon + 1,
            dt->tm_mday,
            dt->tm_hour,
            dt->tm_min,
            dt->tm_sec,
            error_titles[level]);

    va_start(al, fmt);
    off2 = vsprintf(log_buffer + off1, fmt, al);
    va_end(al);

    log_buffer[off1 + off2] = '\n';

    write(log_fd, log_buffer, off1 + off2 + 1);
}

void log_destroy()
{
    if (!log_initialize) {
        return;
    }

    if (log_fd && log_fd != STDERR_FILENO) {
        close(log_fd);
    }
}
