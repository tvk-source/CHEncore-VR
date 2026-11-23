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
    std::puts("STEREO_HIGHWAY_TEST PASS live gems, five lanes, left/right depth, invalid-note rejection");
}
