// SPDX-License-Identifier: MIT
// Pure CPU-side live chart geometry for the first real stereo highway.
// A chart snapshot comes from the original game, not from a demo clock.
#pragma once
#include "cinema_geometry.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace encore::vr {
inline constexpr uint32_t kStereoHighwayNotes=96;
inline constexpr uint32_t kStereoHighwayMaxVertices=4096;
inline constexpr uint32_t kStereoAssetMaxVertices=4096; // per texture, before eye duplication
struct StereoSourceVertex {
    float pos[3]{};
    float uv[2]{};
    float color[4]{1.f,1.f,1.f,1.f};
    float uvNext[2]{};
    float frameBlend{};
};
struct StereoHighwayNote {
    uint8_t lane{};         // 0..4 = G R Y B O
    float untilHit{};       // seconds on the game's rendered/audio-calibrated timeline
    float sustain{};        // seconds remaining to tail, non-negative
};
struct StereoHighwayFrame {
    bool active{};          // only live, unpaused five-fret gameplay
    uint32_t count{};
    std::array<StereoHighwayNote,kStereoHighwayNotes> notes{};
    float sourceFar{39.35f}; // source game's visible world-space depth
    std::vector<StereoSourceVertex> board{};   // PC's scrolling highway board
    std::vector<StereoSourceVertex> highway{}; // PC highway texture UVs/geometry
    std::vector<StereoSourceVertex> sustain{}; // Source-authored sustain ribbon/UVs
    std::vector<StereoSourceVertex> guitar{};  // PC guitar atlas: gems/receptors
    std::vector<StereoSourceVertex> effects{}; // Original animated guitar hit effects
};
struct StereoHighwayDraw {
    uint32_t first[2]{},count[2]{};
    uint32_t chartNotes{};  // number of live chart gems represented
};
// A solid 3D deck plus actual 3D gem positions, each independently projected
// through the left/right HMD eye, without head-locked billboard shortcuts.
// UV.x encodes a shader palette index; UV.y encodes brightness.
inline bool stereoHighwayGeometry(
    const StereoHighwayFrame& frame,const CinemaScreen& screen,
    const Eye (&eyes)[2],std::vector<CinemaVertex>& vertices,StereoHighwayDraw& draw)
{
    draw={};
    if(!frame.active)return true;
    if(frame.count>kStereoHighwayNotes||!validTrackedPose(screen.pose))return false;
    constexpr float nearZ=.76f,farZ=.05f;
    constexpr float leftEdge=-.85f,rightEdge=.85f;
    for(int eye=0;eye<2;++eye){
        if(!validTrackedPose(eyes[eye].pose))return false;
        const auto proj=projection(eyes[eye].fov,.05f,50.f);
        if(!proj)return false;
        const auto camera=view(eyes[eye].pose);
        draw.first[eye]=static_cast<uint32_t>(vertices.size());
        auto emit=[&](float x,float along,float elevation,int palette,float intensity)->bool {
            // along=0 is the strike zone, 1 is the distant tip.
            const Vec local{x,-.74f+.69f*along+elevation,
                            nearZ+(farZ-nearZ)*along};
            const Vec world=screen.pose.pos+rotate(screen.pose.rot,local);
            const auto clip=cinemaTransform(*proj,
                cinemaTransform(camera,{world.x,world.y,world.z,1.f}));
            for(float c:clip)if(!std::isfinite(c))return false;
            if(clip[3]<.06f)return false;
            vertices.push_back({clip,float(palette),intensity});
            return vertices.size()<=kStereoHighwayMaxVertices;
        };
        auto quad=[&](float x0,float x1,float a0,float a1,
                      int palette,float intensity,float elevation)->bool {
            const float v[6][2]={{x0,a0},{x1,a0},{x1,a1},
                                  {x0,a0},{x1,a1},{x0,a1}};
            for(const auto& xy:v)if(!emit(xy[0],xy[1],elevation,palette,intensity))return false;
            return true;
        };
        // Opaque charcoal deck, visually in front of the original 2D source.
        if(!quad(leftEdge,rightEdge,0,1,0,1,0))return false;
        // Five luminous longitudinal rails, solid physical stereo geometry.
        for(int lane=0;lane<5;++lane){
            const float x=-.68f+.34f*lane;
            if(!quad(x-.008f,x+.008f,0,1,lane+1,.48f,.012f))return false;
            if(!quad(x-.13f,x+.13f,.015f,.045f,lane+1,.86f,.022f))return false;
        }
        // Visible beat markers, along the slope (not a 2D overlay).
        for(int tick=1;tick<=7;++tick)
            if(!quad(leftEdge,rightEdge,float(tick)/8.f,
                    float(tick)/8.f+.004f,6,.28f,.015f))return false;
        // Append far notes before near ones. The depth in *each eye*
        // comes from the actual HMD position, not UV offsets.
        for(int n=int(frame.count)-1;n>=0;--n){
            const auto& note=frame.notes[uint32_t(n)];
            if(note.lane>4||!std::isfinite(note.untilHit)||
               !std::isfinite(note.sustain)||note.untilHit<0||
               note.untilHit>2.25f||note.sustain<0)return false;
            const float x=-.68f+.34f*note.lane;
            const float along=std::clamp(note.untilHit/2.25f,0.f,1.f);
            const float tail=std::min(1.f,along+note.sustain/2.25f);
            if(tail>along+.012f)
                if(!quad(x-.028f,x+.028f,along,tail,note.lane+1,.7f,.026f))return false;
            if(!quad(x-.125f,x+.125f,std::max(0.f,along-.013f),
                     std::min(1.f,along+.013f),note.lane+1,1.15f,.045f))return false;
        }
        draw.count[eye]=static_cast<uint32_t>(vertices.size())-draw.first[eye];
    }
    draw.chartNotes=frame.count;
    return vertices.size()<=kStereoHighwayMaxVertices;
}
// Preserve PC geometry, sprite UVs and tints; change only its world placement
// and the left/right headset projection. The 2D cinema is NOT sampled here.
struct StereoSourceDraw {
    uint32_t first[2][5]{},count[2][5]{}; // [eye][board, highway, sustain, guitar, effects]
};
// The source chart extends tens of game units into the distance. Alpha 5's
// 0.59 m vertical rise compressed the track to roughly half its visual height.
// Preserve the original source depth ordering, but give the neck a comfortable
// full-height stereo incline with its far tip above the eye-level center.
inline Vec stereoSourceLocal(const StereoSourceVertex& src,float sourceFar) {
    const float along=std::clamp(-src.pos[2]/sourceFar,0.f,1.f);
    // Preserve the original sprite aspect. Alpha 6 scaled their local Y by
    // 0.085 vs 0.34 for X, squashing round note gems into flat rectangles.
    return {.34f*src.pos[0],-.80f+1.37f*along+.30f*src.pos[1],
            .87f-1.12f*along-.05f*src.pos[1]};
}
inline bool stereoSourceGeometry(const StereoHighwayFrame& frame,const CinemaScreen& screen,
    const Eye (&eyes)[2],std::vector<CinemaVertex>& vertices,StereoSourceDraw& draw)
{
    draw={};
    if(!frame.active)return true;
    if(!validTrackedPose(screen.pose)||!std::isfinite(frame.sourceFar)||frame.sourceFar<1.f||
       frame.board.size()>kStereoAssetMaxVertices||frame.highway.size()>kStereoAssetMaxVertices||frame.sustain.size()>kStereoAssetMaxVertices||frame.guitar.size()>kStereoAssetMaxVertices||frame.effects.size()>kStereoAssetMaxVertices)return false;
    for(int eye=0;eye<2;++eye){
        if(!validTrackedPose(eyes[eye].pose))return false;
        const auto proj=projection(eyes[eye].fov,.05f,50.f);
        if(!proj)return false;
        const auto camera=view(eyes[eye].pose);
        for(int kind=0;kind<5;++kind){
            const auto& source=kind==0?frame.board:kind==1?frame.highway:kind==2?frame.sustain:kind==3?frame.guitar:frame.effects;
            draw.first[eye][kind]=static_cast<uint32_t>(vertices.size());
            // A flying hit particle may cross behind the headset. Skip only
            // its triangle instead of aborting the whole VR renderer. Always
            // preserve original triangle boundaries and valid UV/tint data.
            if(source.size()%3)return false;
            for(size_t n=0;n<source.size();n+=3){
                std::array<CinemaVertex,3> triangle{};
                bool visible=true;
                for(unsigned j=0;j<3;++j){
                    const auto& src=source[n+j];
                    for(float f:src.pos)if(!std::isfinite(f))return false;
                    for(float f:src.uv)if(!std::isfinite(f))return false;
                    for(float f:src.uvNext)if(!std::isfinite(f))return false;
                    for(float f:src.color)if(!std::isfinite(f))return false;
                    if(!std::isfinite(src.frameBlend))return false;
                    const Vec local=stereoSourceLocal(src,frame.sourceFar);
                    const Vec world=screen.pose.pos+rotate(screen.pose.rot,local);
                    const auto clip=cinemaTransform(*proj,
                        cinemaTransform(camera,{world.x,world.y,world.z,1.f}));
                    for(float c:clip)if(!std::isfinite(c))return false;
                    if(clip[3]<.06f)visible=false;
                    triangle[j]={clip,src.uv[0],src.uv[1],
                        {src.color[0],src.color[1],src.color[2],src.color[3]},
                        {src.uvNext[0],src.uvNext[1]},src.frameBlend};
                }
                if(visible)vertices.insert(vertices.end(),triangle.begin(),triangle.end());
            }
            draw.count[eye][kind]=static_cast<uint32_t>(vertices.size())-draw.first[eye][kind];
        }
    }
    return true;
}
#if defined(ENCORE_VR_CINEMA)
void encoreCinemaPublishHighway(const StereoHighwayFrame& frame);
StereoHighwayFrame encoreCinemaReadHighway();
#endif
}
