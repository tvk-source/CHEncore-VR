/* SPDX-License-Identifier: GPL-3.0-or-later
 * Diagnostic five-lane panel, not the CHEncore renderer. Four immutable
 * variants keyed by trigger bits. All drawing occurs before registration.
 */
#pragma once
#include "probe_stereo.h"
static inline uint32_t encore_highway_pixel(const encore_probe_eye *eye,float u,float v,unsigned variant) {
    float x,y;encore_probe_unproject(eye,u,v,&x,&y);
    if(y<-.62f||y>.62f)return UINT32_C(0xff181818);
    float t=(y+.62f)/1.24f;
    float w=.66f-.36f*t;
    if(fabsf(x)>w)return UINT32_C(0xff181818);
    float lane=(x+w)/(2*w)*5.f;
    int l=(int)lane;if(l<0||l>4)return UINT32_C(0xff181818);
    float f=lane-l;
    const uint32_t color[5]={UINT32_C(0xff40d040),UINT32_C(0xff4040df),UINT32_C(0xff40d7df),UINT32_C(0xffdf7040),UINT32_C(0xff40a0f0)};
    if(f<.025f||f>.975f)return UINT32_C(0xffb0b0b0);
    if(y<-.43f&&y>-.53f&&f>.10f&&f<.90f){
        if((l==0&&(variant&1))||(l==4&&(variant&2)))return UINT32_C(0xfff0f0f0);
        return color[l];
    }
    const float note_y[5]={-.12f,.06f,.27f,.42f,.15f};
    if(f>.18f&&f<.82f&&fabsf(y-note_y[l])<.045f)return color[l];
    if(y<-.39f&&y>-.41f)return UINT32_C(0xffb0b0b0);
    return UINT32_C(0xff303030);
}
