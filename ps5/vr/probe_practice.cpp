// SPDX-License-Identifier: GPL-3.0-or-later
#include "probe_practice.h"
#include "../../examples/cloneheronative/gameplay/song_pipeline.hpp"
#include "../src/platform.h"
#include <atomic>
#include <cmath>
#include <limits>
namespace {
rhythm::SongChart makeChart() {
    rhythm::SongChart c;
    // Original, asset-free exercise at 120 BPM. First note at 1 second;
    // last at 28.5 s, followed by time to finish judging.
    constexpr unsigned pattern[]={0,1,2,3,4,3,2,1,0,2,4,1,3,2,1,4};
    for(unsigned n=0;n<56;++n){
        rhythm::Note note{};
        note.time=1.0+.5*n;note.frets=uint16_t(1u<<pattern[n%16]);
        c.notes.push_back(note);
    }
    return c;
}
struct Practice {
    rhythm::SongChart chart=makeChart();
    rhythm::InputEvaluator input{chart};
    unsigned previous=0,hits=0,misses=0,wrong=0,combo=0,best=0,score=0;
    uint64_t last=0,flash[5]{};
};
Practice& session(){static Practice p;return p;}
const rhythm::InputEvaluator::WindowProvider window=[](size_t,const rhythm::Note&){return rhythm::makeHitWindow(.12,false);};
std::atomic<uint64_t> audioFrames{0};
bool audioActive=false;
void tone(int16_t* dest,int frames,void*) {
    uint64_t cursor=audioFrames.load(std::memory_order_relaxed);
    for(int i=0;i<frames;++i,++cursor){
        // Quiet, linearly decaying 20ms square-wave click; two accents.
        // No external music or files; callback does not allocate or lock.
        uint64_t beat=cursor/24000u,phase=cursor%24000u;
        int amplitude=phase<960u && cursor<30u*48000u ? int((960u-phase)*(beat%4==2?960u:600u)/960u):0;
        int16_t v=int16_t((phase/(beat%4==2?24u:40u))%2 ? amplitude:-amplitude);
        dest[2*i]=dest[2*i+1]=v;
    }
    audioFrames.store(cursor,std::memory_order_release);
}
}
extern "C" void encore_practice_begin(unsigned held){
    auto& p=session();
    p.input.reset();p.previous=held&31u;
    p.hits=p.misses=p.wrong=p.combo=p.best=p.score=0;p.last=0;
    for(auto& t:p.flash)t=0;
}
extern "C" int encore_practice_tick(uint64_t us,unsigned lanes,encore_practice_view* view){
    if(!view||us>60000000u)return -1;
    auto& p=session();
    if(us<p.last)return -1;
    p.last=us;lanes&=31u;unsigned rising=lanes&~p.previous;p.previous=lanes;
    const double now=double(us)/1e6;
    const auto expired=p.input.expireNotes(now,window);
    if(!expired.empty()){p.misses+=unsigned(expired.size());p.combo=0;}
    if(rising){
        auto eligible=p.input.eligibleNotes(now,window,.12);
        for(unsigned l=0;l<5;++l)if(rising&(1u<<l)){
            size_t best=p.chart.notes.size();double bestError=1.0;
            for(auto i:eligible)if(p.chart.notes[i].frets==(1u<<l)){
                const double err=std::abs(p.chart.notes[i].time-now);
                if(err<bestError){best=i;bestError=err;}
            }
            auto hit=p.input.tryHit(best, now,window,[l](const rhythm::Note& n){return n.frets==(1u<<l);});
            if(hit){++p.hits;++p.combo;p.best=std::max(p.best,p.combo);p.score+=50;p.flash[l]=us+120000u;}
            else{++p.wrong;p.combo=0;}
        }
    }
    *view={};
    rhythm::submitVisibleNotes(p.chart,now,2.5,.12,[&](const rhythm::RenderNote& r){
        if(p.input.state(r.noteIndex)!=rhythm::NoteState::Pending)return;
        for(unsigned l=0;l<5;++l)if((r.frets&(1u<<l))&&!(view->notes&(1u<<l))){
            view->notes|=1u<<l;view->distance[l]=float(r.timeUntilHit/2.5);
        }
    });
    for(unsigned l=0;l<5;++l)if(us<p.flash[l])view->hit_flash|=1u<<l;
    view->hits=p.hits;view->misses=p.misses;view->wrong=p.wrong;view->combo=p.combo;
    view->best_combo=p.best;view->score=p.score;
    return 0;
}
extern "C" int encore_practice_audio_start(void){
    if(audioActive)return 0;
    audioFrames.store(0,std::memory_order_relaxed);
    audioActive=audio_start(tone,nullptr);
    return audioActive?1:0;
}
extern "C" void encore_practice_audio_stop(void){
    if(audioActive){audio_stop();audioActive=false;}
}
extern "C" uint64_t encore_practice_audio_frames(void){return audioFrames.load(std::memory_order_acquire);}
