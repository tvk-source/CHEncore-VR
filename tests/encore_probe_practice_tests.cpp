// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ps5/vr/probe_practice.h"
#include "../ps5/src/platform.h"
#include <cassert>
#include <cstdio>
#include <cmath>
static audio_fill_fn callback=nullptr;
extern "C" bool audio_start(audio_fill_fn f,void*){callback=f;return true;}
extern "C" void audio_stop(void){callback=nullptr;}
int main(){
    encore_practice_view v{};
    encore_practice_begin(0);
    assert(!encore_practice_tick(0,0,&v)&&(v.notes&1)&&std::abs(v.distance[0]-.4f)<.0001f);
    assert(!encore_practice_tick(1000000,1,&v)&&v.hits==1&&v.score==50&&(v.hit_flash&1));
    assert(!encore_practice_tick(1100000,1,&v)&&v.hits==1);
    assert(!encore_practice_tick(1200000,0,&v));
    assert(!encore_practice_tick(1500000,2,&v)&&v.hits==2&&v.combo==2&&v.best_combo==2);
    assert(!encore_practice_tick(2200000,0,&v)&&v.misses==1&&v.combo==0);
    assert(!encore_practice_tick(2500000,1,&v)&&v.wrong==1);
    assert(encore_practice_tick(2499999,0,&v)==-1);
    encore_practice_begin(1);
    assert(!encore_practice_tick(1000000,1,&v)&&v.hits==0);
    assert(!encore_practice_tick(30000000,0,&v)&&v.misses==56&&v.notes==0);
    assert(encore_practice_audio_start()==1&&callback);
    int16_t pcm[512];callback(pcm,256,nullptr);
    assert(encore_practice_audio_frames()==256&&pcm[0]!=0&&pcm[0]==pcm[1]);
    encore_practice_audio_stop();assert(!callback);
    std::puts("PASS original SongChart/InputEvaluator practice bridge, scoring, visibility and audio callback");
}
