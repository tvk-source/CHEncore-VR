// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ps5/vr/ghost_runtime.hpp"
#include <cassert>
#include <iostream>
using namespace encore::vr;
static int repeats=0,submitted=0,shutdowns=0,started=0;static bool tracked=true,worn=true;static uint64_t last=0;
extern "C" {
int psvr2_init_output(){return 0;}
void psvr2_set_dualsense(int use){assert(use==0);}
void psvr2_get_info(psvr2_info* i){*i={};i->fov[0]=1.8f;i->fov[1]=.9f;i->fov[2]=i->fov[3]=1.3f;}
void psvr2_set_mirror(int on){assert(on==1);}
int psvr2_output_start(uint32_t w,uint32_t h,void*const* p,int count,int mode){assert(w==1024&&h==1024&&count==3&&mode==2&&p[0]);++started;return 0;}
int psvr2_begin_frame(uint64_t n,uint64_t* t){assert(n>last);last=n;*t=100000+n;return 0;}
int psvr2_exit_requested(){return 0;}
int psvr2_head(uint64_t,psvr2_pose* h,psvr2_pose eyes[2],float*,float*){*h={{0,0,0},{0,0,0,1}};eyes[0]=*h;eyes[1]=*h;return tracked;}
int psvr2_worn(){return worn;}
void psvr2_poll(){}
uint64_t psvr2_now_us(){return 99999;}
void psvr2_get_controller(int side,uint64_t t,psvr2_controller* c){assert(t==99999);*c={};c->connected=c->tracked=1;c->buttons=side+1;c->pose.quat[3]=1;}
int psvr2_submit(int b,uint64_t n,uint64_t,const psvr2_pose*,const float* l,const float* r){assert(n==last&&l[0]==r[1]&&l[1]==r[0]);assert(b==submitted%3);++submitted;return 0;}
void psvr2_repeat_frame(uint64_t n){assert(n==last);++repeats;}
void psvr2_shutdown(){++shutdowns;}
void psvr2_haptic(int,float,float){}
void psvr2_headset_haptic(float){}
}
int main(){
 GhostRuntime g;Targets t{{256,512,768},2048,1024,2048,false};assert(validTargets(t));assert(g.start(t));assert(started==1);Frame f;
 for(int n=0;n<5;++n){assert(g.waitFrame(f));assert(f.buffer==unsigned(n%3));assert(f.inputUs<f.predictedUs);assert(f.hands[1].buttons==2);assert(!g.waitFrame(f));assert(!g.submit((f.buffer+1)%3));assert(g.submit(f.buffer));}
 tracked=false;assert(!g.waitFrame(f));assert(repeats==1);tracked=true;worn=false;assert(!g.waitFrame(f));assert(repeats==2);worn=true;assert(g.waitFrame(f));g.stop();assert(repeats==3&&shutdowns==1);assert(!g.start(t));
 std::cout<<"PASS GhostVR adapter: linear registration, FOV, timing, buffer rotation, tracking loss and shutdown (mock hardware)\n";
}
