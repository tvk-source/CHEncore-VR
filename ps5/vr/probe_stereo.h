/* SPDX-License-Identifier: GPL-3.0-or-later
 * Static optical reference plane. Parallel eye axes assumed; not a tracked scene.
 */
#pragma once
#include <math.h>
#include <stdint.h>
enum { ENCORE_PROBE_FRAMES = 2700, ENCORE_PROBE_DEADLINE_US = 34000000 };
typedef struct {
    float left, right, up, down, eye_x, depth;
} encore_probe_eye;

static inline int encore_probe_eye_init(encore_probe_eye *e, const float fov[4],
                                        float ipd, int side)
{
    if (!e || !fov || (side != 0 && side != 1) ||
        !(ipd >= .04f && ipd <= .09f))
        return 0;
    for (int i=0; i<4; ++i)
        if (!(fov[i] > 0.f && fov[i] <= 8.f))
            return 0;
    e->left = fov[side];
    e->right = fov[1-side];
    e->up = fov[2];
    e->down = fov[3];
    e->eye_x = (side ? .5f : -.5f)*ipd;
    e->depth = 2.f;
    return 1;
}
static inline void encore_probe_project(const encore_probe_eye *e,
                                        float x, float y, float *u, float *v)
{
    *u = ((x-e->eye_x)/e->depth + e->left)/(e->left+e->right);
    *v = (e->up-y/e->depth)/(e->up+e->down);
}
static inline void encore_probe_unproject(const encore_probe_eye *e,
                                          float u, float v, float *x, float *y)
{
    *x = e->eye_x + e->depth*(u*(e->left+e->right)-e->left);
    *y = e->depth*(e->up-v*(e->up+e->down));
}
static inline uint32_t encore_probe_pixel(const encore_probe_eye *e, float u, float v)
{
    float x,y;
    encore_probe_unproject(e,u,v,&x,&y);
    float r2=x*x+y*y;
    int cross=(fabsf(x)<=.012f && fabsf(y)<=.20f) ||
              (fabsf(y)<=.012f && fabsf(x)<=.20f);
    int ring=r2>=.28f*.28f && r2<=.30f*.30f;
    return (cross || ring) ? UINT32_C(0xff808080) : UINT32_C(0xff202020);
}

// Static head-relative scene: near square (1 m), original ring (2 m),
// far diamond (4 m). Equal angular sizes approximately, distinct shapes.
// Render from nearest to farthest. No in-flight buffer updates.
static inline uint32_t encore_probe_scene_pixel(const encore_probe_eye *eye, float u, float v)
{
    encore_probe_eye e = *eye;
    float x,y;
    e.depth=1.f;
    encore_probe_unproject(&e,u,v,&x,&y);
    float ax=fabsf(x+.48f), ay=fabsf(y);
    if (ax<=.14f && ay<=.14f && (ax>=.115f || ay>=.115f))
        return UINT32_C(0xff808080);
    uint32_t mid=encore_probe_pixel(eye,u,v);
    if (mid!=UINT32_C(0xff202020)) return mid;
    e.depth=4.f;
    encore_probe_unproject(&e,u,v,&x,&y);
    float d=fabsf(x-1.92f)+fabsf(y);
    if (d>=.48f && d<=.56f) return UINT32_C(0xff808080);
    return UINT32_C(0xff202020);
}
