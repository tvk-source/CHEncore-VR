#include "gvr_stats.h"

void gvr_stats_init(gvr_stats *s)
{
    uint64_t *f = (uint64_t *)s;
    for (size_t i = 0; i < sizeof *s / sizeof *f; i++)
        f[i] = GVR_UNKNOWN;
}

typedef struct {
    char *buf;
    size_t size, len;
} out;

static void put(out *o, char c)
{
    if (o->len + 1 < o->size)
        o->buf[o->len] = c;
    o->len++;
}

static void puts_(out *o, const char *s)
{
    while (*s)
        put(o, *s++);
}

static void putu(out *o, uint64_t v)
{
    char d[20];
    int n = 0;
    do
        d[n++] = (char)('0' + v % 10);
    while (v /= 10);
    while (n)
        put(o, d[--n]);
}

/* v thousandths as v/1000 with one decimal, rounded. */
static void putmilli(out *o, uint64_t v)
{
    v = (v + 50) / 100;
    putu(o, v / 10);
    put(o, '.');
    put(o, (char)('0' + v % 10));
}

static void field(out *o, const char *name, uint64_t v)
{
    put(o, ' ');
    puts_(o, name);
    put(o, ' ');
    if (v == GVR_UNKNOWN)
        put(o, '-');
    else
        putu(o, v);
}

size_t gvr_stats_format(char *buf, size_t size, const gvr_stats *s)
{
    out o = {buf, size, 0};
    puts_(&o, "gvr: stats ");
    if (s->window_ms == GVR_UNKNOWN)
        put(&o, '-');
    else
        putmilli(&o, s->window_ms);
    puts_(&o, "s ");
    if (s->refresh_hz == GVR_UNKNOWN)
        put(&o, '-');
    else
        putu(&o, s->refresh_hz);
    puts_(&o, "Hz");
    field(&o, "refreshes", s->refreshes);
    field(&o, "handed", s->handed);
    field(&o, "again", s->again);
    field(&o, "never", s->never);
    field(&o, "late", s->late);
    field(&o, "qfull", s->qfull);
    field(&o, "err", s->err);
    puts_(&o, " gpu ");
    if (s->gpu_us == GVR_UNKNOWN)
        put(&o, '-');
    else {
        putmilli(&o, s->gpu_us);
        puts_(&o, "ms");
    }
    if (size)
        buf[o.len < size ? o.len : size - 1] = 0;
    return o.len < size ? o.len : (size ? size - 1 : 0);
}
