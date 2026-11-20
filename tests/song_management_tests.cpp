// Copyright (C) 2026 HOWL
// SPDX-License-Identifier: MIT
#include "gameplay/song_storage.hpp"
#include "gameplay/persistent_storage.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
namespace fs=std::filesystem;
static void put(const fs::path& path,const char* data="chart"){fs::create_directories(path.parent_path());std::ofstream(path)<<data;}
int main(){
 assert(rhythm::persistent::identity(CH_USERDATA "/updates/runtime/g-10002-abc/assets/songs/Bundled")=="/app0/assets/songs/Bundled");
 char tmp[]="/tmp/encore-songs-XXXXXX";assert(mkdtemp(tmp));fs::path root=tmp;
 auto old=root/"old",central=root/"SONGS",bundles=root/"bundled",registry=root/"removed.cfg";
 put(old/"Pack"/"Song"/"notes.chart");put(old/"Collision"/"notes.chart","old");put(central/"Collision"/"notes.chart","new");
 fs::create_directory_symlink(root,old/"outside");
 auto migrated=rhythm::persistent::migrateSongs(old.string(),central.string());
 assert(migrated.moved==2 && migrated.failed==1);
 assert(fs::exists(central/"Pack"/"Song"/"notes.chart"));
 assert(fs::exists(central/"Collision (migrated 1)"/"notes.chart"));
 std::string contents;std::ifstream(central/"Collision"/"notes.chart")>>contents;assert(contents=="new");
 auto again=rhythm::persistent::migrateSongs(old.string(),central.string());assert(!again.moved && again.failed==1);
 assert(rhythm::persistent::identity("/data/homebrew/SONGS-VR/Pack/Song/notes.chart")==rhythm::persistent::identity("/data/homebrew/PPSA98743/songs/Pack/Song/notes.chart"));
 put(bundles/"Original"/"notes.chart");put(root/"keep"/"notes.chart","untouched");
 fs::create_directory_symlink(root/"keep",central/"escape");
 auto result=rhythm::storage::removeSongs({(central/"Pack"/"Song").string(),(central/"Pack"/"Song").string(),(bundles/"Original").string()},central.string(),bundles.string(),registry.string());
 assert(result.removed==1 && result.hidden==1 && !result.failed);
 assert(!fs::exists(central/"Pack"/"Song"));
 auto removed=rhythm::storage::removedBundles(registry.string());assert(removed.size()==1 && removed[0]=="Original");
 assert(rhythm::storage::bundleRemoved((bundles/"Original").string(),bundles.string(),removed));
 // Reinstalling a bundled file cannot make a removed song appear again.
 put(bundles/"Original"/"notes.chart");
 assert(rhythm::storage::bundleRemoved((bundles/"Original").string(),bundles.string(),rhythm::storage::removedBundles(registry.string())));
 result=rhythm::storage::removeSongs({central.string(),root.string(),(central/"escape").string(),(central/"../keep").string()},central.string(),bundles.string(),registry.string());
 assert(result.failed==4 && !result.removed && fs::exists(root/"keep"/"notes.chart"));
 // Never remove a pack that contains unselected songs in child directories.
 put(central/"Pack2"/"notes.chart");put(central/"Pack2"/"Child"/"notes.chart");
 result=rhythm::storage::removeSongs({(central/"Pack2").string()},central.string(),bundles.string(),registry.string());
 assert(result.failed==1 && fs::exists(central/"Pack2"/"Child"/"notes.chart"));
 // Cancelling before the first song has no destructive effects.
 std::atomic<bool> cancel{true};result=rhythm::storage::removeSongs({(central/"Collision").string()},central.string(),bundles.string(),registry.string(),&cancel);
 assert(result.cancelled && !result.removed && fs::exists(central/"Collision"/"notes.chart"));
 fs::remove_all(root);std::puts("PASS central migration, collisions, score identities, internal/bundled removal, persistent hiding, duplicate selections, bounds, symlinks, nested songs and cancellation");
}
