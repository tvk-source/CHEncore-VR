/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_retirement.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    encore_live_slot s[4]={{0}};
    assert(encore_live_choose(s,0,0,100000,0)==-1);
    assert(encore_live_choose(s,10,10,100000,-1)==-1);
    assert(encore_live_choose(s,11,10,100000,0)==-1);
    assert(encore_live_choose(s,8,10,100000,0)==-1);
    assert(encore_live_choose(s,10,10,100000,0)==0);
    s[0]=(encore_live_slot){10,100000};
    assert(encore_live_choose(s,10,10,100000,0)==1);
    assert(!encore_live_eligible(&s[0],11,11,200000,0));
    assert(!encore_live_eligible(&s[0],12,12,133333,0));
    assert(encore_live_eligible(&s[0],12,12,133334,0));
    assert(!encore_live_eligible(&s[0],12,12,99999,0));
    s[1]=(encore_live_slot){11,116667};s[2]=(encore_live_slot){12,133334};s[3]=(encore_live_slot){13,150001};
    assert(encore_live_choose(s,13,13,150002,0)==0);
    s[0]=(encore_live_slot){14,166668};
    assert(encore_live_choose(s,14,14,166669,0)==1);
    for(int i=0;i<4;++i)s[i]=(encore_live_slot){14,166668};
    assert(encore_live_choose(s,14,14,166669,0)==-1);
    puts("PASS experimental four-slot flip/age gate, stale status and fail-closed rotation");
    return 0;
}
