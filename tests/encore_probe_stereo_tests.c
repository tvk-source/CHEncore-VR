/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_stereo.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const float fov[4]={1.8418f,.9472f,1.3290f,1.3290f};
    encore_probe_eye e[2];
    assert(encore_probe_eye_init(&e[0],fov,.059f,0));
    assert(encore_probe_eye_init(&e[1],fov,.059f,1));
    float u[2],v[2];
    for(int s=0;s<2;++s) {
        encore_probe_project(&e[s],0.f,0.f,&u[s],&v[s]);
        assert(fabsf(v[s]-.5f)<1e-5f);
        assert(encore_probe_pixel(&e[s],u[s],v[s])==UINT32_C(0xff808080));
        assert(encore_probe_pixel(&e[s],.5f,.5f)==UINT32_C(0xff202020));
        float a,b,x,y;
        encore_probe_project(&e[s],.29f,0.f,&a,&b);
        assert(encore_probe_pixel(&e[s],a,b)==UINT32_C(0xff808080));
        encore_probe_project(&e[s],.12f,.16f,&a,&b);
        encore_probe_unproject(&e[s],a,b,&x,&y);
        assert(fabsf(x-.12f)<1e-5f && fabsf(y-.16f)<1e-5f);
        assert(encore_probe_pixel(&e[s],a,b)==UINT32_C(0xff202020));
    }
    assert(fabsf(u[0]-.66567f)<.0001f);
    assert(fabsf(u[1]-.33433f)<.0001f);
    assert(fabsf(u[0]+u[1]-1.f)<1e-5f);
    assert(!encore_probe_eye_init(&e[0],fov,NAN,0));
    assert(!encore_probe_eye_init(&e[0],fov,.059f,2));
    assert(ENCORE_PROBE_FRAMES==2700 && ENCORE_PROBE_DEADLINE_US==34000000);
    puts("PASS asymmetric stereo reference plane, IPD, inverse projection and duration");
    return 0;
}
