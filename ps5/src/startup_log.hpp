// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#pragma once
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <initializer_list>
#include <sys/stat.h>
#include <unistd.h>

// Synchronous startup-only checkpoints: no thread, heap allocation or GPU calls.
// Separate from the gameplay event log, which starts after Vulkan initialization.
namespace encore_startup {
inline bool enabled=false;
inline const char* path="/data/homebrew/PPSA98743/userdata/logs/encore-startup.log";
inline void checkpoint(const char* stage) noexcept {
    if(!enabled)return;
    FILE* file=std::fopen(path,"ab");
    if(!file)return;
    timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);
    std::fprintf(file,"%lld.%03ld %s\n",static_cast<long long>(ts.tv_sec),ts.tv_nsec/1000000,stage);
    std::fflush(file);(void)fsync(fileno(file));std::fclose(file);
}
inline bool begin(const char* title) noexcept {
    for(const char* dir:{"/data/homebrew","/data/homebrew/PPSA98743","/data/homebrew/PPSA98743/userdata","/data/homebrew/PPSA98743/userdata/logs"})
        if(mkdir(dir,0777)!=0 && errno!=EEXIST)return false;
    char previous[512];std::snprintf(previous,sizeof(previous),"%s.previous",path);
    (void)std::rename(path,previous);
    FILE* file=std::fopen(path,"wb");if(!file)return false;
    std::fprintf(file,"Encore startup checkpoints: %s; utc_epoch=%lld\n",title,static_cast<long long>(std::time(nullptr)));
    std::fflush(file);(void)fsync(fileno(file));std::fclose(file);
    enabled=true;checkpoint("main entered; before platform initialization");return true;
}
inline void finish() noexcept {checkpoint("startup complete; entering render loop");enabled=false;}
}
