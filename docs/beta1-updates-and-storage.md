# Beta 1.0: updates and song management

## Updates

Settings > General > Check for Updates uses the public PS5 GitHub release
channel. Automatic Update Checks defaults to On and can be turned off. A quiet
worker checks once after the main-menu opening cue. A newer release opens a
confirmation at the main menu when no other player dialog is open. Green chooses
**Update**; Red chooses **Later**. Updates never interrupt a song.

The download must match the release asset's SHA-256 digest and exact size. Each
application file is then verified separately. Green confirms **Restart and play**.
Final preparation runs on the worker while the game displays progress. After
normal audio, worker and Vulkan teardown, the bootstrap restarts and loads the replacement.

The replacement engine is a native shared module built from the same PIC game
and RADV objects as the embedded executable. A versioned initialization contract
returns its entry point to the bootstrap; the engine runs on the main thread after
the loader returns, before the usual pad, audio and Vulkan initialization. Assets
and shaders reside alongside the module in a complete runtime under
`/data/homebrew/PPSA98743/userdata/updates`. The original installed folder, PKG or
exFAT container stays intact. The PS5 app-information screen may continue to show
the base package version; the in-game version identifies the running engine.
A small activation record selects the new runtime;
no installer service or access token is needed. The launcher must provide the
usual writable `/data` sandbox mount used for the game's existing storage.

A replacement is acknowledged only after it has rendered successfully. If its
first startup is interrupted, the next launch restores the previous runtime or
the embedded build. A newer manual installation takes precedence over an older
cached runtime. The active and previous generations are retained; obsolete ones
and redundant package downloads are reclaimed off the rendering thread. Songs,
profiles, scores and other userdata are excluded from the update payload. Bundled
song score identities remain stable when the runtime directory changes.

The updater starts with **Beta 1.0**. Older alphas need one manual update first.
New releases must attach `CH_Encore_Beta_1.0_update.encore` (with the
matching version in its filename) alongside the normal install formats. Build it
with `ps5/tools/build-update-package.py`. The bounded streaming format carries a
full application manifest, sizes and per-file hashes, with no full-package memory
allocation or decompressor required on the console.

On the PS5 Pro (firmware 11.00, ShadowMount+ 1.7beta11),
the full worker installation, bootstrap restart, replacement engine rendering,
health acknowledgement and fresh-launch selection passed on the folder install.
The production shared engine also rendered from an actual read-only exFAT image
whose bootstrap had no embedded assets or shaders. It completed a 240-frame run
at approximately 59.9 FPS and exited normally.

An installed native PKG also loaded the shared engine, rendered, exited normally
and selected it on a fresh launch. Full update staging, activation and automatic
restart also passed from the installed PKG and read-only exFAT image. The original
package writers failed before
application entry; the final packaging path uses the pinned released PS5 UltraPack
writer and the application's own metadata module. See [the package validation
record](native-pkg-validation.md). No console payload change was needed.

## Internal songs

The shared internal library is `/data/homebrew/SONGS`. Song downloads and the
existing USB copy actions use it. At startup, folders from
`/data/homebrew/PPSA98743/songs` move there. Existing destinations are never
overwritten: collisions get a numbered migrated name. Failed moves remain in
the legacy root, which remains a scan fallback. USB libraries still use a
root-level `SONGS` folder on an exFAT drive.

Normal migrated paths keep the same score/offset/date identity. Files in
collision-renamed folders are preserved, although their path-based record keys
may differ. Profiles and scores remain under the existing userdata directory.

## Remove Songs

Settings > General > Remove Songs sits below the copy actions. Strum browses;
Green marks songs; Yellow reviews the selection; Green confirms the displayed
song count. Red cancels. Removal runs on the storage worker. Stop requests take
effect between songs. Saved scores and profiles are kept.

Installed song folders are moved out of the scanner atomically before erasure.
Bundled songs are included. Their removal is recorded before any file operation,
so an update or reinstall cannot make them reappear. Writable bundled files may
be erased; embedded files in PKGs/read-only images remain in the image but are
hidden from the library. USB songs are managed separately through the copy
commands and are not included in this internal-song removal list.

## Validation

`bash ps5/tools/test-updates-and-storage.sh` runs ASan/UBSan coverage for hashing,
release/channel validation, transfer corruption, staging, install/recovery,
userdata preservation, immutable runtime activation, first-start failure,
corruption recovery, manual-upgrade precedence, traversal/symlinks, migration,
collisions, bundled-song
removal and cancellation. The native QA path additionally checks public HTTPS,
redirected release downloads and installation/rollback in an isolated fixture
folder. It does not replace the running application during that fixture test.

RB4 controller work and its physical-validation limits are documented in
[the controller notes](rb4-bluetooth-and-rcm.md).
