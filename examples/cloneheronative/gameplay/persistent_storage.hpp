// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#pragma once
#include "file_status.hpp"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// app0 is writable for the development folder, but not for installed packages.
// This absolute data root is the SAME physical location used by the existing
// folder build, preserving its songs and profiles while supporting images/PKGs.
#define CH_DATA_ROOT "/data/homebrew/PPSA98743"
#define CH_USERDATA CH_DATA_ROOT "/userdata"
#define CH_SONGS "/data/homebrew/SONGS-VR"
#define CH_LEGACY_SONGS CH_DATA_ROOT "/songs"
namespace rhythm::persistent {
inline std::string identity(const std::string& path) {
    // Runtime generations contain the same bundled charts as app0. Their
    // versioned cache directory must never become part of a saved-score key.
    const std::string runtime=CH_USERDATA "/updates/runtime/";
    if(path.compare(0,runtime.size(),runtime)==0){
        const auto slash=path.find('/',runtime.size());
        if(slash!=std::string::npos && path.compare(runtime.size(),2,"g-")==0 &&
           path.compare(slash,8,"/assets/")==0)return "/app0"+path.substr(slash);
    }
    // Preserve the keys used by existing scores, offsets and dates after moving.
    const std::string songs=CH_SONGS;
    if(path.size()>songs.size() && path.compare(0,songs.size(),songs)==0 && path[songs.size()]=='/')
        return "/app0/songs"+path.substr(songs.size());
    const std::string root=CH_DATA_ROOT;
    if(path.compare(0,root.size(),root)==0 && (path.size()==root.size() || path[root.size()]=='/'))
        return "/app0"+path.substr(root.size());
    return path;
}
inline bool directory(const char* path) {
    if(mkdir(path,0777)!=0 && errno!=EEXIST)return false;
    // Existing shared/FTP-created directories may be owned by another user;
    // inability to chmod them does not prevent ordinary file access.
    (void)chmod(path,0777);
    DIR* dir=opendir(path);if(!dir)return false;closedir(dir);return true;
}
inline bool copyMissing(const std::string& source,const std::string& dest) {
    if(FILE* old=std::fopen(dest.c_str(),"rb")){std::fclose(old);return true;}
    FILE* in=std::fopen(source.c_str(),"rb");if(!in)return false;
    const auto temp=dest+".migration.tmp";FILE* out=std::fopen(temp.c_str(),"wb");
    if(!out){std::fclose(in);return false;}
    std::array<char,16384> bytes;bool ok=true;
    for(;;){const auto n=std::fread(bytes.data(),1,bytes.size(),in);
        if(n && std::fwrite(bytes.data(),1,n,out)!=n){ok=false;break;}
        if(n<bytes.size()){ok=!std::ferror(in);break;}}
    std::fclose(in);
    if(ok)ok=std::fflush(out)==0 && fsync(fileno(out))==0;
    if(std::fclose(out))ok=false;
    if(ok)ok=chmod(temp.c_str(),0666)==0 && std::rename(temp.c_str(),dest.c_str())==0;
    if(!ok)(void)std::remove(temp.c_str());return ok;
}
inline void migrateFlat(const char* oldRoot,const char* newRoot) {
    DIR* dir=opendir(oldRoot);if(!dir)return;
    while(auto* entry=readdir(dir)) {
        const std::string name=entry->d_name;
        if(name=="." || name==".." || name.size()<4 || name.substr(name.size()-4)!=".cfg")continue;
        copyMissing(std::string(oldRoot)+"/"+name,std::string(newRoot)+"/"+name);
    }
    closedir(dir);
}
struct SongMigration {unsigned moved{},failed{};};
// Same-volume moves are atomic. Never overwrite a central library entry or
// follow a legacy symlink. Failed entries remain in the old library for retry.
inline SongMigration migrateSongs(const std::string& from,const std::string& to) {
    SongMigration result;struct stat root{};
    if(from==to || rhythm::fs::statusNoFollow(from.c_str(),&root) || !S_ISDIR(root.st_mode))return result;
    DIR* directory=opendir(from.c_str());if(!directory){++result.failed;return result;}
    while(auto* entry=readdir(directory)) {
        const std::string name=entry->d_name;if(name=="." || name=="..")continue;
        const auto source=from+"/"+name;struct stat st{};
        if(rhythm::fs::statusNoFollow(source.c_str(),&st) || S_ISLNK(st.st_mode)){++result.failed;continue;}
        auto destination=to+"/"+name;unsigned suffix=0;
        while(rhythm::fs::statusNoFollow(destination.c_str(),&st)==0)destination=to+"/"+name+" (migrated "+std::to_string(++suffix)+")";
        if(errno!=ENOENT || std::rename(source.c_str(),destination.c_str()))++result.failed;
        else ++result.moved;
    }
    closedir(directory);if(!result.failed)(void)rmdir(from.c_str());return result;
}
inline bool prepare() {
    if(mkdir("/data/homebrew",0777)!=0 && errno!=EEXIST)return false;
    for(const char* path:{CH_DATA_ROOT,CH_USERDATA,CH_SONGS,CH_USERDATA "/controllers",CH_USERDATA "/external-charts",
        CH_USERDATA "/song-settings",CH_USERDATA "/song-dates",CH_USERDATA "/scores"})if(!directory(path))return false;
    const auto migration=migrateSongs(CH_LEGACY_SONGS,CH_SONGS);
    if(migration.moved || migration.failed)std::fprintf(stderr,"[storage] Central songs migration: moved=%u retained=%u\n",migration.moved,migration.failed);
    copyMissing("/download0/clonehero-settings.cfg",CH_USERDATA "/clonehero-settings.cfg");
    migrateFlat("/download0/clonehero-song-settings",CH_USERDATA "/song-settings");
    migrateFlat("/download0/clonehero-song-dates",CH_USERDATA "/song-dates");
    migrateFlat("/download0/clonehero-scores",CH_USERDATA "/scores");
    return true;
}
}
