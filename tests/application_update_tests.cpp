// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "gameplay/application_update.hpp"
#include "gameplay/update_sha256.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#define JSON_NOEXCEPTION 1
#include "json.hpp"
using namespace rhythm::updates;using Json=nlohmann::json;namespace fs=std::filesystem;
namespace chorus {bool httpGet(const std::string&,HttpSink,void*,uint64_t,const std::atomic<bool>&,std::string&) noexcept{return false;}}
static std::string response,payload;static bool corrupt,slow;
static std::string hash(const std::string& s){Sha256 sha;sha.add(s.data(),s.size());return sha.finish();}
static bool fetch(const std::string& url,chorus::HttpSink sink,void* user,uint64_t limit,const std::atomic<bool>& cancel,std::string& error) noexcept{
 if(slow){for(unsigned i=0;i<100 && !cancel;++i)usleep(1000);}
 if(cancel){error="Update cancelled.";return false;}
 const bool api=url.find("api.github.com")!=std::string::npos;
 auto data=api?response:payload;if(!api && corrupt && !data.empty())data[0]^=1;
 if(data.size()>limit){error="Too large";return false;}
 for(size_t i=0;i<data.size();i+=7)if(!sink(data.data()+i,std::min<size_t>(7,data.size()-i),user))return false;
 return true;
}
static void wait(ApplicationUpdate& updater){for(unsigned i=0;i<1000;++i){updater.poll();const auto state=updater.state();if(state!=UpdateState::Checking && state!=UpdateState::Downloading && state!=UpdateState::Installing){updater.poll();return;}usleep(1000);}assert(false);}
int main(){
 const std::string base="https://github.com/MyNameIsAwoo/CHEncore-PS5/releases/download/alpha-v41/";
 const std::vector<std::string> names{"eboot.bin","sce_sys/param.json"},data{"new game",R"({"titleId":"PPSA98743","contentVersion":"01.000.041"})"};
 Json manifest={{"schema",1u},{"title","PPSA98743"},{"version",41u},{"files",Json::array()}};
 for(size_t i=0;i<names.size();++i)manifest["files"].push_back({{"path",names[i]},{"size",data[i].size()},{"sha256",hash(data[i])}});
 const auto text=manifest.dump();payload="CHENUP1\n";for(unsigned i=0;i<4;++i)payload+=char(text.size()>>(8*i));payload+=text;for(const auto& bytes:data)payload+=bytes;
 Json release={{"tag_name","alpha-v41"},{"draft",false},{"prerelease",false},{"body","Release notes"},{"assets",Json::array()}};
 for(const auto& suffix:{"_update.encore",".pkg",".exfat"}){std::string name="CH_Encore_Alpha_v41"+std::string(suffix);release["assets"].push_back({{"name",name},{"size",payload.size()},{"digest","sha256:"+hash(payload)},{"browser_download_url",base+name}});}
 response=release.dump();ReleaseInfo info;std::string error;assert(parseRelease(response,info,error) && info.version==41 && !info.assets[0].url.empty());
 // Builder-first releases expose the universal runtime update, without ready-
 // made PKG/exFAT files. Existing installations must still discover/install it.
 auto builderOnly=release;builderOnly["assets"]=Json::array({release["assets"][0]});
 assert(parseRelease(builderOnly.dump(),info,error) && !info.assets[0].url.empty() && info.assets[1].url.empty() && info.assets[2].url.empty());
 for(const auto& field:{"draft","prerelease"}){auto wrong=release;wrong[field]=true;assert(!parseRelease(wrong.dump(),info,error));}
 auto wrong=release;wrong["tag_name"]="v1.0-rc1";assert(!parseRelease(wrong.dump(),info,error));
 wrong=release;wrong["assets"][0]["browser_download_url"]="https://other.example/update";assert(parseRelease(wrong.dump(),info,error) && info.assets[0].url.empty());
 wrong=release;wrong["assets"][0]["digest"]="sha256:abc";assert(parseRelease(wrong.dump(),info,error) && info.assets[0].url.empty());
 wrong=release;wrong["assets"].push_back(wrong["assets"][0]);assert(!parseRelease(wrong.dump(),info,error));
 assert(!parseRelease("{",info,error));
 // The beta channel is published as a GitHub prerelease and must be visible
 // through the releases list, rather than the stable-only /latest endpoint.
 auto beta=release;beta["tag_name"]="beta-v1.0";beta["prerelease"]=true;
 for(auto& asset:beta["assets"]){auto name=asset["name"].get<std::string>();const auto at=name.find("Alpha_v41");assert(at!=std::string::npos);name.replace(at,9,"Beta_1.0");asset["name"]=name;asset["browser_download_url"]="https://github.com/MyNameIsAwoo/CHEncore-PS5/releases/download/beta-v1.0/"+name;}
 assert(parseRelease(beta.dump(),info,error) && info.version==10000 && info.title=="Beta 1.0" && !info.assets[0].url.empty());
 auto future=beta;future["tag_name"]="beta-v2.0";future["draft"]=true;
 assert(parseRelease(Json::array({release,future,beta}).dump(),info,error) && info.version==10000);
 assert(parseRelease(Json::array({beta,release}).dump(),info,error) && info.version==10000);
 char tmp[]="/tmp/encore-updater-XXXXXX";assert(mkdtemp(tmp));fs::path root=tmp;
 {ApplicationUpdate updater(40,(root/"folder").string(),fetch);assert(updater.check());wait(updater);assert(updater.state()==UpdateState::Available);usleep(1000);updater.poll();assert(updater.download(UpdateFormat::Folder));wait(updater);assert(updater.state()==UpdateState::Ready && fs::exists(root/"folder/stage/eboot.bin"));assert(updater.install(error));wait(updater);assert(updater.state()==UpdateState::RestartReady && fs::exists(root/"folder/runtime.json"));}
 {ApplicationUpdate updater(41,(root/"current").string(),fetch);assert(updater.check());wait(updater);assert(updater.state()==UpdateState::Current);}
 {ApplicationUpdate updater(40,(root/"pkg").string(),fetch);assert(updater.check());wait(updater);usleep(1000);updater.poll();assert(updater.download(UpdateFormat::Package));wait(updater);assert(updater.state()==UpdateState::Downloaded && !updater.snapshot().path.empty());}
 corrupt=true;{ApplicationUpdate updater(40,(root/"bad").string(),fetch);assert(updater.check());wait(updater);usleep(1000);updater.poll();assert(updater.download(UpdateFormat::Folder));wait(updater);assert(updater.state()==UpdateState::Error && !fs::exists(root/"bad/stage/eboot.bin"));}corrupt=false;
 slow=true;{ApplicationUpdate updater(40,(root/"cancel").string(),fetch);assert(updater.check());updater.cancel();wait(updater);assert(updater.state()==UpdateState::Cancelled);}slow=false;
 fs::remove_all(root);puts("PASS release parsing/channel/repository/hash validation, newer/current checks, streamed folder staging, package download, corrupted transfer and cancellation");
}
