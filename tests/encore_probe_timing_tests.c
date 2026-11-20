/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_timing.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    encore_origin_gate g={0};
    float p[3]={-.0808f,-2.1355f,-.0023f},q[4]={0,0,0,1};
    for(unsigned i=0;i<200;++i)assert(!encore_origin_sample(&g,i*10000u,p,q));
    p[1]=1.1644f;
    assert(!encore_origin_sample(&g,3000000,p,q));
    for(unsigned i=1;i<45;++i)assert(!encore_origin_sample(&g,3000000+i*11111u,p,q));
    assert(encore_origin_sample(&g,3500000,p,q));
    assert(!encore_origin_sample(&g,3511111,p,q));
    encore_origin_gate bad={0};
    p[0]=NAN;assert(!encore_origin_sample(&bad,5000000,p,q));
    assert(!bad.have_prev);
    encore_flip_observation s={0};
    assert(!encore_flip_sample(&s,-1,1,2));
    assert(!encore_flip_sample(&s,0,0,2));
    assert(!encore_flip_sample(&s,0,3,2));
    assert(encore_flip_sample(&s,0,1,2));
    assert(encore_flip_sample(&s,0,3,5));
    assert(!encore_flip_sample(&s,0,2,6));
    assert(s.valid==2&&s.invalid==3&&s.regressed==1&&s.max_lag==2);
    puts("PASS delayed origin candidate and conservative flip marker observations");
}
