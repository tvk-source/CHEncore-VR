// SPDX-License-Identifier: MIT
// CHEncore-VR experimental, platform-independent core. No console ABI assumptions.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
namespace encore::vr {
struct Vec { float x{},y{},z{}; };
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool finite(Vec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct Quat {float x{},y{},z{},w{1};};
inline Quat normalize(Quat q){float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return n>1e-6f&&std::isfinite(n)?Quat{q.x/n,q.y/n,q.z/n,q.w/n}:Quat{};}
inline Quat inverse(Quat q){q=normalize(q);return {-q.x,-q.y,-q.z,q.w};}
inline Quat mul(Quat a,Quat b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline Vec rotate(Quat q,Vec v){q=normalize(q);auto p=mul(mul(q,{v.x,v.y,v.z,0}),inverse(q));return {p.x,p.y,p.z};}
struct Pose {Vec pos;Quat rot;bool tracked{};};
struct Origin {Vec pos;Quat yaw;};
inline Origin recenter(Pose head){auto f=rotate(head.rot,{0,0,-1});float yaw=std::atan2(-f.x,-f.z);return {head.pos,{0,std::sin(yaw/2),0,std::cos(yaw/2)}};}
inline Pose relative(Pose p, Origin o){auto q=inverse(o.yaw);return {rotate(q,p.pos-o.pos),normalize(mul(q,p.rot)),p.tracked};}
// Column-major Vulkan clip space: right-handed, -Z forward, depth 0..1, y flipped.
using Mat=std::array<float,16>;
struct Frustum {float left{},right{},up{},down{};}; // positive tangent magnitudes
inline std::optional<Mat> projection(Frustum f,float n,float z){
 if(!(std::isfinite(f.left)&&std::isfinite(f.right)&&std::isfinite(f.up)&&std::isfinite(f.down)&&std::isfinite(n)&&std::isfinite(z)&&f.left>0&&f.right>0&&f.up>0&&f.down>0&&n>0&&z>n))return {};
 Mat m{};m[0]=2/(f.left+f.right);m[5]=-2/(f.up+f.down);m[8]=(f.right-f.left)/(f.left+f.right);m[9]=-(f.up-f.down)/(f.up+f.down);m[10]=z/(n-z);m[11]=-1;m[14]=z*n/(n-z);return m;
}
inline Mat view(Pose p){auto q=inverse(p.rot);auto x=rotate(q,{1,0,0}),y=rotate(q,{0,1,0}),z=rotate(q,{0,0,1}),t=rotate(q,p.pos*-1);return {x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,t.x,t.y,t.z,1};}
struct Eye {Pose pose;Frustum fov;};
struct Hand {Pose pose;uint32_t buttons{};float trigger{};bool connected{};};
struct Frame {std::array<Eye,2> eyes;std::array<Hand,2> hands;uint32_t buffer{};uint64_t predictedUs{},inputUs{};};
struct Targets {std::array<uint64_t,3> gpuAddress{};uint32_t width{},height{},pitch{};bool tiled{};};
// Ghost PS5XR uses linear RGBA8, shared CPU/GPU address, 256-byte alignment.
// The stricter tiled mode remains available for the earlier Pulse experiment.
inline bool validTargets(Targets t){
 if(!t.width||!t.height||t.width%2||t.pitch!=t.width||t.width>16384||t.height>8192)return false;
 if(t.tiled&&(t.width%256||t.height%128))return false;
 const uint64_t alignment=t.tiled?65536:256;
 for(size_t i=0;i<3;++i){if(!t.gpuAddress[i]||t.gpuAddress[i]%alignment)return false;for(size_t j=0;j<i;++j)if(t.gpuAddress[i]==t.gpuAddress[j])return false;}return true;
}
// Actual backend must own both runtime and GPU lifetime. Implement only after ABI verification.
struct Runtime {virtual ~Runtime()=default;virtual bool start(const Targets&)=0;virtual bool waitFrame(Frame&)=0;virtual bool submit(uint32_t buffer)=0;virtual void stop() noexcept=0;virtual void haptic(unsigned hand,float strength)=0;};
enum class Mode{TV,Cinema,Stereo,Drums,AirGuitar};
struct Pad {Vec center;float radius{.18f};uint8_t lane{};}; // lane 0..7 is CHEncore raw bit position
struct Strike {uint8_t lane{},velocity{};uint64_t timeUs{};};
// Detect a downward crossing, interpolate acquisition timestamp, suppress held contacts.
// Tip positions must come from measured controller samples (not predicted rendering poses).
class DrumDetector {
 std::optional<Vec> prev_;uint64_t time_{};
 public:
 void reset(){prev_.reset();time_=0;}
 std::optional<Strike> sample(Vec tip,uint64_t us,bool tracked,const std::array<Pad,7>& pads){
  if(!tracked||!finite(tip)){reset();return {};}
  auto old=prev_;auto before=time_;prev_=tip;time_=us;
  if(!old||us<=before||us-before>100000)return {};
  const float dt=float(us-before)/1e6f;const float speed=(old->y-tip.y)/dt;
  if(speed<.25f)return {};
  float best=2;uint8_t lane=0;
  for(auto p:pads){if(p.radius<=0||p.lane>=8||old->y<=p.center.y||tip.y>p.center.y)continue;float a=(old->y-p.center.y)/(old->y-tip.y);auto v=*old+(tip-*old)*a-p.center;if(v.x*v.x+v.z*v.z<=p.radius*p.radius&&a<best){best=a;lane=p.lane;}}
  if(best>1)return {};
  return Strike{lane,uint8_t(std::clamp(int(speed*25),1,127)),before+uint64_t(best*(us-before))};
 }
};
struct Strum{uint8_t frets{};int direction{};uint64_t timeUs{};};
class GuitarDetector {
 std::optional<Vec> prev_;uint64_t time_{};bool armed_{true};
 public:
 void reset(){prev_.reset();time_=0;armed_=true;}
 // Configurable five fret mask from button adapter; y is relative to guitar plane.
 std::optional<Strum> sample(Vec tip,uint8_t frets,uint64_t us,bool tracked){
  if(!tracked||!finite(tip)){reset();return {};}
  auto old=prev_;auto before=time_;prev_=tip;time_=us;
  if(!old||us<=before||us-before>100000){armed_=true;return {};}
  if(std::abs(tip.y)>.08f)armed_=true;
  const float dy=tip.y-old->y;
  if(!armed_||std::abs(tip.x)>.18f||std::abs(tip.z)>.18f||std::abs(dy)<.001f||std::abs(dy)*1e6f/float(us-before)<.25f)return {};
  if(!((old->y>0&&tip.y<=0)||(old->y<0&&tip.y>=0)))return {};
  const float a=-old->y/dy;armed_=false;return Strum{uint8_t(frets&31),dy>0?1:-1,before+uint64_t(a*(us-before))};
 }
};
}
