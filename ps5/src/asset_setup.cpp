// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "vulkanexamplebase.h"
#include "platform.h"
#include "ps5_samples.h"
#include "../../examples/cloneheronative/gameplay/asset_pack.hpp"
#include "../../examples/cloneheronative/gameplay/application_update.hpp"
#include "../../examples/cloneheronative/gameplay/persistent_storage.hpp"
#include <atomic>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PS5_APP_ROOT
#define PS5_APP_ROOT "/app0"
#endif
namespace {
class AssetSetup final:public VulkanExampleBase {
public:
 std::string work,package,message,installedRoot;
 std::atomic<bool> done{false},cancel{false};std::atomic<double> progress{0};
 std::array<pad,PAD_PLAYERS> pads{};
 pthread_t worker{};bool running{},success{},ready{},bundledAttempted{};double nextScan{};
 explicit AssetSetup(std::string directory):work(std::move(directory)) {
  title="CH: Encore";name="asset-setup";apiVersion=VK_API_VERSION_1_3;
  useDynamicRendering=true;deviceCreatepNextChain=&baseDynamicRenderingFeatures;
  ps5.ownOverlay=ps5.customOverlay=true;ps5.optionsEnds=false;
  overlayFontPath=getAssetPath()+"Roboto-Medium.ttf";
  defaultClearColor={{0.025f,0.04f,0.065f,1.0f}};
 }
 ~AssetSetup(){cancel=true;if(running)pthread_join(worker,nullptr);}
 static void* installWorker(void* pointer){auto* self=static_cast<AssetSetup*>(pointer);
  self->success=rhythm::assets::install(self->package,self->work,self->installedRoot,self->message,&self->cancel,&self->progress);
  self->done.store(true,std::memory_order_release);return nullptr;
 }
 void beginImport(){if(running || package.empty())return;cancel=false;done=false;progress=0;message.clear();
  running=pthread_create(&worker,nullptr,installWorker,this)==0;
  if(!running)message="Could not start asset import. Try again.";
 }
 void prepare() override{VulkanExampleBase::prepare();prepared=true;}
 void render() override {
  if(!prepared)return;
  if(running && done.load(std::memory_order_acquire)){
   pthread_join(worker,nullptr);running=false;
   if(success){rhythm::assets::setCurrentRoot(installedRoot);ready=true;quit=true;return;}
  }
  if(!running && now_seconds()>=nextScan){nextScan=now_seconds()+1;package.clear();
   // Personal desktop builds carry their pack inside the title. Keep the
   // immutable base path as a fallback when an updated runtime supplies assets/.
   for(const auto& path:{getAssetPath()+"EncoreAssets-v1.encore-assets",std::string(PS5_APP_ROOT "/assets/EncoreAssets-v1.encore-assets")}){
    struct stat st{};if(!stat(path.c_str(),&st) && S_ISREG(st.st_mode)){
     package=path;if(!bundledAttempted){bundledAttempted=true;beginImport();}break;
    }
   }
   if(package.empty())
   for(const auto& path:{std::string(CH_DATA_ROOT "/EncoreAssets-v1.encore-assets"),std::string("/data/homebrew/EncoreAssets-v1.encore-assets")}){
    struct stat st{};if(!stat(path.c_str(),&st) && S_ISREG(st.st_mode)){package=path;break;}
   }
#if defined(PS5_HOST_REFERENCE)
   if(package.empty()){const std::string path=std::string(PS5_APP_ROOT)+"/EncoreAssets-v1.encore-assets";struct stat st{};if(!stat(path.c_str(),&st) && S_ISREG(st.st_mode))package=path;}
#endif
   for(int i=0;package.empty() && i<8;++i){const auto path="/mnt/usb"+std::to_string(i)+"/EncoreAssets-v1.encore-assets";struct stat st{};
    if(!stat(path.c_str(),&st) && S_ISREG(st.st_mode))package=path;}
  }
  uint32_t pressed=ps5_pad().pressed;
  for(int i=0;i<PAD_PLAYERS;++i)if(pad_player(i,&pads[i]))pressed|=pads[i].pressed;
  // Explicit bounded test runs can exercise first-run setup without hardware.
  if(ps5.frameBudget)for(const auto& press:ps5Presses)if(press.program=="asset-setup" && press.frame==ps5.framesDrawn){
   if(press.action=="cross")pressed|=PAD_CROSS;
   if(press.action=="circle")pressed|=PAD_CIRCLE;
  }
  if(pressed&PAD_CROSS)beginImport();
  if(pressed&PAD_CIRCLE){if(running)cancel=true;else quit=true;}
  prepareFrame();auto cmd=drawCmdBuffers[currentBuffer];auto begin=vks::initializers::commandBufferBeginInfo();
  VK_CHECK_RESULT(vkBeginCommandBuffer(cmd,&begin));beginDynamicRendering(cmd);
  VkViewport viewport=vks::initializers::viewport(float(width),float(height),0,1);vkCmdSetViewport(cmd,0,1,&viewport);
  VkRect2D scissor{{0,0},{width,height}};vkCmdSetScissor(cmd,0,1,&scissor);drawUI(cmd);endDynamicRendering(cmd);
  VK_CHECK_RESULT(vkEndCommandBuffer(cmd));submitFrame();
  if(ps5.framesDrawn>=120)encore_application_rendered();
 }
 void OnUpdateUIOverlay(vks::UIOverlay*) override {
  const auto scale=std::clamp(float(width)/1920.f,.75f,2.5f);
  ImGui::SetNextWindowPos(ImVec2(width*.5f,height*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));
  ImGui::SetNextWindowSize(ImVec2(std::min(float(width)-60,1000.f*scale),0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(32*scale,26*scale));
  ImGui::Begin("Asset setup",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::SetWindowFontScale(1.4f);
  ImGui::TextUnformatted("CH: ENCORE");ImGui::Separator();ImGui::Spacing();
  ImGui::TextUnformatted(bundledAttempted?"PREPARING YOUR INSTALLATION":"IMPORT YOUR PC ASSETS");ImGui::Spacing();ImGui::PushTextWrapPos(0);
  if(bundledAttempted){
   ImGui::TextUnformatted("Preparing the personal assets included by your desktop builder. This happens once; future code updates keep them.");
  }else{
  ImGui::TextUnformatted("Artwork, fonts and sound effects are supplied by your own PC installation. They are not included in this download.");
  ImGui::Spacing();ImGui::TextUnformatted("1. Run the desktop asset importer on your PC game folder.");
  ImGui::TextUnformatted("2. Copy EncoreAssets-v1.encore-assets to an exFAT USB drive.");
  ImGui::TextUnformatted("3. Connect the drive, then select Import. Songs are separate.");
  }
  ImGui::Spacing();
  if(running){ImGui::TextUnformatted("Importing assets...");ImGui::ProgressBar(float(progress.load()),ImVec2(-1,20*scale));ImGui::TextUnformatted("Circle: Cancel");}
  else{
   ImGui::TextColored(package.empty()?ImVec4(.7f,.7f,.7f,1):ImVec4(.45f,.9f,.6f,1),"%s",package.empty()?"Waiting for the asset pack...":"Asset pack detected. Ready to import.");
   if(!package.empty() && ImGui::Button("Cross: Import",ImVec2(260*scale,44*scale)))beginImport();
   ImGui::SameLine();if(ImGui::Button("Circle: Close",ImVec2(220*scale,44*scale)))quit=true;
   if(!message.empty())ImGui::TextWrapped("%s",message.c_str());
  }
  ImGui::PopTextWrapPos();ImGui::End();ImGui::PopStyleVar(2);
 }
};
}
bool ps5_prepare_pc_assets(uint32_t frameBudget,const std::string& screenshot) {
#if defined(PS5_HOST_REFERENCE)
 const std::string work=std::string(PS5_APP_ROOT)+"/userdata/pc-assets";
#else
 (void)rhythm::persistent::prepare();
 const std::string work=CH_USERDATA "/pc-assets";
#endif
 std::string root,error;
 if(rhythm::assets::installed(work,root,error)){
  rhythm::assets::setCurrentRoot(root);
#if defined(PS5_HOST_REFERENCE)
  if(std::getenv("ENCORE_CINEMA_SHADER_DIR"))std::fprintf(stderr,"CINEMA_ASSETS PASS installed host pack\n");
#endif
  return true;
 }
#if defined(PS5_HOST_REFERENCE)
 // For automated cinema validation, import the user's own pack directly
 // with the existing verified importer. Do not render the setup screen
 // or incorrectly report a successful original-game reference.
 if(std::getenv("ENCORE_CINEMA_SHADER_DIR")){
  std::string package;
  if(const char* explicitPack=std::getenv("ENCORE_ASSET_PACK");explicitPack && *explicitPack)
   package=explicitPack;
  else for(const auto& candidate:{std::string(PS5_APP_ROOT "/EncoreAssets-v1.encore-assets"),
          std::string(PS5_APP_ROOT "/assets/EncoreAssets-v1.encore-assets")}){
   struct stat st{};if(!stat(candidate.c_str(),&st)&&S_ISREG(st.st_mode)){package=candidate;break;}
  }
  if(package.empty()){
   std::fprintf(stderr,"CINEMA_ASSETS FAIL: set ENCORE_ASSET_PACK to your authorized EncoreAssets-v1.encore-assets file\n");
   return false;
  }
  std::fprintf(stderr,"CINEMA_ASSETS importing %s\n",package.c_str());
  if(!rhythm::assets::install(package,work,root,error)){
   std::fprintf(stderr,"CINEMA_ASSETS FAIL: %s\n",error.c_str());return false;
  }
  rhythm::assets::setCurrentRoot(root);
  std::fprintf(stderr,"CINEMA_ASSETS PASS imported host pack\n");
  return true;
 }
#endif
 AssetSetup setup(work);setup.message=error;setup.ps5.frameBudget=frameBudget;setup.ps5.screenshotPath=screenshot;
 if(!setup.initVulkan())return false;setup.prepare();setup.renderLoop();
 if(setup.vulkanDevice)vkDeviceWaitIdle(setup.vulkanDevice->logicalDevice);
 return setup.ready;
}
