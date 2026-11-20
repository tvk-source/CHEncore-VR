/* PS5XR logging: one line to standard error (the title sends it to klog) and
 * into a 64 KiB ring for the debug server's "log" command. */
#include "xr_log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define RING 65536

static char ring[RING];
static size_t head;     /* next write position */
static int wrapped;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void ps5xr_log(const char *format, ...)
{
    char line[1024];
    int n = snprintf(line, sizeof line, "[ps5xr] ");
    va_list args;
    va_start(args, format);
    vsnprintf(line + n, sizeof line - (size_t)n - 1, format, args);
    va_end(args);
    size_t len = strlen(line);
    line[len++] = '\n';
    line[len] = 0;
    fputs(line, stderr);
    fflush(stderr);
    pthread_mutex_lock(&lock);
    for (size_t i = 0; i < len; i++) {
        ring[head++] = line[i];
        if (head == RING) {
            head = 0;
            wrapped = 1;
        }
    }
    pthread_mutex_unlock(&lock);
}

size_t ps5xr_log_tail(char *buf, size_t size)
{
    if (!size)
        return 0;
    pthread_mutex_lock(&lock);
    size_t avail = wrapped ? RING : head;
    size_t want = avail < size - 1 ? avail : size - 1;
    size_t start = (head + RING - want) % RING;
    for (size_t i = 0; i < want; i++)
        buf[i] = ring[(start + i) % RING];
    pthread_mutex_unlock(&lock);
    buf[want] = 0;
    return want;
}
