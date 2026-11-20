/* The one frame-stats line both tracks log every 10 s (PLAN S6), so a console run on
 * either track is judged by the same numbers (1h targets):
 *
 *   gvr: stats 10.0s 120Hz refreshes 1200 handed 600 again 0 never 0 late 0 qfull 0 err 0 gpu 0.9ms
 *
 * handed: frames given to Hmd2; again: refreshes that showed a frame a second time
 * without the design asking for it; never: frames handed over but never shown;
 * late: frames that missed their refresh; qfull: Hmd2 queue-full waits; err: Hmd2
 * errors; gpu: our own GPU time per frame. A count the track can't measure yet is
 * GVR_UNKNOWN and prints as "-". */
#ifndef GVR_STATS_H
#define GVR_STATS_H

#include <stddef.h>
#include <stdint.h>

#define GVR_UNKNOWN UINT64_MAX

typedef struct {
    uint64_t window_ms;  /* length of the window these counts cover */
    uint64_t refresh_hz; /* headset refresh */
    uint64_t refreshes, handed, again, never, late, qfull, err;
    uint64_t gpu_us;     /* our GPU time per frame, in microseconds */
} gvr_stats;

/* Starts every field as unknown. */
void gvr_stats_init(gvr_stats *s);
/* Writes the line (no newline) into buf; returns its length, cut to size - 1. */
size_t gvr_stats_format(char *buf, size_t size, const gvr_stats *s);

#endif
