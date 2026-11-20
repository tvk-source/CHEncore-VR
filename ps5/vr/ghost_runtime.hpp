// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "vr_core.hpp"
#include "ghost/psvr2.h"
namespace encore::vr {
// Single render-thread owner. The caller must stop before destroying the GPU buffers,
// and must not call TV present while the headset owns output. No automatic retry
// after failures: restart on console if teardown cannot be verified.
class GhostRuntime final : public Runtime {
    bool active_{},pending_{},fault_{};
    uint64_t sequence_{},display_{};
    uint32_t next_{},current_{};
    int mode_{PSVR2_MODE_120_HALF};
    psvr2_pose head_{}; float tan_[2][4]{};
    static Pose convert(const psvr2_pose& p,bool tracked) noexcept {
        return {{p.pos[0],p.pos[1],p.pos[2]}, {p.quat[0],p.quat[1],p.quat[2],p.quat[3]},tracked};
    }
    static Frustum fov(const float* t) noexcept {return {t[0],t[1],t[2],t[3]};}
public:
    GhostRuntime()=default;
    GhostRuntime(const GhostRuntime&)=delete;
    GhostRuntime& operator=(const GhostRuntime&)=delete;
    ~GhostRuntime() override {stop();}
    bool setMode(int mode) noexcept {if(active_||mode<0||mode>2)return false;mode_=mode;return true;}
    bool start(const Targets& t) override {
        if(active_||fault_||t.tiled||!validTargets(t))return false;
        // Physical guitar, drums and DualSense remain owned by CHEncore.
        psvr2_set_dualsense(0);
        if(psvr2_init_output()!=0){psvr2_shutdown();fault_=true;return false;}
        psvr2_info info{};psvr2_get_info(&info);
        if(info.status!=0){psvr2_shutdown();fault_=true;return false;}
        for(int e=0;e<2;++e){tan_[e][0]=info.fov[e];tan_[e][1]=info.fov[1-e];tan_[e][2]=info.fov[2];tan_[e][3]=info.fov[3];if(!projection(fov(tan_[e]),.1f,100.f)){psvr2_shutdown();fault_=true;return false;}}
        void* addresses[3]{};for(unsigned i=0;i<3;++i)addresses[i]=reinterpret_cast<void*>(static_cast<uintptr_t>(t.gpuAddress[i]));
        psvr2_set_mirror(1);
        if(psvr2_output_start(t.width/2,t.height,addresses,3,mode_)!=0){psvr2_shutdown();fault_=true;return false;}
        sequence_=0; next_=0; pending_=false; active_=true;return true;
    }
    bool waitFrame(Frame& f) override {
        if(!active_||pending_||fault_||psvr2_exit_requested())return false;
        ++sequence_;
        if(psvr2_begin_frame(sequence_,&display_)!=0)return false;
        pending_=true;current_=next_;
        psvr2_pose eyes[2]{};float lv[3]{},av[3]{};
        const bool tracked=psvr2_head(display_,&head_,eyes,lv,av)!=0;
        if(!tracked||!psvr2_worn()){discard();return false;}
        f={};f.buffer=current_;f.predictedUs=display_;
        for(int e=0;e<2;++e)f.eyes[e]={convert(eyes[e],true),fov(tan_[e])};
        psvr2_poll();
        // Input uses acquisition time, never the predicted display time.
        const uint64_t now=psvr2_now_us();f.inputUs=now;
        for(int side=0;side<2;++side){psvr2_controller c{};psvr2_get_controller(side,now,&c);f.hands[side]={convert(c.pose,c.tracked!=0),c.buttons,c.trigger,c.connected!=0};}
        return true;
    }
    // Caller MUST have completed a GPU fence for this frame before invocation.
    bool submit(uint32_t buffer) override {
        if(!active_||!pending_||buffer!=current_||fault_)return false;
        pending_=false;
        const int rc=psvr2_submit(static_cast<int>(buffer),sequence_,display_,&head_,tan_[0],tan_[1]);
        if(rc!=0){fault_=true;return false;}
        next_=(next_+1)%3;return true;
    }
    void discard() noexcept {if(pending_){psvr2_repeat_frame(sequence_);pending_=false;}}
    void stop() noexcept override {if(!active_)return;discard();psvr2_shutdown();active_=false;fault_=true;}
    void haptic(unsigned hand,float strength) override {if(active_&&hand<2&&std::isfinite(strength))psvr2_haptic(static_cast<int>(hand),std::clamp(strength,0.f,1.f),.06f);}
    void headsetHaptic(float strength){if(active_&&std::isfinite(strength))psvr2_headset_haptic(std::clamp(strength,0.f,1.f));}
    bool faulted()const noexcept{return fault_;}
};
}
