/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free, five-lane rhythm clock. Pure code; no compositor ownership.
 * Each lane has one beat per 2.5s, staggered by 500ms.
 */
#pragma once
#include <stdint.h>
enum { ENCORE_RHYTHM_LANES=5, ENCORE_RHYTHM_PERIOD_US=2500000,
       ENCORE_RHYTHM_STEP_US=500000, ENCORE_RHYTHM_WINDOW_US=120000 };
typedef struct {
    uint64_t next[ENCORE_RHYTHM_LANES], last_us;
    unsigned previous, hits, misses, early;
} encore_rhythm;
static inline void encore_rhythm_init(encore_rhythm *s) {
    *s=(encore_rhythm){0};
    for(unsigned i=0;i<ENCORE_RHYTHM_LANES;++i)s->next[i]=(i+1)*ENCORE_RHYTHM_STEP_US;
}
/* Monotonic song clock, lane rising edges only. Held notes never repeat. */
static inline int encore_rhythm_tick(encore_rhythm *s,uint64_t us,unsigned lanes) {
    if(us<s->last_us)return 0;
    unsigned rising=(lanes&31u)&~s->previous;
    for(unsigned i=0;i<ENCORE_RHYTHM_LANES;++i){
        if(us>s->next[i] && us-s->next[i]>ENCORE_RHYTHM_WINDOW_US){
            uint64_t skipped=1+(us-s->next[i]-ENCORE_RHYTHM_WINDOW_US-1)/ENCORE_RHYTHM_PERIOD_US;
            s->misses+=(unsigned)skipped;
            s->next[i]+=skipped*ENCORE_RHYTHM_PERIOD_US;
        }
        if(rising&(1u<<i)){
            uint64_t distance=us>s->next[i]?us-s->next[i]:s->next[i]-us;
            if(distance<=ENCORE_RHYTHM_WINDOW_US){++s->hits;s->next[i]+=ENCORE_RHYTHM_PERIOD_US;}
            else ++s->early;
        }
    }
    s->previous=lanes&31u;s->last_us=us;return 1;
}
/* Note distance in travel periods; negative until the late window expires. */
static inline float encore_rhythm_distance(const encore_rhythm *s,unsigned lane) {
    if(lane>=ENCORE_RHYTHM_LANES)return 0.f;
    double delta=(double)s->next[lane]-(double)s->last_us;
    return (float)(delta/(double)ENCORE_RHYTHM_PERIOD_US);
}
