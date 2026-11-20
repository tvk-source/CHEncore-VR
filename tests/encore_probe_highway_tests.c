/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_highway.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    const float fov[]={1.8418f,.9472f,1.329f,1.329f};
    for(int s=0;s<2;++s){
        encore_probe_eye e;
        assert(encore_probe_eye_init(&e,fov,.059f,s));
        float u,v;
        float y=-.48f,w=.66f-.36f*(y+.62f)/1.24f;
        encore_probe_project(&e,-.8f*w,y,&u,&v);
        assert(encore_highway_pixel(&e,u,v,0)==UINT32_C(0xff40d040));
        assert(encore_highway_pixel(&e,u,v,1)==UINT32_C(0xfff0f0f0));
        assert(encore_highway_pixel(&e,u,v,2)==UINT32_C(0xff40d040));
        assert(encore_highway_pixel(&e,u,v,3)==UINT32_C(0xfff0f0f0));
        encore_probe_project(&e,.8f*w,y,&u,&v);
        assert(encore_highway_pixel(&e,u,v,0)==UINT32_C(0xff40a0f0));
        assert(encore_highway_pixel(&e,u,v,2)==UINT32_C(0xfff0f0f0));
        assert(encore_highway_pixel(&e,u,v,1)==UINT32_C(0xff40a0f0));
        assert(encore_highway_pixel(&e,u,v,3)==UINT32_C(0xfff0f0f0));
    }
    puts("PASS binocular highway input variants, left/right/both independent");
    return 0;
}
