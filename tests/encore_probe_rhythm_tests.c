/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_rhythm.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    encore_rhythm s;encore_rhythm_init(&s);
    assert(fabsf(encore_rhythm_distance(&s,0)-.2f)<.0001f);
    assert(encore_rhythm_tick(&s,380000,1)); /* inclusive early boundary */
    assert(s.hits==1&&s.misses==0);
    assert(encore_rhythm_tick(&s,500000,1)&&s.hits==1);
    assert(encore_rhythm_tick(&s,900000,0));
    assert(encore_rhythm_tick(&s,1120000,2)&&s.hits==2); /* inclusive late */
    assert(encore_rhythm_tick(&s,1200000,0));
    assert(encore_rhythm_tick(&s,1250000,4)&&s.early==1);
    assert(encore_rhythm_tick(&s,1620001,0)&&s.misses==1);
    assert(!encore_rhythm_tick(&s,1620000,8)&&s.last_us==1620001);
    encore_rhythm_init(&s);
    assert(encore_rhythm_tick(&s,10000000,0)&&s.misses==19);
    assert(encore_rhythm_tick(&s,10000000,16)&&s.hits==1);
    puts("PASS five-lane rhythm clock, boundaries, held input, missed beats, monotonicity");
}
