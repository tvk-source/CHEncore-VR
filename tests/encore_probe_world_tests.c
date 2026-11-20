/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../ps5/vr/probe_world.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    const float fov[4]={1.8418f,.9472f,1.329f,1.329f};
    encore_probe_eye optical[2];
    psvr2_pose head={{0,0,0},{0,0,0,1}};
    psvr2_pose eyes[2]={{{-.0295f,.001f,.012f},{0,0,0,1}},{{.0295f,.001f,.012f},{0,0,0,1}}};
    float points[3][3],uv[2][2];
    encore_world_anchor(&head,points);
    for(int s=0;s<2;++s){
        assert(encore_probe_eye_init(&optical[s],fov,.059f,s));
        assert(encore_world_project(&optical[s],&eyes[s],points[1],uv[s])==1);
        float u,v;encore_probe_project(&optical[s],0,0,&u,&v);
        assert(fabsf(u-uv[s][0])<1e-5f&&fabsf(v-uv[s][1])<1e-5f);
        float old=uv[s][0];
        eyes[s].pos[0]+=.2f;
        assert(encore_world_project(&optical[s],&eyes[s],points[1],uv[s])==1&&uv[s][0]<old);
        eyes[s].quat[1]=1;eyes[s].quat[3]=0;
        assert(encore_world_project(&optical[s],&eyes[s],points[1],uv[s])==0);
        eyes[s].quat[0]=NAN;
        assert(encore_world_project(&optical[s],&eyes[s],points[1],uv[s])==-1);
    }
    assert(encore_batch_phase(1)==0&&encore_batch_phase(450)==0);
    assert(encore_batch_phase(451)==1&&encore_batch_phase(570)==1);
    assert(encore_batch_phase(571)==2&&encore_batch_end(2)==2370);
    puts("PASS tracked world projection, motion parallax, clipping, invalid pose and batch boundaries");
}
