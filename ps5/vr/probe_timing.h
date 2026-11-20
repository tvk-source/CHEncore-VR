/* SPDX-License-Identifier: GPL-3.0-or-later
 * Conservative origin-candidate and flip telemetry. Neither grants ownership.
 */
#pragma once
#include <stdint.h>
#include <math.h>
typedef struct {
    float prev_p[3], prev_q[4];
    unsigned stable_samples;
    int have_prev, anchored;
} encore_origin_gate;
static inline int encore_origin_sample(encore_origin_gate *g,uint64_t elapsed,
                                       const float p[3],const float q[4])
{
    float norm=0;
    for(int k=0;k<3;++k)if(!(p[k]>=-100.f&&p[k]<=100.f)){
        g->have_prev=0;g->stable_samples=0;return 0;
    }
    for(int k=0;k<4;++k)norm+=q[k]*q[k];
    if(!(norm>.98f&&norm<1.02f)){g->have_prev=0;g->stable_samples=0;return 0;}
    float d2=0,dot=0;
    for(int k=0;k<3;++k){float d=p[k]-g->prev_p[k];d2+=d*d;g->prev_p[k]=p[k];}
    for(int k=0;k<4;++k){dot+=q[k]*g->prev_q[k];g->prev_q[k]=q[k];}
    if(elapsed>=3000000u&&g->have_prev&&d2<=.02f*.02f&&fabsf(dot)>=.999f){
        if(g->stable_samples<45)++g->stable_samples;
    }else g->stable_samples=0;
    g->have_prev=1;
    if(!g->anchored&&g->stable_samples>=45){g->anchored=1;return 1;}
    return 0;
}
typedef struct {
    uint64_t last,max_lag,valid,invalid,regressed;
} encore_flip_observation;
static inline int encore_flip_sample(encore_flip_observation *s,int rc,
                                     uint64_t shown,uint64_t submitted)
{
    if(rc||!shown||shown>submitted){++s->invalid;return 0;}
    if(s->valid&&shown<s->last){++s->regressed;return 0;}
    ++s->valid;s->last=shown;
    uint64_t lag=submitted-shown;
    if(lag>s->max_lag)s->max_lag=lag;
    return 1;
}
