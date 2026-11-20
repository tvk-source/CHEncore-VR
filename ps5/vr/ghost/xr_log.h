/* PS5XR logging: klog (through standard error) plus a ring the debug server
 * can return. */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void ps5xr_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
/* Copies the newest log text into buf (NUL-terminated); returns its length. */
size_t ps5xr_log_tail(char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#define XRLOG(...) ps5xr_log(__VA_ARGS__)
