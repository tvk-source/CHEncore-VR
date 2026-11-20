// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "application_update.hpp"
#include "update_package.hpp"
#include "update_sha256.hpp"
#include "release_version.hpp"
#include "application_version.hpp"
#include "persistent_storage.hpp"
#include "song_storage.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#define JSON_NOEXCEPTION 1
#include "json.hpp"
#if defined(VK_EXAMPLE_PS5) && !defined(PS5_HOST_REFERENCE)
#include "../../../ps5/src/runtime_module_api.h"
extern "C" int sceSystemServiceLoadExec(const char*,const char* const*);
extern "C" int sceKernelLoadStartModule(const char*,size_t,const void*,unsigned,const void*,int*);
#endif
namespace rhythm::updates {
namespace {
using Json=nlohmann::json;
constexpr const char* repository="https://github.com/MyNameIsAwoo/CHEncore-PS5/releases/download/";
struct Lock {pthread_mutex_t* mutex;explicit Lock(pthread_mutex_t* p):mutex(p){pthread_mutex_lock(p);}~Lock(){pthread_mutex_unlock(mutex);}};
bool nameSafe(const std::string& name){return !name.empty() && name.size()<160 && std::all_of(name.begin(),name.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.';});}
bool hashSafe(const std::string& hash){return hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
}
bool parseRelease(const std::string& text,ReleaseInfo& out,std::string& error){
    error.clear();if(text.size()>4*1024*1024){error="Update response is too large.";return false;}
    const auto root=Json::parse(text,nullptr,false);
    const auto decode=[](const Json& j,ReleaseInfo& info){
        if(!j.is_object() || !j.count("tag_name") || !j["tag_name"].is_string() || !j.count("assets") || !j["assets"].is_array() || j["assets"].size()>64)return false;
        if(j.count("draft") && (!j["draft"].is_boolean() || j["draft"].get<bool>()))return false;
        ReleaseVersion version;const auto tag=j["tag_name"].get<std::string>();if(!releaseVersion(tag,version))return false;
        if(j.count("prerelease") && (!j["prerelease"].is_boolean() || (j["prerelease"].get<bool>() && !version.beta)))return false;
        info.version=version.code;info.title=version.label;info.tag=tag;
        if(j.count("body") && j["body"].is_string())info.notes=j["body"].get<std::string>().substr(0,1400);
        for(const auto& asset:j["assets"]){
            if(!asset.is_object() || !asset.count("name") || !asset["name"].is_string() || !asset.count("browser_download_url") || !asset["browser_download_url"].is_string() || !asset.count("size") || !asset["size"].is_number_unsigned() || !asset.count("digest") || !asset["digest"].is_string())continue;
            UpdateAsset item;item.name=asset["name"].get<std::string>();item.url=asset["browser_download_url"].get<std::string>();item.size=asset["size"].get<uint64_t>();const auto digest=asset["digest"].get<std::string>();
            if(!nameSafe(item.name) || !item.size || item.size>2ull*1024*1024*1024 || item.url.rfind(std::string(repository)+tag+"/",0)!=0 || digest.rfind("sha256:",0)!=0)continue;
            item.hash=digest.substr(7);if(!hashSafe(item.hash))continue;
            int slot=-1;if(item.name==version.prefix+"_update.encore")slot=0;else if(item.name==version.prefix+".pkg")slot=1;else if(item.name==version.prefix+".exfat")slot=2;
            if(slot>=0){if(!info.assets[slot].name.empty())return false;info.assets[slot]=std::move(item);}
        }return true;
    };
    if(root.is_array() && root.size()<=20){
        bool found=false;ReleaseInfo best;
        for(const auto& entry:root){ReleaseInfo info;if(decode(entry,info) && (!found || info.version>best.version)){best=std::move(info);found=true;}}
        if(found){out=std::move(best);return true;}
    } else if(!root.is_discarded()) {ReleaseInfo parsed;if(decode(root,parsed)){out=std::move(parsed);return true;}}
    error="No compatible published PS5 update was found.";return false;
}
ApplicationUpdate::ApplicationUpdate(unsigned current,std::string work,UpdateFetch fetch):current_(current),work_(std::move(work)),fetch_(fetch){}
ApplicationUpdate::~ApplicationUpdate(){cancel_=true;if(joinable_)pthread_join(thread_,nullptr);pthread_mutex_destroy(&mutex_);}
void ApplicationUpdate::publish(UpdateState state,const std::string& message){Lock lock(&mutex_);message_=message;state_.store(state,std::memory_order_release);}
ApplicationUpdate::Snapshot ApplicationUpdate::snapshot() const{Lock lock(&mutex_);return {state(),release_,message_,path_};}
void ApplicationUpdate::poll(){if(joinable_ && done_.load(std::memory_order_acquire)){pthread_join(thread_,nullptr);joinable_=false;}}
bool ApplicationUpdate::begin(bool download){
    poll();if(joinable_)return false;downloadJob_=download;cancel_=false;done_=false;progress_=0;
    publish(installJob_?UpdateState::Installing:download?UpdateState::Downloading:UpdateState::Checking,
        installJob_?"Preparing the verified update...":download?"Downloading update...":"Checking GitHub releases...");
    if(pthread_create(&thread_,nullptr,worker,this)){publish(UpdateState::Error,"Could not start update worker.");return false;}
    joinable_=true;return true;
}
bool ApplicationUpdate::check(){publish(UpdateState::Error,"VR development build: upstream updates disabled.");return false;}
bool ApplicationUpdate::selfTest(bool runtime){poll();if(joinable_)return false;installJob_=false;selfTestJob_=true;runtimeTestJob_=runtime;return begin(false);}
bool ApplicationUpdate::download(UpdateFormat format){if(state()!=UpdateState::Available)return false;installJob_=false;format_=format;return begin(true);}
bool ApplicationUpdate::install(std::string& error){
    if(state()!=UpdateState::Ready){error="No verified update is ready.";return false;}
    poll();if(joinable_){error="The update worker is finishing. Try again shortly.";return false;}
    if(!requestInstall(work_,error))return false;installJob_=true;return begin(false);
}
void* ApplicationUpdate::worker(void* p){auto* self=static_cast<ApplicationUpdate*>(p);self->run();self->done_.store(true,std::memory_order_release);return nullptr;}
void ApplicationUpdate::run(){
    std::string error;
    if(installJob_){
        if(!activateRuntime(work_,error,&progress_)){publish(UpdateState::Error,error);return;}
        publish(UpdateState::RestartReady,"Restarting with the verified update...");return;
    }
    if(selfTestJob_) {
        if(runtimeTestJob_){
            publish(UpdateState::Downloading,"Verifying the console runtime test...");
            if(!stagePackage(work_+"/runtime-selftest.encore",work_,current_+2,error,&cancel_,&progress_)){publish(UpdateState::Error,error);return;}
            publish(UpdateState::Ready,"Console runtime test is ready.");return;
        }
        // Probe public HTTPS metadata + a real redirected release attachment.
        // The application install itself targets an isolated fixture tree.
        std::string response;const auto collect=[](const void* data,size_t count,void* user){static_cast<std::string*>(user)->append(static_cast<const char*>(data),count);return true;};
        if(!fetch_("https://api.github.com/repos/MyNameIsAwoo/CHEncore-PS5/releases?per_page=10",collect,&response,4*1024*1024,cancel_,error)){publish(UpdateState::Error,error);return;}
        ReleaseInfo release;if(!parseRelease(response,release,error)){publish(UpdateState::Error,error);return;}
        const auto metadata=Json::parse(response,nullptr,false);const Json* selected=&metadata;
        if(metadata.is_array()){selected=nullptr;for(const auto& entry:metadata)if(entry.is_object() && entry.count("tag_name") && entry["tag_name"]==release.tag){selected=&entry;break;}}
        if(!selected){publish(UpdateState::Error,"QA release metadata is unavailable.");return;}
        std::string url,expected;uint64_t size=0;
        for(const auto& asset:(*selected)["assets"])if(asset.is_object() && asset.count("name") && asset["name"]=="README.txt" && asset.count("browser_download_url") && asset["browser_download_url"].is_string() && asset.count("digest") && asset["digest"].is_string() && asset.count("size") && asset["size"].is_number_unsigned()) {
            url=asset["browser_download_url"].get<std::string>();const auto digest=asset["digest"].get<std::string>();if(digest.rfind("sha256:",0)==0)expected=digest.substr(7);size=asset["size"].get<uint64_t>();break;
        }
        if(url.rfind(repository,0)!=0 || !hashSafe(expected) || !size || size>65536){publish(UpdateState::Error,"QA release attachment is unavailable.");return;}
        struct Digest {Sha256 hash;uint64_t bytes{};} digest;
        const auto consume=[](const void* bytes,size_t count,void* user){auto& d=*static_cast<Digest*>(user);d.hash.add(bytes,count);d.bytes+=count;return true;};
        if(!fetch_(url,consume,&digest,size,cancel_,error) || digest.bytes!=size || digest.hash.finish()!=expected){publish(UpdateState::Error,error.empty()?"QA HTTPS attachment hash mismatch.":error);return;}
        const auto app=work_+"/selftest-app",work=work_+"/selftest-work";
        (void)mkdir(work_.c_str(),0777);(void)mkdir(app.c_str(),0777);(void)mkdir((app+"/sce_sys").c_str(),0777);(void)mkdir((app+"/userdata").c_str(),0777);
        const auto write=[](const std::string& path,const char* text){FILE* f=std::fopen(path.c_str(),"wb");if(!f)return false;bool ok=std::fputs(text,f)>=0 && std::fflush(f)==0 && fsync(fileno(f))==0;if(std::fclose(f))ok=false;return ok;};
        if(!write(app+"/eboot.bin","old fixture") || !write(app+"/sce_sys/param.json","old fixture parameters") || !write(app+"/userdata/keep.txt","preserved")){publish(UpdateState::Error,"QA fixture storage failed.");return;}
        if(!stagePackage(work_+"/selftest.encore",work,41,error,&cancel_,&progress_) || !requestInstall(work,error) || !applyPending(app,work,error)){publish(UpdateState::Error,error);return;}
        std::string hash;
        if(!fileDigest(app+"/userdata/keep.txt",hash) || hash!=[](){Sha256 s;s.add("preserved",9);return s.finish();}()){publish(UpdateState::Error,"QA data preservation failed.");return;}
        (void)unlink((work+"/committed").c_str());
        if(recoverPending(app,work,error)!=Recovery::Restored || !fileDigest(app+"/eboot.bin",hash) || hash!=[](){Sha256 s;s.add("old fixture",11);return s.finish();}()){publish(UpdateState::Error,error.empty()?"QA rollback failed.":error);return;}
        const auto legacy=work_+"/selftest-legacy",central=work_+"/selftest-central",bundled=work_+"/selftest-bundled";
        for(const auto& folder:{legacy,central,bundled,legacy+"/Song",bundled+"/Bundled"})(void)mkdir(folder.c_str(),0777);
        if(!write(legacy+"/Song/notes.chart","fixture chart") || !write(bundled+"/Bundled/notes.chart","bundled chart")){publish(UpdateState::Error,"QA song fixture write failed.");return;}
        const auto migrated=rhythm::persistent::migrateSongs(legacy,central);
        if(migrated.moved!=1 || migrated.failed){publish(UpdateState::Error,"QA native song migration failed.");return;}
        const auto removal=rhythm::storage::removeSongs({central+"/Song",bundled+"/Bundled"},central,bundled,work_+"/selftest-removed.cfg",&cancel_);
        if(removal.removed!=1 || removal.hidden!=1 || removal.failed || !rhythm::storage::bundleRemoved(bundled+"/Bundled",bundled,rhythm::storage::removedBundles(work_+"/selftest-removed.cfg"))){
            char detail[200];std::snprintf(detail,sizeof(detail),"QA native removal failed: removed=%u hidden=%u failed=%u errno=%d",removal.removed,removal.hidden,removal.failed,errno);publish(UpdateState::Error,std::string(detail)+" "+removal.error);return;
        }
        const bool writable=writableApplication("/app0");
        publish(UpdateState::Current,std::string("PASS native metadata, redirected HTTPS attachment hash, staged install, preserved data and rollback, song migration and removal; app writable=")+(writable?"yes":"no"));return;
    }
    if(!downloadJob_){
        std::string text;
        const auto sink=[](const void* bytes,size_t size,void* user){static_cast<std::string*>(user)->append(static_cast<const char*>(bytes),size);return true;};
        if(!fetch_("https://api.github.com/repos/MyNameIsAwoo/CHEncore-PS5/releases?per_page=10",sink,&text,4*1024*1024,cancel_,error)){publish(cancel_?UpdateState::Cancelled:UpdateState::Error,error);return;}
        ReleaseInfo info;if(!parseRelease(text,info,error)){publish(UpdateState::Error,error);return;}
        const unsigned latest=info.version;{Lock lock(&mutex_);release_=std::move(info);path_.clear();}
        publish(latest>current_?UpdateState::Available:UpdateState::Current,latest>current_?"A new release is available.":"You have the latest published build.");return;
    }
    UpdateAsset asset;unsigned version;{Lock lock(&mutex_);asset=release_.assets[size_t(format_)];version=release_.version;}
    if(asset.name.empty()){publish(UpdateState::Error,"This release does not include the selected update format.");return;}
    if(mkdir(work_.c_str(),0777) && errno!=EEXIST){publish(UpdateState::Error,"Update storage is unavailable.");return;}
    const auto destination=work_+"/"+asset.name,temp=destination+".part";
    struct Sink {FILE* file{};Sha256 hash;uint64_t received{},size{};std::atomic<double>* progress;};
    Sink sink;sink.file=std::fopen(temp.c_str(),"wb");sink.size=asset.size;sink.progress=&progress_;
    if(!sink.file){publish(UpdateState::Error,"Cannot create update download.");return;}
    const auto write=[](const void* bytes,size_t size,void* user){auto& sink=*static_cast<Sink*>(user);if(size>sink.size-sink.received || std::fwrite(bytes,1,size,sink.file)!=size)return false;sink.hash.add(bytes,size);sink.received+=size;sink.progress->store(double(sink.received)/sink.size,std::memory_order_relaxed);return true;};
    bool ok=fetch_(asset.url,write,&sink,asset.size,cancel_,error);
    ok=ok && !cancel_ && sink.received==asset.size && sink.hash.finish()==asset.hash && std::fflush(sink.file)==0 && fsync(fileno(sink.file))==0;
    if(std::fclose(sink.file))ok=false;
    if(!ok){(void)unlink(temp.c_str());publish(cancel_?UpdateState::Cancelled:UpdateState::Error,error.empty()?"Update checksum or size verification failed.":error);return;}
    if(std::rename(temp.c_str(),destination.c_str())){publish(UpdateState::Error,"Cannot save downloaded update.");return;}
    {Lock lock(&mutex_);path_=destination;}
    if(format_==UpdateFormat::Folder){
        publish(UpdateState::Downloading,"Verifying application files...");progress_=0;
        if(!stagePackage(destination,work_,version,error,&cancel_,&progress_)){publish(cancel_?UpdateState::Cancelled:UpdateState::Error,error);return;}
        // The verified staged files are now the installation source. Do not
        // retain a second full copy of the application download.
        (void)unlink(destination.c_str());
        {Lock lock(&mutex_);path_.clear();}
        publish(UpdateState::Ready,"Update verified. Restart and play? Songs, scores and profiles are kept.");
    } else publish(UpdateState::Downloaded,"Download verified. Install this file using your compatible package installer or exFAT launcher.");
}
}
namespace {
bool skipShutdownUpdate=false;
bool restartRuntimeOnExit=false;
std::string runningGeneration,runtimeResourceRoot;
bool runtimeAcknowledged=false;
void recordUpdate(const std::string& result){
    FILE* file=std::fopen(CH_USERDATA "/updates/last-result.txt","wb");if(file){std::fprintf(file,"%s\n",result.c_str());std::fflush(file);(void)fsync(fileno(file));std::fclose(file);}
}
}
namespace {
bool launchRuntime(const rhythm::updates::RuntimeBoot& boot){
#if defined(VK_EXAMPLE_PS5) && !defined(PS5_HOST_REFERENCE)
    const auto modulePath=boot.root+"/sce_module/encore-engine.prx";struct stat module{};
    if(stat(modulePath.c_str(),&module)==0){
        EncoreRuntimeExports api{};EncoreRuntimeRequest request{ENCORE_RUNTIME_MODULE_MAGIC,ENCORE_RUNTIME_MODULE_ABI,sizeof(EncoreRuntimeRequest),&api};
        int started=0;const int handle=sceKernelLoadStartModule(modulePath.c_str(),sizeof(request),&request,0,nullptr,&started);
        if(handle>=0 && started==0 && api.magic==ENCORE_RUNTIME_MODULE_MAGIC && api.abi==ENCORE_RUNTIME_MODULE_ABI && api.bytes==sizeof(api) && api.run){
            auto argument="--encore-runtime="+boot.generation;char* args[]={argument.data(),nullptr};
            (void)api.run(1,args);
            recordUpdate("The updated engine returned without its normal shutdown.");
            (void)sceSystemServiceLoadExec("exit",nullptr);for(;;)usleep(100000);
        }
        char text[160];std::snprintf(text,sizeof(text),"Runtime module could not start (%08x/%08x); restoring the previous build.",unsigned(handle),unsigned(started));recordUpdate(text);
        std::string error;(void)rhythm::updates::rejectRuntime(CH_USERDATA "/updates",error);return false;
    }
    recordUpdate("The verified runtime module is missing; using the bundled build.");
    std::string error;(void)rhythm::updates::rejectRuntime(CH_USERDATA "/updates",error);
#else
    (void)boot;
#endif
    return false;
}
}
extern "C" void encore_boot_application_runtime(){
    rhythm::updates::RuntimeBoot boot;std::string error;
    if(!rhythm::updates::prepareRuntime(CH_USERDATA "/updates",rhythm::updates::applicationVersion,runningGeneration,boot,error)){
        recordUpdate(error);return;
    }
    if(boot.restored)recordUpdate("The previous build was restored after an incomplete update launch.");
    if(boot.running)runtimeResourceRoot=boot.root;
    else if(boot.launch)(void)launchRuntime(boot);
}
extern "C" const char* encore_application_resource_root(){return runtimeResourceRoot.c_str();}
extern "C" void encore_restart_application_runtime(){restartRuntimeOnExit=true;}
extern "C" void encore_application_rendered(){
    if(runtimeAcknowledged || runningGeneration.empty() || runtimeResourceRoot.empty())return;
    runtimeAcknowledged=true;std::string error;
    if(rhythm::updates::acknowledgeRuntime(CH_USERDATA "/updates",runningGeneration,error))recordUpdate("Update started successfully.");
    else recordUpdate(error);
}
extern "C" bool encore_update_process_arguments(int argc,char** argv){
#if defined(VK_EXAMPLE_PS5) && !defined(PS5_HOST_REFERENCE)
    for(int i=0;i<argc;++i)if(argv && argv[i] && std::strncmp(argv[i],"--encore-runtime=",17)==0)
        runningGeneration=argv[i]+17;
#else
    (void)argc;(void)argv;
#endif
    return false;
}
extern "C" bool encore_recover_application_update(){
    std::string error;const auto result=rhythm::updates::recoverPending("/app0",CH_USERDATA "/updates",error);
    if(result==rhythm::updates::Recovery::None)return false;
    skipShutdownUpdate=true;recordUpdate(result==rhythm::updates::Recovery::Restored?"Previous application restored after an interrupted update. Reopen the game.":error);return true;
}
extern "C" void encore_apply_application_update(){
    if(skipShutdownUpdate)return;
    struct stat st{};const bool requested=stat(CH_USERDATA "/updates/request",&st)==0;
    if(!requested && !restartRuntimeOnExit)return;
    std::string error;
    if(requested && !rhythm::updates::activateRuntime(CH_USERDATA "/updates",error)){recordUpdate(error);return;}
    // After a rendered session, first restart the embedded bootstrap. The
    // fresh process validates and launches the pending runtime before creating
    // any GPU/audio state. Do not mark its launch attempted until that point.
#if defined(VK_EXAMPLE_PS5) && !defined(PS5_HOST_REFERENCE)
    const int result=sceSystemServiceLoadExec("/app0/eboot.bin",nullptr);
    if(result==0)for(;;)usleep(100000);
    char message[128];std::snprintf(message,sizeof(message),"Update is prepared; restart the game to apply it (%08x).",unsigned(result));recordUpdate(message);
#endif
}
