// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ps5/vr/probe_raster.hpp"
#include <cassert>
#include <cstdio>
int main(){
    const float fov[4]={1.8418f,.9472f,1.329f,1.329f};
    constexpr unsigned size=1024;
    for(int s=0;s<2;++s){
        encore_probe_eye e{};
        assert(encore_probe_eye_init(&e,fov,.059f,s));
        auto runs=encore::vr::probeRuns(e,size);
        std::vector<unsigned char> mask(size*size);
        assert(!runs.empty());
        for(const auto& r:runs){
            assert(r.width&&r.x+r.width<=size&&r.y<size);
            for(unsigned x=r.x;x<r.x+r.width;++x){
                auto& value=mask[r.y*size+x];assert(!value);value=1;
            }
        }
        for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x)
            assert(bool(mask[y*size+x])==(encore_probe_scene_pixel(&e,(float(x)+.5f)/size,(float(y)+.5f)/size)==UINT32_C(0xff808080)));
        assert(encore::vr::probeRuns(e,0).empty());
    }
    puts("PASS stereo raster coverage and nonoverlapping GPU clear rectangles");
}
