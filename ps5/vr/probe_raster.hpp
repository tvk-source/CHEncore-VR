// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "probe_stereo.h"
#include <vector>
namespace encore::vr {
struct ProbeRun { unsigned x,y,width; };
inline std::vector<ProbeRun> probeRuns(const encore_probe_eye& e,unsigned size) {
    std::vector<ProbeRun> runs;
    if(!size || size>2048) return runs;
    for(unsigned y=0;y<size;++y) {
        const float v=(float(y)+.5f)/float(size);
        unsigned start=0;
        bool inside=false;
        for(unsigned x=0;x<=size;++x) {
            const bool on=x<size && encore_probe_scene_pixel(&e,(float(x)+.5f)/float(size),v)==UINT32_C(0xff808080);
            if(on&&!inside){ start=x; inside=true; }
            if(!on&&inside){ runs.push_back({start,y,x-start}); inside=false; }
        }
    }
    return runs;
}
}
