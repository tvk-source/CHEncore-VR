// SPDX-License-Identifier: MIT
#include "../ps5/vr/cinema_geometry.hpp"
#include <cassert>
#include <cstdio>
using namespace encore::vr;
static bool close(float a,float b){return std::abs(a-b)<.0001f;}
int main(){
    Pose head{{0,1.2f,0},{0,0,0,1},true};
    const auto screen=cinemaScreen(head,2.4f,16.f/9.f,2.5f);
    assert(screen&&close(screen->height,1.35f)&&close(screen->pose.pos.z,-2.5f));
    Eye l{{{-.0295f,1.2f,0},{},true},{1.8418f,.9472f,1.329f,1.329f}};
    Eye r{{{.0295f,1.2f,0},{},true},{.9472f,1.8418f,1.329f,1.329f}};
    auto a=cinemaVertices(*screen,l),b=cinemaVertices(*screen,r);
    assert(a&&b);
    for(unsigned k=0;k<6;++k){
        assert(close((*a)[k].clip[3],2.5f)&&close((*b)[k].clip[3],2.5f));
        assert((*a)[k].u==(*b)[k].u&&(*a)[k].v==(*b)[k].v);
    }
    assert((*a)[0].clip[0]<(*a)[1].clip[0]&&(*a)[0].clip[1]<(*a)[2].clip[1]);
    assert((*a)[0].u==0&&(*a)[0].v==0&&(*a)[2].u==1&&(*a)[2].v==1);
    auto old=(*a)[0].clip[0];
    l.pose.pos.x+=.2f;a=cinemaVertices(*screen,l);
    assert(a&&(*a)[0].clip[0]<old);
    head.rot={.25f,0,0,std::sqrt(1-.25f*.25f)};
    const auto level=cinemaScreen(head,2.4f,16.f/9.f,2.5f);
    assert(level&&close(level->pose.pos.y,head.pos.y)&&close(level->pose.rot.x,0));
    head.tracked=false;assert(!cinemaScreen(head,2.4f,16.f/9.f,2.5f));
    l.pose.rot={0,0,0,0};assert(!cinemaVertices(*screen,l));
    std::puts("PASS cinema screen world anchoring, stereo clip vertices, UV orientation and invalid poses");
}
