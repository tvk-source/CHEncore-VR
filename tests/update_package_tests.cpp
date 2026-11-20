// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "gameplay/update_package.hpp"
#include "gameplay/update_sha256.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#define JSON_NOEXCEPTION 1
#include "json.hpp"
using Json=nlohmann::json;namespace fs=std::filesystem;using namespace rhythm::updates;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<s;}
static std::string get(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static std::string hash(const std::string& s){Sha256 h;h.add(s.data(),s.size());return h.finish();}
static void package(const fs::path& p,Json j,std::vector<std::string> data){
 const auto text=j.dump();uint8_t len[4];for(unsigned i=0;i<4;++i)len[i]=uint8_t(text.size()>>(i*8));
 std::ofstream out(p,std::ios::binary);out.write("CHENUP1\n",8);out.write(reinterpret_cast<char*>(len),4);out<<text;for(const auto& s:data)out<<s;
}
int main(){
 assert(hash("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 assert(hash("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 std::string million(1000000,'a');Sha256 streamed;for(size_t i=0;i<million.size();i+=7)streamed.add(million.data()+i,std::min<size_t>(7,million.size()-i));assert(streamed.finish()=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
 for(const char* path:{"../eboot.bin","assets/../../songs/foo","/eboot.bin","userdata/profiles.json","songs/notes.chart","assets//file","assets/./file","assets/file\\x","assets/file\n"})assert(!updatePath(path));
 assert(updatePath("assets/song/icons.png") && updatePath("eboot.bin"));
 char temp[]="/tmp/encore-update-XXXXXX";assert(mkdtemp(temp));fs::path root=temp,app=root/"app",work=root/"work",pkg=root/"update.encore";
 const std::string param=R"({"titleId":"PPSA98743","contentVersion":"01.000.041"})";
 const std::vector<std::string> names{"assets/new.txt","eboot.bin","sce_sys/param.json"},data{"art","new executable",param};
 Json j={{"schema",1u},{"title","PPSA98743"},{"version",41u},{"files",Json::array()}};
 for(size_t i=0;i<names.size();++i)j["files"].push_back({{"path",names[i]},{"size",data[i].size()},{"sha256",hash(data[i])}});
 package(pkg,j,data);std::string error;
 assert(stagePackage(pkg.string(),work.string(),41,error));assert(!stagePackage(pkg.string(),work.string(),42,error));
 put(app/"eboot.bin","old executable");put(app/"sce_sys/param.json","old params");put(app/"userdata/profiles.json","keep profile");put(app/"songs/notes.chart","keep song");
 assert(requestInstall(work.string(),error));assert(applyPending(app.string(),work.string(),error));
 assert(get(app/"eboot.bin")==data[1] && get(app/"assets/new.txt")==data[0]);
 assert(get(app/"userdata/profiles.json")=="keep profile" && get(app/"songs/notes.chart")=="keep song");
 // Simulate power loss after replacing the executable but before writing commit.
 fs::remove(work/"committed");assert(recoverPending(app.string(),work.string(),error)==Recovery::Restored);
 assert(get(app/"eboot.bin")=="old executable" && get(app/"sce_sys/param.json")=="old params" && !fs::exists(app/"assets/new.txt"));
 // Corrupt/truncated payload and wrong title/version cannot enter installation.
 auto bad=j;bad["files"][0]["sha256"]=std::string(64,'0');package(pkg,bad,data);assert(!stagePackage(pkg.string(),work.string(),41,error));
 bad=j;bad["title"]="OTHER";package(pkg,bad,data);assert(!stagePackage(pkg.string(),work.string(),41,error));
 bad=j;bad["files"][0]["path"]="../escape";package(pkg,bad,data);assert(!stagePackage(pkg.string(),work.string(),41,error));
 bad=j;bad["files"].push_back(bad["files"][0]);package(pkg,bad,data);assert(!stagePackage(pkg.string(),work.string(),41,error));
 package(pkg,j,{"short"});assert(!stagePackage(pkg.string(),work.string(),41,error));
 package(pkg,j,data);std::atomic<bool> cancel{true};assert(!stagePackage(pkg.string(),work.string(),41,error,&cancel));cancel=false;
 assert(stagePackage(pkg.string(),work.string(),41,error));
 // Symlink targets cannot redirect a write outside the application tree.
 fs::create_directories(root/"outside");fs::remove_all(app/"assets");fs::create_directory_symlink(root/"outside",app/"assets");
 assert(requestInstall(work.string(),error));assert(!applyPending(app.string(),work.string(),error));assert(!fs::exists(root/"outside/new.txt"));
 fs::remove(app/"assets");assert(requestInstall(work.string(),error));assert(applyPending(app.string(),work.string(),error));
 assert(recoverPending(app.string(),work.string(),error)==Recovery::None);
 assert(get(app/"eboot.bin")=="new executable");

 // Runtime activation never writes the installed title, including read-only images.
 const auto runtimeWork=root/"runtime-work";
 auto stageRuntime=[&](unsigned version,const std::string& executable){
  auto manifest=j;manifest["version"]=version;
  auto content=data;content[1]=executable;
  char versionText[24];std::snprintf(versionText,sizeof(versionText),"01.%03u.%03u",version/1000,version%1000);
  content[2]=Json{{"titleId","PPSA98743"},{"contentVersion",versionText}}.dump();
  for(size_t i=0;i<content.size();++i){manifest["files"][i]["size"]=content[i].size();manifest["files"][i]["sha256"]=hash(content[i]);}
  if(version>=10000){const auto module="native module "+executable;manifest["files"].push_back({{"path","sce_module/encore-engine.prx"},{"size",module.size()},{"sha256",hash(module)}});content.push_back(module);}
  package(pkg,manifest,content);assert(stagePackage(pkg.string(),runtimeWork.string(),version,error));
 };
 RuntimeBoot boot;assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && !boot.launch);
 stageRuntime(41,"runtime one");assert(!activateRuntime(runtimeWork.string(),error));
 assert(requestInstall(runtimeWork.string(),error));assert(activateRuntime(runtimeWork.string(),error));
 assert(get(app/"eboot.bin")=="new executable" && get(app/"userdata/profiles.json")=="keep profile");
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.launch && boot.version==41);
 const auto first=boot.generation;assert(get(fs::path(boot.root)/"eboot.bin")=="runtime one");
 assert((fs::status(fs::path(boot.root)/"eboot.bin").permissions()&fs::perms::owner_exec)!=fs::perms::none);
 assert(!acknowledgeRuntime(runtimeWork.string(),"g-999-abc",error));
 assert(prepareRuntime(runtimeWork.string(),40,first,boot,error) && boot.running && !boot.launch);
 assert(acknowledgeRuntime(runtimeWork.string(),first,error));
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.launch && boot.generation==first);
 stageRuntime(42,"runtime two");assert(requestInstall(runtimeWork.string(),error));assert(activateRuntime(runtimeWork.string(),error));
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.launch && boot.version==42);
 const auto second=boot.generation;assert(second!=first);
 // The replacement failed before its first successful render: next boot restores 41.
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.restored && boot.launch && boot.generation==first);
 assert(!prepareRuntime(runtimeWork.string(),40,second,boot,error));
 stageRuntime(43,"runtime three");assert(requestInstall(runtimeWork.string(),error));assert(activateRuntime(runtimeWork.string(),error));
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.launch);
 const auto third=boot.generation,thirdRoot=boot.root;
 assert(prepareRuntime(runtimeWork.string(),40,third,boot,error));assert(acknowledgeRuntime(runtimeWork.string(),third,error));
 put(fs::path(thirdRoot)/"eboot.bin","corrupt");
 assert(prepareRuntime(runtimeWork.string(),40,"",boot,error) && boot.restored && boot.generation==first);
 // Newer manual installs take precedence over cached older builds.
 assert(prepareRuntime(runtimeWork.string(),10000,"",boot,error) && !boot.launch);
 assert(!fs::exists(runtimeWork/"runtime.json"));
 stageRuntime(10002,"beta patch runtime");assert(requestInstall(runtimeWork.string(),error));assert(activateRuntime(runtimeWork.string(),error));
 assert(prepareRuntime(runtimeWork.string(),10000,"",boot,error) && boot.version==10002);
 const auto betaGeneration=boot.generation;
 assert(prepareRuntime(runtimeWork.string(),10000,betaGeneration,boot,error) && boot.running);
 assert(acknowledgeRuntime(runtimeWork.string(),betaGeneration,error));
 // The native module is the executed payload: corruption must restore embedded.
 put(fs::path(boot.root)/"sce_module/encore-engine.prx","corrupt module");
 assert(prepareRuntime(runtimeWork.string(),10000,"",boot,error) && boot.restored && !boot.launch);
 stageRuntime(10002,"beta patch runtime two");assert(requestInstall(runtimeWork.string(),error));assert(activateRuntime(runtimeWork.string(),error));
 assert(prepareRuntime(runtimeWork.string(),10000,"",boot,error) && boot.launch);
 // A first-generation launch failure falls back to the bundled executable.
 assert(prepareRuntime(runtimeWork.string(),10000,"",boot,error) && boot.restored && !boot.launch);
 stageRuntime(10002,"beta patch runtime");put(runtimeWork/"stage/assets/new.txt","changed");
 assert(requestInstall(runtimeWork.string(),error));assert(!activateRuntime(runtimeWork.string(),error));
 assert(!fs::exists(runtimeWork/"runtime.json"));
 // Missing native module in a Beta manifest must be rejected before installation.
 auto missingModule=j;missingModule["version"]=10002u;
 package(pkg,missingModule,data);
 assert(!stagePackage(pkg.string(),runtimeWork.string(),10002,error));
 put(runtimeWork/"runtime.json",R"({"schema":"invalid"})");assert(!prepareRuntime(runtimeWork.string(),40,"",boot,error));
 put(runtimeWork/"runtime.json",R"({"schema":1,"active":"../escape","previous":"","pending":false,"attempted":false})");assert(!prepareRuntime(runtimeWork.string(),40,"",boot,error));
 puts("PASS immutable runtime activation, launch acknowledgement, failed-start rollback, corruption fallback, manual upgrade precedence and beta version ordering");
 fs::remove_all(root);puts("PASS SHA-256 vectors/chunks, package staging, application-only writes, commit/recovery, preservation, corruption, traversal, duplicate paths, truncation, cancellation and symlink rejection");
}
