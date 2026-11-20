// SPDX-License-Identifier: MIT
#include "../ps5/vr/vr_core.hpp"
#include <cassert>
#include <iostream>
using namespace encore::vr;
int main(){
 auto p=projection({1,2,1.5f,.5f},.1f,100);assert(p);assert(!projection({0,1,1,1},.1f,100));
 auto ndc=[&](Vec v,int i){return ((*p)[i]*v.x+(*p)[4+i]*v.y+(*p)[8+i]*v.z+(*p)[12+i])/(-v.z);};
 assert(std::abs(ndc({-.1f,0,-.1f},0)+1)<1e-5f);assert(std::abs(ndc({.2f,0,-.1f},0)-1)<1e-5f);assert(std::abs(ndc({0,.15f,-.1f},1)+1)<1e-5f);assert(std::abs(ndc({0,-.05f,-.1f},1)-1)<1e-5f);assert(std::abs(ndc({0,0,-.1f},2))<1e-5f);assert(std::abs(ndc({0,0,-100},2)-1)<1e-5f);
 Pose head{{1,2,3},{0,.70710678f,0,.70710678f},true};auto r=relative(head,recenter(head));assert(dot(r.pos,r.pos)<1e-6f&&std::abs(r.rot.w-1)<1e-5f);auto v=view(Pose{{1,2,3},{},true});assert(v[12]==-1&&v[13]==-2&&v[14]==-3);
 assert(validTargets({{65536,131072,196608},4096,2048,4096,true}));assert(!validTargets({{65536,65536,196608},4096,2048,4096,true}));
 std::array<Pad,7> pads{};for(unsigned i=0;i<7;i++)pads[i]={{float(i),0,0},.18f,uint8_t(i+1)};
 DrumDetector d;assert(!d.sample({0,.1f,0},1000,true,pads));auto hit=d.sample({0,-.1f,0},21000,true,pads);assert(hit&&hit->lane==1&&hit->velocity==127&&hit->timeUs==11000);assert(!d.sample({0,-.2f,0},41000,true,pads));assert(!d.sample({0,.1f,0},61000,false,pads));assert(!d.sample({0,-.1f,0},81000,true,pads));
 GuitarDetector g;assert(!g.sample({0,.1f,0},5,1000,true));auto s=g.sample({0,-.1f,0},5,21000,true);assert(s&&s->frets==5&&s->direction==-1&&s->timeUs==11000);g.reset();assert(!g.sample({0,-.1f,0},7,31000,true));s=g.sample({0,.1f,0},7,51000,true);assert(s&&s->direction==1);
 std::cout<<"PASS asymmetric projection, depth, recenter, view, target validation, drum and strum edges\n";
}
