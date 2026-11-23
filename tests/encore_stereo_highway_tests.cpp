// SPDX-License-Identifier: MIT
#include "../ps5/vr/stereo_highway.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace encore::vr;
int main(){
    Pose head{{0,1.2f,0},{0,0,0,1},true};
    auto screen=cinemaScreen(head,3.6f,16.f/9.f,2.f);
    assert(screen);
    Eye eyes[2]={
      {{{-.0295f,1.2f,0},{0,0,0,1},true},{1.8418f,.9472f,1.329f,1.329f}},
      {{{ .0295f,1.2f,0},{0,0,0,1},true},{.9472f,1.8418f,1.329f,1.329f}}
    };
    StereoHighwayFrame frame;
    std::vector<CinemaVertex> vertices;
    StereoHighwayDraw draw;
    assert(stereoHighwayGeometry(frame,*screen,eyes,vertices,draw));
    assert(vertices.empty());
    frame.active=true;
    frame.count=5;
    for(int i=0;i<5;++i)frame.notes[i]={uint8_t(i),.2f+float(i)*.4f,.25f};
    assert(stereoHighwayGeometry(frame,*screen,eyes,vertices,draw));
    assert(draw.chartNotes==5&&draw.count[0]>0&&draw.count[0]==draw.count[1]);
    assert(vertices.size()<kStereoHighwayMaxVertices);
    // Same world-space deck vertex must be perceived at a different
    // horizontal angle from left vs right: actual binocular parallax.
    const auto& left=vertices[draw.first[0]];
    const auto& right=vertices[draw.first[1]];
    assert(std::abs(left.clip[0]/left.clip[3]-right.clip[0]/right.clip[3])>.005f);
    frame.notes[0].lane=6;vertices.clear();
    assert(!stereoHighwayGeometry(frame,*screen,eyes,vertices,draw));
    frame.notes[0].lane=0;frame.count=kStereoHighwayNotes+1;vertices.clear();
    assert(!stereoHighwayGeometry(frame,*screen,eyes,vertices,draw));
    // Alpha 5: original source sprite UV/tint must survive independently
    // projected left/right eye geometry, with no flat screen quad required.
    StereoHighwayFrame textured{};
    textured.active=true;
    textured.sourceFar=39.35f;
    for(int i=0;i<3;++i){
        StereoSourceVertex h{};
        h.pos[0]=-.6f+float(i)*.6f;
        h.pos[2]=-12.f;
        h.uv[0]=float(i)*.1f;h.uv[1]=.3f;
        textured.highway.push_back(h);
        StereoSourceVertex g=h;
        g.pos[1]=.2f;g.color[0]=.4f;g.color[3]=.75f;
        textured.guitar.push_back(g);
    }
    for(int i=0;i<3;++i){
        StereoSourceVertex v{};
        v.pos[0]=float(i)-1.f;
        v.pos[2]=-20.f;
        v.uv[1]=float(i)*4.f; // scrolling board uses repeated V
        textured.board.push_back(v);
    }
    std::vector<CinemaVertex> texturedVerts(12);
    StereoSourceDraw source{};
    assert(stereoSourceGeometry(textured,*screen,eyes,texturedVerts,source));
    assert(source.count[0][0]==3&&source.count[0][1]==3&&source.count[0][2]==3);
    assert(source.count[1][0]==3&&source.count[1][1]==3&&source.count[1][2]==3);
    assert(texturedVerts[source.first[0][2]].color[0]==.4f);
    assert(texturedVerts[source.first[0][2]].color[3]==.75f);
    assert(texturedVerts[source.first[0][2]].u==0.f);
    const auto& a=texturedVerts[source.first[0][2]];
    const auto& b=texturedVerts[source.first[1][2]];
    assert(std::abs(a.clip[0]/a.clip[3]-b.clip[0]/b.clip[3])>.005f);
    // Alpha 6: the far tip must fill the upper part of the world-space
    // board rather than ending half-way up the view.
    StereoSourceVertex near{},far{};
    near.pos[2]=0.f;
    far.pos[2]=-textured.sourceFar;
    const auto nearPoint=stereoSourceLocal(near,textured.sourceFar);
    const auto farPoint=stereoSourceLocal(far,textured.sourceFar);
    assert(farPoint.y-nearPoint.y>1.25f);
    assert(farPoint.z<nearPoint.z);
    // Alpha 6 HUD: original game pixels projected only into left/right
    // peripheral panels, not into a second flat gameplay screen.
    CinemaHudPanel leftHud{-1.48f,-.83f,-.71f,.47f,-.06f,0.f,.08f,.29f,.94f};
    CinemaHudPanel rightHud{.83f,1.48f,-.71f,.47f,-.06f,.71f,.08f,1.f,.94f};
    const auto leftPanel=cinemaHudPanelVertices(*screen,eyes[0],leftHud);
    const auto rightPanel=cinemaHudPanelVertices(*screen,eyes[0],rightHud);
    assert(leftPanel&&rightPanel);
    assert((*leftPanel)[0].u==0.f&&(*leftPanel)[0].v==.08f);
    assert((*rightPanel)[0].u==.71f&&(*rightPanel)[0].v==.08f);
    const auto leftEyeHud=cinemaHudPanelVertices(*screen,eyes[0],rightHud);
    const auto rightEyeHud=cinemaHudPanelVertices(*screen,eyes[1],rightHud);
    assert(leftEyeHud&&rightEyeHud);
    const auto& l=(*leftEyeHud)[0];
    const auto& r=(*rightEyeHud)[0];
    assert(std::abs(l.clip[0]/l.clip[3]-r.clip[0]/r.clip[3])>.005f);
    rightHud.u0=-.01f;
    assert(!cinemaHudPanelVertices(*screen,eyes[0],rightHud));
    textured.sourceFar=0.f;
    assert(!stereoSourceGeometry(textured,*screen,eyes,texturedVerts,source));
    std::puts("STEREO_HIGHWAY_TEST PASS full-height track, original UV/tint, stereo HUD crops, binocular depth, invalid input");
}
