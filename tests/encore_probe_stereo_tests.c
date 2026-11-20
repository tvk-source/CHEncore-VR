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
    assert(ENCORE_PROBE_FRAMES==2700 && ENCORE_PROBE_DEADLINE_US==45000000);
    const float bads[]={NAN, INFINITY, -INFINITY, 0.f};
    for(unsigned i=0;i<sizeof(bads)/sizeof(bads[0]);++i) {
        float invalid[4]={bads[i],.9472f,1.329f,1.329f};
        assert(!encore_probe_eye_init(&e[0],invalid,.059f,0));
        assert(!encore_probe_eye_init(&e[0],fov,bads[i],0));
    }
    assert(encore_probe_eye_init(&e[0],fov,.059f,0));
    float disparity[3];
    const float depths[]={1.f,2.f,4.f};
    for(int k=0;k<3;++k) {
        float a[2],b;
        for(int s=0;s<2;++s) {
            encore_probe_eye plane=e[s];
            plane.depth=depths[k];
            encore_probe_project(&plane,0.f,0.f,&a[s],&b);
        }
        disparity[k]=a[0]-a[1]-(fov[0]-fov[1])/(fov[0]+fov[1]);
        assert(fabsf(disparity[k]-.059f/(depths[k]*(fov[0]+fov[1])))<1e-5f);
    }
    assert(disparity[0]>disparity[1] && disparity[1]>disparity[2]);
    for(int s=0;s<2;++s) {
        encore_probe_eye plane=e[s];
        float a,b;
        plane.depth=1.f;
        encore_probe_project(&plane,-.48f,.13f,&a,&b);
        assert(encore_probe_scene_pixel(&e[s],a,b)==UINT32_C(0xff808080));
        plane.depth=2.f;
        encore_probe_project(&plane,0.f,0.f,&a,&b);
        assert(encore_probe_scene_pixel(&e[s],a,b)==UINT32_C(0xff808080));
        plane.depth=4.f;
        encore_probe_project(&plane,1.92f,.52f,&a,&b);
        assert(encore_probe_scene_pixel(&e[s],a,b)==UINT32_C(0xff808080));
    }
    puts("PASS asymmetric stereo reference, IPD, three depths and invalid optics rejection");
    return 0;
}
