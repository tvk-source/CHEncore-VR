// SPDX-License-Identifier: MIT
// Platform-independent cinema screen geometry. Does not own GPU or display buffers.
#pragma once
#include "vr_core.hpp"
namespace encore::vr {
struct CinemaScreen {
    Pose pose;
    float width{},height{};
};
struct CinemaVertex {
    std::array<float,4> clip;
    float u{},v{};
};
inline bool validTrackedPose(Pose p) {
    const auto q=p.rot;
    const float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return p.tracked&&finite(p.pos)&&std::isfinite(n)&&n>.98f&&n<1.02f;
}
// Use yaw only: looking down during recenter must not tilt the screen.
// Face the seated viewer; preserve world-space position until explicit recenter.
inline std::optional<CinemaScreen> cinemaScreen(Pose head,float width,float aspect,float distance) {
    if(!validTrackedPose(head)||!std::isfinite(width)||!std::isfinite(aspect)||!std::isfinite(distance)||
        width<.5f||width>6.f||aspect<.5f||aspect>4.f||distance<1.f||distance>8.f)return {};
    const auto forward=rotate(head.rot,{0,0,-1});
    if(forward.x*forward.x+forward.z*forward.z<.01f)return {};
    const auto origin=recenter(head);
    return CinemaScreen{{head.pos+rotate(origin.yaw,{0,0,-distance}),origin.yaw,true},width,width/aspect};
}
inline std::array<float,4> cinemaTransform(const Mat& m,const std::array<float,4>& v) {
    std::array<float,4> out{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)out[r]+=m[c*4+r]*v[c];
    return out;
}
// Leave perspective division and clipping to the graphics pipeline. UV (0,0)
// is the top left of the source game image. Triangles share identical vertices.
inline std::optional<std::array<CinemaVertex,6>> cinemaVertices(const CinemaScreen& s,const Eye& eye) {
    if(!validTrackedPose(s.pose)||!validTrackedPose(eye.pose)||!std::isfinite(s.width)||
       !std::isfinite(s.height)||s.width<=0||s.height<=0)return {};
    auto proj=projection(eye.fov,.05f,50.f);
    if(!proj)return {};
    const Mat camera=view(eye.pose);
    const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    constexpr unsigned indices[6]={0,1,2,0,2,3};
    std::array<CinemaVertex,6> vertices{};
    for(unsigned k=0;k<6;++k){
        const unsigned i=indices[k];
        const Vec local{(uv[i][0]-.5f)*s.width,(.5f-uv[i][1])*s.height,0};
        const Vec world=s.pose.pos+rotate(s.pose.rot,local);
        auto clip=cinemaTransform(*proj,cinemaTransform(camera,{world.x,world.y,world.z,1}));
        for(float v:clip)if(!std::isfinite(v))return {};
        vertices[k]={clip,uv[i][0],uv[i][1]};
    }
    return vertices;
}
}
