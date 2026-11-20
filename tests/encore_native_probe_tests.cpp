// SPDX-License-Identifier: GPL-3.0-or-later
// Mock the public headset/platform calls, exercise the actual native entry point.
#include "../ps5/vr/native_probe.cpp"
#include <cassert>
#include <limits>
namespace {
int fwResult=0, initResult=0, submitted=ENCORE_PROBE_FRAMES, status=0, worn=1;
unsigned fw=0x0761, initCalls=0, selfCalls=0, shutdownCalls=0;
float tangent=1.2f;
int gpuResult=0, gpuStarts=0, gpuStops=0;
void reset() {
    fwResult=initResult=status=0; submitted=ENCORE_PROBE_FRAMES; worn=1; fw=0x0761; tangent=1.2f;
    initCalls=selfCalls=shutdownCalls=0;
    gpuResult=gpuStarts=gpuStops=0;
}
}
extern "C" void say(const char*, ...) {}
extern "C" int sceKernelGetProsperoSystemSwVersion(ProbeSoftwareVersion* v) {
    assert(v->size == sizeof(*v)); v->version=fw<<16; return fwResult;
}
extern "C" int sceKernelGetSystemSwVersion(ProbeSoftwareVersion* v) {
    assert(v->size == sizeof(*v)); v->version=0x10790001u; return 0;
}
extern "C" int encore_probe_gpu_init() { ++gpuStarts; return gpuResult; }
extern "C" void encore_probe_gpu_stop() { ++gpuStops; }
extern "C" void psvr2_set_dualsense(int v) { assert(v==0); }
extern "C" void psvr2_set_mirror(int v) { assert(v==0); }
extern "C" int psvr2_init_output() { assert(gpuStarts==1 && gpuStops==0); ++initCalls; return initResult; }
extern "C" void psvr2_shutdown() { ++shutdownCalls; }
extern "C" void psvr2_get_info(psvr2_info* i) {
    i->status=status; i->worn=worn; i->panel_w=2000; i->panel_h=2040; i->ipd=.064f;
    for(float& t:i->fov)t=tangent;
}
extern "C" int psvr2_worn() { return worn; }
extern "C" int psvr2_selftest(const char*,int n) { assert(n==ENCORE_PROBE_FRAMES); ++selfCalls; return submitted; }
int main() {
    reset(); fw=0x1360; assert(encore_vr_native_probe()==2 && initCalls==0);
    reset(); fw=0x0760; assert(encore_vr_native_probe()==2 && initCalls==0);
    reset(); fwResult=-1; assert(encore_vr_native_probe()==2 && initCalls==0);
    reset(); gpuResult=-1; assert(encore_vr_native_probe()==5 && initCalls==0 && gpuStarts==1 && gpuStops==1);
    reset(); initResult=-1; assert(encore_vr_native_probe()==3 && selfCalls==0 && shutdownCalls==1);
    reset(); worn=0; assert(encore_vr_native_probe()==4 && selfCalls==0 && shutdownCalls==1);
    reset(); status=1; assert(encore_vr_native_probe()==4 && selfCalls==0 && shutdownCalls==1);
    reset(); tangent=std::numeric_limits<float>::quiet_NaN(); assert(encore_vr_native_probe()==4 && selfCalls==0);
    reset(); submitted=12; assert(encore_vr_native_probe()==4 && selfCalls==1 && shutdownCalls==1);
    reset(); assert(encore_vr_native_probe()==0 && selfCalls==1 && shutdownCalls==1 && gpuStarts==1 && gpuStops==1);
    std::puts("PASS native probe firmware, init failure, optical/wear gates, partial submissions and cleanup");
}
