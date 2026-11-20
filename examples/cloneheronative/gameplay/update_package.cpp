// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "update_package.hpp"
#include "update_sha256.hpp"
#include "file_status.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#define JSON_NOEXCEPTION 1
#include "json.hpp"
namespace rhythm::updates {
namespace {
using Json=nlohmann::json;
constexpr uint64_t maxPackage=2ull*1024*1024*1024,maxFile=512ull*1024*1024;
struct Entry{std::string path,hash,backupHash;uint64_t size{};bool existed{};};
struct Manifest{unsigned version{};std::vector<Entry> files;};
bool exists(const std::string& p){struct stat st{};return rhythm::fs::statusNoFollow(p.c_str(),&st)==0;}
bool stopped(const std::atomic<bool>* c){return c && c->load(std::memory_order_relaxed);}
bool directory(const std::string& p){
    if(p.empty())return false;
    const auto slash=p.find_last_of('/');if(slash!=std::string::npos && slash && !exists(p.substr(0,slash)) && !directory(p.substr(0,slash)))return false;
    struct stat st{};if(rhythm::fs::statusNoFollow(p.c_str(),&st)==0)return S_ISDIR(st.st_mode);
    return errno==ENOENT && mkdir(p.c_str(),0777)==0;
}
bool parents(const std::string& p){const auto end=p.find_last_of('/');return end!=std::string::npos && directory(p.substr(0,end));}
bool safeParents(const std::string& root,const std::string& path){
    if(!directory(root))return false;
    auto full=root;size_t start=0;
    while(start<path.size()){const auto end=path.find('/',start);if(end==std::string::npos)break;full+="/"+path.substr(start,end-start);if(!directory(full))return false;start=end+1;}
    return true;
}
void discard(const std::string& p){
    struct stat st{};if(rhythm::fs::statusNoFollow(p.c_str(),&st))return;
    if(S_ISDIR(st.st_mode)){DIR* d=opendir(p.c_str());if(!d)return;while(auto* e=readdir(d))if(std::strcmp(e->d_name,".") && std::strcmp(e->d_name,".."))discard(p+"/"+e->d_name);closedir(d);(void)rmdir(p.c_str());}
    else (void)unlink(p.c_str());
}
bool syncParent(const std::string& path){
    const auto slash=path.find_last_of('/');if(slash==std::string::npos)return false;
    const int fd=open(path.substr(0,slash).c_str(),O_RDONLY|O_DIRECTORY);if(fd<0)return false;
    const bool ok=fsync(fd)==0;close(fd);return ok;
}
bool writeText(const std::string& p,const std::string& text){
    if(!parents(p))return false;const auto tmp=p+".tmp";
    const int fd=open(tmp.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0666);if(fd<0)return false;
    FILE* f=fdopen(fd,"wb");if(!f){close(fd);return false;}
    bool ok=std::fwrite(text.data(),1,text.size(),f)==text.size() && std::fflush(f)==0 && fsync(fd)==0;if(std::fclose(f))ok=false;
    if(ok)ok=std::rename(tmp.c_str(),p.c_str())==0 && syncParent(p);if(!ok)(void)unlink(tmp.c_str());return ok;
}
bool readJson(const std::string& p,Json& out){
    FILE* f=std::fopen(p.c_str(),"rb");if(!f)return false;struct stat st{};
    if(fstat(fileno(f),&st) || st.st_size<=0 || st.st_size>2*1024*1024){std::fclose(f);return false;}
    std::string text(size_t(st.st_size),'\0');bool ok=std::fread(text.data(),1,text.size(),f)==text.size();std::fclose(f);
    if(!ok)return false;out=Json::parse(text,nullptr,false);return !out.is_discarded();
}
bool hashString(const std::string& h){return h.size()==64 && std::all_of(h.begin(),h.end(),[](char c){return (c>='0' && c<='9') || (c>='a' && c<='f');});}
bool parse(const Json& j,Manifest& manifest,bool plan=false){
    if(!j.is_object() || !j.count("schema") || !j["schema"].is_number_unsigned() || j["schema"]!=1 ||
       !j.count("title") || j["title"]!="PPSA98743" || !j.count("version") || !j["version"].is_number_unsigned())return false;
    auto version=j["version"].get<uint64_t>();if(!version || version>999999)return false;manifest.version=unsigned(version);
    if(!j.count("files") || !j["files"].is_array() || j["files"].empty() || j["files"].size()>4096)return false;
    uint64_t total=0;bool executable=false,parameters=false,engine=false;
    for(const auto& file:j["files"]){
        if(!file.is_object() || !file.count("path") || !file["path"].is_string() || !file.count("sha256") || !file["sha256"].is_string() || !file.count("size") || !file["size"].is_number_unsigned())return false;
        Entry e;e.path=file["path"].get<std::string>();e.hash=file["sha256"].get<std::string>();e.size=file["size"].get<uint64_t>();
        if(!updatePath(e.path) || !hashString(e.hash) || e.size>maxFile || e.size>maxPackage-total)return false;total+=e.size;
        for(const auto& old:manifest.files)if(old.path==e.path)return false;
        if(plan){if(!file.count("existed") || !file["existed"].is_boolean())return false;e.existed=file["existed"].get<bool>();if(e.existed){if(!file.count("backup_sha256") || !file["backup_sha256"].is_string())return false;e.backupHash=file["backup_sha256"].get<std::string>();if(!hashString(e.backupHash))return false;}}
        executable|=e.path=="eboot.bin";parameters|=e.path=="sce_sys/param.json";engine|=e.path=="sce_module/encore-engine.prx";manifest.files.push_back(std::move(e));
    }return executable && parameters && (manifest.version<10000 || engine);
}
Json serialize(const Manifest& m){Json j={{"schema",1u},{"title","PPSA98743"},{"version",m.version},{"files",Json::array()}};for(const auto& f:m.files)j["files"].push_back({{"path",f.path},{"size",f.size},{"sha256",f.hash},{"existed",f.existed},{"backup_sha256",f.backupHash}});return j;}
bool copy(const std::string& from,const std::string& to,const std::string& expected={}){
    struct stat st{};if(rhythm::fs::statusNoFollow(from.c_str(),&st) || !S_ISREG(st.st_mode) || !parents(to))return false;
    FILE* in=std::fopen(from.c_str(),"rb");if(!in)return false;
    const int fd=open(to.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0666);FILE* out=fd<0?nullptr:fdopen(fd,"wb");
    if(!out){if(fd>=0)close(fd);std::fclose(in);return false;}
    std::array<uint8_t,32768> bytes;Sha256 hash;bool ok=true;
    for(;;){const auto n=std::fread(bytes.data(),1,bytes.size(),in);if(n){hash.add(bytes.data(),n);if(std::fwrite(bytes.data(),1,n,out)!=n){ok=false;break;}}if(n<bytes.size()){ok=!std::ferror(in);break;}}
    std::fclose(in);ok=ok && (expected.empty() || hash.finish()==expected) && std::fflush(out)==0 && fsync(fd)==0;
    if(std::fclose(out))ok=false;if(!ok)(void)unlink(to.c_str());return ok;
}
bool replace(const std::string& from,const std::string& root,const Entry& e,const std::string& hash={}){
    if(!safeParents(root,e.path))return false;const auto target=root+"/"+e.path;struct stat st{};
    if(rhythm::fs::statusNoFollow(target.c_str(),&st)==0 && !S_ISREG(st.st_mode))return false;
    const auto tmp=target+".encore-new";if(!copy(from,tmp,hash))return false;
    if(std::rename(tmp.c_str(),target.c_str())){(void)unlink(tmp.c_str());return false;}return syncParent(target);
}
bool rollback(const std::string& root,const std::string& work,std::string& error){
    Json j;Manifest plan;if(!readJson(work+"/plan.json",j) || !parse(j,plan,true)){error="Update recovery plan is unreadable; application was left untouched.";return false;}
    std::stable_sort(plan.files.begin(),plan.files.end(),[](const Entry& a,const Entry& b){return (a.path=="eboot.bin")<(b.path=="eboot.bin");});
    bool ok=true;for(const auto& e:plan.files){
        if(e.existed)ok=replace(work+"/backup/"+e.path,root,e,e.backupHash) && ok;
        else {const auto target=root+"/"+e.path;struct stat st{};if(!rhythm::fs::statusNoFollow(target.c_str(),&st))ok=S_ISREG(st.st_mode) && unlink(target.c_str())==0 && ok;}
    }
    if(!ok){error="Update recovery needs another attempt. Previous files remain in the update backup.";return false;}
    (void)unlink((work+"/prepared").c_str());(void)unlink((work+"/request").c_str());return true;
}
}
bool updatePath(const std::string& path) noexcept {
    if(path.empty() || path.size()>220 || path.front()=='/' || path.find_first_of("\\:\r\n")!=std::string::npos)return false;
    for(unsigned char c:path)if(c<32 || c==127)return false;
    size_t begin=0;for(;;){const auto end=path.find('/',begin);const auto piece=path.substr(begin,end==std::string::npos?end:end-begin);if(piece.empty() || piece=="." || piece=="..")return false;if(end==std::string::npos)break;begin=end+1;}
    if(path=="eboot.bin" || path=="LEGAL.txt")return true;
    for(const char* prefix:{"assets/","shaders/","licenses/","sce_sys/","sce_module/"})if(path.rfind(prefix,0)==0)return true;
    return false;
}
bool fileDigest(const std::string& path,std::string& digest,uint64_t* count) noexcept {
    struct stat st{};if(rhythm::fs::statusNoFollow(path.c_str(),&st) || !S_ISREG(st.st_mode))return false;
    FILE* in=std::fopen(path.c_str(),"rb");if(!in)return false;Sha256 hash;uint64_t total=0;std::array<uint8_t,32768> bytes;bool ok=true;
    for(;;){const auto n=std::fread(bytes.data(),1,bytes.size(),in);hash.add(bytes.data(),n);total+=n;if(n<bytes.size()){ok=!std::ferror(in);break;}}
    std::fclose(in);if(!ok)return false;digest=hash.finish();if(count)*count=total;return true;
}
bool stagePackage(const std::string& package,const std::string& work,unsigned version,std::string& error,const std::atomic<bool>* cancel,std::atomic<double>* progress){
    error.clear();FILE* in=std::fopen(package.c_str(),"rb");if(!in){error="Cannot open downloaded update.";return false;}
    struct Close{FILE* f;~Close(){std::fclose(f);}} inputGuard{in};
    uint8_t header[12];if(std::fread(header,1,12,in)!=12 || std::memcmp(header,"CHENUP1\n",8)){error="Invalid update package header.";return false;}
    const uint32_t length=uint32_t(header[8])|(uint32_t(header[9])<<8)|(uint32_t(header[10])<<16)|(uint32_t(header[11])<<24);
    if(!length || length>2*1024*1024){error="Invalid update manifest size.";return false;}
    std::string text(length,'\0');if(std::fread(text.data(),1,length,in)!=length){error="Incomplete update manifest.";return false;}
    const Json j=Json::parse(text,nullptr,false);Manifest m;
    if(j.is_discarded() || !parse(j,m) || m.version!=version){error="Update manifest does not match the selected PS5 release.";return false;}
    if(exists(work+"/prepared") || exists(work+"/request")){error="An earlier installation must be recovered first.";return false;}
    if(!directory(work)){
        const int saved=errno;struct stat a{},b{};const auto slash=work.find_last_of('/');const auto parent=work.substr(0,slash);
        const int l=rhythm::fs::statusNoFollow(parent.c_str(),&a),le=errno,t=stat(parent.c_str(),&b),te=errno;
        char detail[240];std::snprintf(detail,sizeof(detail),"Update storage unavailable: errno=%d parent nofollow=%d/%d mode=%o stat=%d/%d mode=%o",saved,l,le,unsigned(a.st_mode),t,te,unsigned(b.st_mode));error=detail;return false;
    }
    discard(work+"/stage");if(!directory(work+"/stage")){error="Cannot create update staging folder.";return false;}
    uint64_t total=0,done=0;for(const auto& e:m.files)total+=e.size;
    std::array<uint8_t,32768> bytes;
    for(const auto& e:m.files){
        if(stopped(cancel)){error="Update cancelled.";return false;}
        const auto destination=work+"/stage/"+e.path;
        if(!safeParents(work+"/stage",e.path)){error="Invalid staging folder.";return false;}
        const int fd=open(destination.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0666);FILE* out=fd<0?nullptr:fdopen(fd,"wb");
        if(!out){if(fd>=0)close(fd);error="Cannot write staged update.";return false;}
        Sha256 hash;uint64_t left=e.size;bool ok=true;
        while(left){if(stopped(cancel)){ok=false;break;}const auto n=size_t(std::min<uint64_t>(left,bytes.size()));if(std::fread(bytes.data(),1,n,in)!=n || std::fwrite(bytes.data(),1,n,out)!=n){ok=false;break;}hash.add(bytes.data(),n);left-=n;done+=n;if(progress)progress->store(total?double(done)/total:1,std::memory_order_relaxed);}
        ok=ok && hash.finish()==e.hash && std::fflush(out)==0 && fsync(fd)==0;if(std::fclose(out))ok=false;
        if(!ok){error=stopped(cancel)?"Update cancelled.":"Update file verification failed or storage is full.";return false;}
    }
    if(std::fgetc(in)!=EOF || std::ferror(in)){error="Unexpected data after update payload.";return false;}
    Json param;if(!readJson(work+"/stage/sce_sys/param.json",param) || !param.is_object() || !param.count("titleId") || param["titleId"]!="PPSA98743" || !param.count("contentVersion") || !param["contentVersion"].is_string()){error="Update application identity is invalid.";return false;}
    char expected[24];std::snprintf(expected,sizeof(expected),"01.%03u.%03u",version/1000,version%1000);
    if(param["contentVersion"]!=expected){error="Update application version is inconsistent.";return false;}
    if(!writeText(work+"/manifest.json",serialize(m).dump())){error="Cannot save verified update manifest.";return false;}return true;
}
bool writableApplication(const std::string& root) noexcept {
    static std::atomic<unsigned> sequence{0};
    for(unsigned attempt=0;attempt<32;++attempt){
        const auto path=root+"/.encore-update-write-test-"+std::to_string(getpid())+"-"+std::to_string(sequence.fetch_add(1));
        const int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
        if(fd<0){if(errno==EEXIST)continue;return false;}
        bool ok=close(fd)==0;ok=unlink(path.c_str())==0 && ok;return ok;
    }return false;
}
bool requestInstall(const std::string& work,std::string& error){
    Json j;Manifest m;if(!readJson(work+"/manifest.json",j) || !parse(j,m)){error="No verified update is ready.";return false;}
    if(!writeText(work+"/request","install after clean shutdown\n")){error="Cannot save update confirmation.";return false;}return true;
}
bool applyPending(const std::string& root,const std::string& work,std::string& error){
    if(!exists(work+"/request"))return true;
    struct RequestCleanup {std::string work;~RequestCleanup(){if(!exists(work+"/prepared"))(void)unlink((work+"/request").c_str());}} cleanup{work};
    if(exists(work+"/prepared")){error="An interrupted installation requires recovery.";return false;}
    Json j;Manifest m;if(!readJson(work+"/manifest.json",j) || !parse(j,m) || !writableApplication(root)){error="Application folder is unavailable or read-only.";return false;}
    // Verify every staged file again before backing up or replacing anything.
    for(const auto& e:m.files){std::string hash;uint64_t size;if(!fileDigest(work+"/stage/"+e.path,hash,&size) || hash!=e.hash || size!=e.size){error="Staged update changed; installation was not started.";return false;}}
    discard(work+"/backup");if(!directory(work+"/backup")){error="Cannot create update backup.";return false;}
    for(auto& e:m.files){
        if(!safeParents(root,e.path)){error="Application contains an invalid directory.";return false;}
        const auto target=root+"/"+e.path;struct stat st{};e.existed=rhythm::fs::statusNoFollow(target.c_str(),&st)==0;
        if(e.existed && (!S_ISREG(st.st_mode) || !fileDigest(target,e.backupHash) || !copy(target,work+"/backup/"+e.path,e.backupHash))){error="Cannot back up the current application. Nothing was replaced.";return false;}
    }
    if(!writeText(work+"/plan.json",serialize(m).dump()) || !writeText(work+"/prepared","backup complete\n")){error="Cannot commit update recovery plan.";return false;}
    std::stable_sort(m.files.begin(),m.files.end(),[](const Entry& a,const Entry& b){return (a.path=="eboot.bin")<(b.path=="eboot.bin");});
    for(const auto& e:m.files)if(!replace(work+"/stage/"+e.path,root,e,e.hash)){error="Installation failed; restoring the previous application.";std::string recovery;(void)rollback(root,work,recovery);if(!recovery.empty())error+=" "+recovery;return false;}
    if(!writeText(work+"/committed","update complete\n")){error="Cannot commit update; restoring the previous application.";std::string recovery;(void)rollback(root,work,recovery);return false;}
    (void)unlink((work+"/request").c_str());
    // Committed application files are durable; staging and backups are no longer
    // needed for recovery. Read-only installer downloads are kept separately.
    const auto package=work+"/CH_Encore_Alpha_v"+std::to_string(m.version)+"_update.encore";(void)unlink(package.c_str());
    return true;
}
Recovery recoverPending(const std::string& root,const std::string& work,std::string& error){
    if(exists(work+"/committed")){discard(work+"/stage");discard(work+"/backup");(void)unlink((work+"/prepared").c_str());(void)unlink((work+"/request").c_str());(void)unlink((work+"/committed").c_str());return Recovery::None;}
    if(!exists(work+"/prepared")){(void)unlink((work+"/request").c_str());return Recovery::None;}
    return rollback(root,work,error)?Recovery::Restored:Recovery::Failed;
}

namespace {
struct RuntimePointer {std::string active,previous;bool pending{},attempted{};};
bool generationName(const std::string& s){
    return s.size()>4 && s.size()<96 && s.compare(0,2,"g-")==0 &&
        std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||c=='g'||c=='-';});
}
bool runtimePointer(const std::string& work,RuntimePointer& p){
    Json j;if(!readJson(work+"/runtime.json",j) || !j.is_object() || !j.count("schema") || !j["schema"].is_number_unsigned() || j["schema"]!=1u ||
        !j.count("active") || !j["active"].is_string() || !j.count("previous") || !j["previous"].is_string() ||
        !j.count("pending") || !j["pending"].is_boolean() || !j.count("attempted") || !j["attempted"].is_boolean())return false;
    p.active=j["active"].get<std::string>();p.previous=j["previous"].get<std::string>();
    p.pending=j["pending"].get<bool>();p.attempted=j["attempted"].get<bool>();
    return generationName(p.active) && (p.previous.empty() || generationName(p.previous)) && p.active!=p.previous;
}
bool savePointer(const std::string& work,const RuntimePointer& p){
    return writeText(work+"/runtime.json",Json{{"schema",1u},{"active",p.active},{"previous",p.previous},{"pending",p.pending},{"attempted",p.attempted}}.dump());
}
bool runtimeManifest(const std::string& root,Manifest& m){
    m={};
    Json j;struct stat st{};if(rhythm::fs::statusNoFollow(root.c_str(),&st) || !S_ISDIR(st.st_mode) ||
        !readJson(root+"/.encore-manifest.json",j) || !parse(j,m))return false;
    const auto e=std::find_if(m.files.begin(),m.files.end(),[&](const Entry& f){return f.path==(m.version>=10000?"sce_module/encore-engine.prx":"eboot.bin");});
    std::string hash;uint64_t bytes{};
    return e!=m.files.end() && fileDigest(root+"/"+e->path,hash,&bytes) && hash==e->hash && bytes==e->size;
}
}
bool activateRuntime(const std::string& work,std::string& error,std::atomic<double>* progress){
    error.clear();if(!exists(work+"/request")){error="The update has not been confirmed.";return false;}
    Json j;Manifest m;if(!readJson(work+"/manifest.json",j) || !parse(j,m)){error="No verified update is ready.";return false;}
    // Verify the entire generation before publishing a pointer. Never change app0.
    uint64_t total=0,done=0;for(const auto& e:m.files)total+=e.size;
    for(const auto& e:m.files){std::string hash;uint64_t bytes{};
        if(!safeParents(work+"/stage",e.path) || !fileDigest(work+"/stage/"+e.path,hash,&bytes) || hash!=e.hash || bytes!=e.size){error="Staged update changed. The current build is unchanged.";return false;}
        done+=bytes;if(progress)progress->store(total?double(done)/total:1,std::memory_order_relaxed);}
    const auto manifest=serialize(m).dump();Sha256 digest;digest.add(manifest.data(),manifest.size());
    const auto name="g-"+std::to_string(m.version)+"-"+digest.finish().substr(0,24),root=work+"/runtime/"+name;
    if(!directory(work+"/runtime")){error="Cannot create the runtime library.";return false;}
    if(exists(root)){
        // Reusing a prior download still requires every file to match.
        for(const auto& e:m.files){std::string hash;uint64_t bytes{};if(!safeParents(root,e.path) || !fileDigest(root+"/"+e.path,hash,&bytes) || hash!=e.hash || bytes!=e.size){error="An existing runtime is incomplete.";return false;}}
    }else{
        if(!writeText(work+"/stage/.encore-manifest.json",manifest) || std::rename((work+"/stage").c_str(),root.c_str()) || !syncParent(root)){error="Cannot save the verified runtime.";return false;}
    }
    for(const auto& e:m.files)if(e.path=="eboot.bin" || e.path.compare(0,11,"sce_module/")==0)
        if(chmod((root+"/"+e.path).c_str(),0755)){error="Cannot set runtime module permissions.";return false;}
    RuntimePointer previous;const bool hadPrevious=runtimePointer(work,previous);
    if(!hadPrevious && exists(work+"/runtime.json")){error="The current runtime record requires recovery before updating.";return false;}
    if(hadPrevious && previous.pending){error="The previous runtime has not completed its first launch.";return false;}
    RuntimePointer next{name,hadPrevious?previous.active:std::string{},true,false};
    if(hadPrevious && previous.active==name){(void)unlink((work+"/request").c_str());discard(work+"/stage");return true;}
    if(!savePointer(work,next)){error="Cannot activate the update. The current build is unchanged.";return false;}
    (void)unlink((work+"/request").c_str());
    discard(work+"/stage");
    // Installation runs on the update worker. Reclaim obsolete generations here,
    // rather than deleting hundreds of files on the rendering thread.
    if(DIR* d=opendir((work+"/runtime").c_str())){
        while(auto* e=readdir(d)){const std::string old=e->d_name;if(generationName(old) && old!=next.active && old!=next.previous)discard(work+"/runtime/"+old);}closedir(d);
    }
    return true;
}
bool rejectRuntime(const std::string& work,std::string& error){
    RuntimePointer p;if(!runtimePointer(work,p)){error="Invalid runtime activation record.";return false;}
    if(p.previous.empty()){
        if(unlink((work+"/runtime.json").c_str()) || !syncParent(work+"/runtime.json")){error="Cannot restore the bundled build.";return false;}
    }else{RuntimePointer old{p.previous,{},false,false};if(!savePointer(work,old)){error="Cannot restore the previous runtime.";return false;}}
    return true;
}
bool prepareRuntime(const std::string& work,unsigned embeddedVersion,const std::string& runningGeneration,RuntimeBoot& boot,std::string& error){
    boot={};error.clear();if(!exists(work+"/runtime.json"))return true;
    RuntimePointer p;if(!runtimePointer(work,p)){error="Invalid update activation record; using the bundled build.";return false;}
    // A replacement must match the active generation, including before it is healthy.
    if(!runningGeneration.empty() && runningGeneration!=p.active){error="Runtime activation changed; using the bundled build.";return false;}
    if(p.pending && p.attempted && runningGeneration.empty()){
        if(!rejectRuntime(work,error))return false;boot.restored=true;
        if(!exists(work+"/runtime.json"))return true;
        if(!runtimePointer(work,p)){error="Previous runtime record is invalid.";return false;}
    }
    const auto root=work+"/runtime/"+p.active;Manifest m;
    if(!runtimeManifest(root,m)){
        if(!runningGeneration.empty()){error="Active runtime verification failed.";return false;}
        if(!rejectRuntime(work,error))return false;boot.restored=true;
        if(!exists(work+"/runtime.json"))return true;
        if(!runtimePointer(work,p) || !runtimeManifest(work+"/runtime/"+p.active,m)){error="Previous runtime verification failed; using the bundled build.";return false;}
    }
    if(runningGeneration.empty() && m.version<=embeddedVersion){
        if(unlink((work+"/runtime.json").c_str()) || !syncParent(work+"/runtime.json")){error="Cannot clear a superseded runtime checkpoint.";return false;}return true;
    }
    boot.generation=p.active;boot.root=work+"/runtime/"+p.active;boot.version=m.version;
    const auto e=std::find_if(m.files.begin(),m.files.end(),[&](const Entry& f){return f.path==(m.version>=10000?"sce_module/encore-engine.prx":"eboot.bin");});boot.executableHash=e->hash;
    if(!runningGeneration.empty()){boot.running=true;return true;}
    if(p.pending){p.attempted=true;if(!savePointer(work,p)){error="Cannot save the update launch checkpoint.";return false;}}
    boot.launch=true;return true;
}
bool acknowledgeRuntime(const std::string& work,const std::string& generation,std::string& error){
    RuntimePointer p;if(!generationName(generation) || !runtimePointer(work,p) || p.active!=generation){error="The running update no longer matches the active build.";return false;}
    if(!p.pending)return true;
    if(!p.attempted){error="The runtime was not launched by the bootstrap.";return false;}
    p.pending=false;p.attempted=false;
    if(!savePointer(work,p)){error="Cannot save the successful update checkpoint.";return false;}
    return true;
}
}
