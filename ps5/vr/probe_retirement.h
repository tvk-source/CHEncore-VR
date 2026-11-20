/* SPDX-License-Identifier: GPL-3.0-or-later
 * Experimental read-only flip-age gate. NOT a compositor release fence.
 * Only for the bounded 60 Hz submission experiment. No deallocation.
 */
#pragma once
#include <stdint.h>
enum { ENCORE_LIVE_SLOTS=4, ENCORE_LIVE_HOLDOFF_US=33334 };
typedef struct { uint64_t frame, display_us; } encore_live_slot;
static inline int encore_live_observation(uint64_t shown,uint64_t submitted,int rc){
    return rc==0&&shown>0&&shown<=submitted&&submitted-shown<=1;
}
static inline int encore_live_eligible(const encore_live_slot *s,uint64_t shown,
                                      uint64_t submitted,uint64_t now,int rc){
    if(!encore_live_observation(shown,submitted,rc))return 0;
    if(!s->frame)return 1;
    return shown>=s->frame&&shown-s->frame>=2&&s->display_us>0&&
           now>=s->display_us&&now-s->display_us>=ENCORE_LIVE_HOLDOFF_US;
}
static inline int encore_live_choose(const encore_live_slot slots[4],uint64_t shown,
                                     uint64_t submitted,uint64_t now,int rc){
    int best=-1;
    for(int i=0;i<ENCORE_LIVE_SLOTS;++i)
        if(encore_live_eligible(&slots[i],shown,submitted,now,rc)&&
           (best<0||slots[i].frame<slots[best].frame))best=i;
    return best;
}
