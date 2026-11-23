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
    // Optional original sprite tint. The ordinary cinema remains white.
    float color[4]{1.f,1.f,1.f,1.f};
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
// Render ONLY the original game's peripheral HUD sections, never the flat
// gameplay image itself. These cropped panels retain the original animated
// power/health meters, score glyphs and UI without introducing a second neck.
// They share the recentered world anchor and project separately through each eye.
struct CinemaHudPanel {
    float x0{},x1{},y0{},y1{},z{};
    float u0{},v0{},u1{},v1{};
};
inline std::optional<std::array<CinemaVertex,6>> cinemaHudPanelVertices(
    const CinemaScreen& screen,const Eye& eye,const CinemaHudPanel& panel)
{
    if(!validTrackedPose(screen.pose)||!validTrackedPose(eye.pose)||
       !std::isfinite(panel.x0)||!std::isfinite(panel.x1)||
       !std::isfinite(panel.y0)||!std::isfinite(panel.y1)||!std::isfinite(panel.z)||
       !std::isfinite(panel.u0)||!std::isfinite(panel.u1)||
       !std::isfinite(panel.v0)||!std::isfinite(panel.v1)||
       !(panel.x0<panel.x1)||!(panel.y0<panel.y1)||
       panel.u0<0.f||panel.u1>1.f||!(panel.u0<panel.u1)||
       panel.v0<0.f||panel.v1>1.f||!(panel.v0<panel.v1))return {};
    const auto proj=projection(eye.fov,.05f,50.f);
    if(!proj)return {};
    const auto camera=view(eye.pose);
    const float xy[4][2]={
        {panel.x0,panel.y1},{panel.x1,panel.y1},
        {panel.x1,panel.y0},{panel.x0,panel.y0}};
    const float uv[4][2]={
        {panel.u0,panel.v0},{panel.u1,panel.v0},
        {panel.u1,panel.v1},{panel.u0,panel.v1}};
    constexpr unsigned indices[6]={0,1,2,0,2,3};
    std::array<CinemaVertex,6> vertices{};
    for(unsigned k=0;k<6;++k){
        const unsigned i=indices[k];
        const Vec local{xy[i][0],xy[i][1],panel.z};
        const Vec world=screen.pose.pos+rotate(screen.pose.rot,local);
        const auto clip=cinemaTransform(*proj,cinemaTransform(camera,{world.x,world.y,world.z,1.f}));
        for(float value:clip)if(!std::isfinite(value))return {};
        if(clip[3]<.06f)return {};
        vertices[k]={clip,uv[i][0],uv[i][1]};
    }
    return vertices;
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
