/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "probe_stereo.h"
#include "ghost/psvr2.h"
static inline int encore_pose_valid(const psvr2_pose *p) {
    float n=0.f;
    for(int i=0;i<3;++i)if(!(p->pos[i]>=-100.f&&p->pos[i]<=100.f))return 0;
    for(int i=0;i<4;++i)n+=p->quat[i]*p->quat[i];
    return n>.98f&&n<1.02f;
}
static inline void encore_rotate(const float q[4],const float v[3],float out[3]) {
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    const float a=2*(y*v[2]-z*v[1]),b=2*(z*v[0]-x*v[2]),c=2*(x*v[1]-y*v[0]);
    out[0]=v[0]+w*a+y*c-z*b;
    out[1]=v[1]+w*b+z*a-x*c;
    out[2]=v[2]+w*c+x*b-y*a;
}
static inline void encore_world_anchor(const psvr2_pose *head,float points[3][3]) {
    const float local[3][3]={{-.48f,.001f,-.988f},{0.f,.001f,-1.988f},{1.92f,.001f,-3.988f}};
    for(int j=0;j<3;++j){
        encore_rotate(head->quat,local[j],points[j]);
        for(int k=0;k<3;++k)points[j][k]+=head->pos[k];
    }
}
/* Project through the actual tracked eye, never subtract IPD twice.
 * Return 1 visible, 0 clipped, -1 invalid pose/point. */
static inline int encore_world_project(const encore_probe_eye *optical,const psvr2_pose *eye,
                                       const float point[3],float uv[2]) {
    if(!encore_pose_valid(eye))return -1;
    float d[3],v[3];
    for(int k=0;k<3;++k){
        if(!(point[k]>=-110.f&&point[k]<=110.f))return -1;
        d[k]=point[k]-eye->pos[k];
    }
    const float inv[4]={-eye->quat[0],-eye->quat[1],-eye->quat[2],eye->quat[3]};
    encore_rotate(inv,d,v);
    if(v[2]>=-.1f)return 0;
    uv[0]=(v[0]/(-v[2])+optical->left)/(optical->left+optical->right);
    uv[1]=(optical->up-v[1]/(-v[2]))/(optical->up+optical->down);
    return uv[0]>.02f&&uv[0]<.98f&&uv[1]>.02f&&uv[1]<.98f;
}
/* 5 seconds warmup, 1 second 120Hz regression, 30 seconds live. */
static inline int encore_batch_phase(int frame) {
    return frame<=450?0:frame<=570?1:2;
}
static inline int encore_batch_end(int phase) {
    return phase==0?450:phase==1?570:ENCORE_PROBE_FRAMES;
}
