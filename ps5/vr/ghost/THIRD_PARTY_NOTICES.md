# Third-party notices

Ghost VR is Copyright (C) 2026 theghostonline and is distributed under the GNU General Public
License v3.0 or later (see `LICENSE`). Its releases carry the components below, each under its own
licence. The texts are in `LICENSES/`. Everything GPL or LGPL that ships is built from the source in
this repository or from the upstream source linked here, at the version given.

PS5 React, created by half144 and PS5 React contributors.
https://github.com/half144/ps5-react

## The app (`Ghost-VR-PPSA99480.zip`)

| Component | Where | License |
| --- | --- | --- |
| PS5 React runtime and QuickJS | the title's `eboot.bin`; its own notices are in the title's `notices/` | GPL-3.0-or-later with the PS5 React attribution term; MIT |
| Playden's Steam client (sign-in, app info, depot download), from [leolobato/playden-ps5](https://github.com/leolobato/playden-ps5) commit 99e7d17, ported; its Steam online handoff, Valve client install and zip reader from commit ff49202 | `engine/playden/` | GPL-3.0-or-later; `content_language.*` and `title_allow_list.h` MIT |
| Playden's Steam host (`steam.exe`), from [leolobato/playden-ps5](https://github.com/leolobato/playden-ps5) commit ff49202 `tools/steam-host`, with its files in the game's bottle under `C:\ghostvr` | `engine/steam-host/`, shipped as `data/kit/steam-host.exe` | GPL-3.0-or-later |
| miniz 3.0.2 (Rich Geldreich, RAD Game Tools, Valve Software) | `engine/playden/third_party/miniz/` | MIT |
| QR Code generator library, Copyright (c) Project Nayuki | `engine/playden/` | MIT |
| LZMA SDK 24.09 (Igor Pavlov) | `engine/playden/` | public domain |
| zstd 1.5.6 headers (Meta Platforms) | `engine/playden/` | BSD-3-Clause |
| stb_image, stb_image_write (Sean Barrett) | `engine/third_party/` | MIT or public domain |
| jxrlib, the JPEG XR codec (Copyright © Microsoft Corp.) | `engine/third_party/jxrlib/` | BSD-2-Clause |
| Mozilla's CA certificate list (2026-09-25) | `data/kit/cacert.pem` | MPL-2.0 (notice in the file) |
| Noto Emoji, Noto Sans Symbols 2, Inter | the title's `fonts/` | SIL Open Font License 1.1 |

Steam online downloads Valve's Steam client from Valve on the player's console (client-update.akamai.steamstatic.com). Ghost VR doesn't ship any of Valve's files.

## TGOVR (`tgovr.elf`)

| Component | Where | License |
| --- | --- | --- |
| `pt.c`, `pt.h` from [elfldr](https://github.com/ps5-payload-dev/elfldr) by John Törnblom, changed (registers of a chosen thread, thread list, timed attach) | `ps4/payload/` | GPL-3.0-or-later |
| `fself.c`, a C version of `make_fself.py` from the [PS5 payload SDK](https://github.com/ps5-payload-dev/sdk) (John Törnblom) | `ps4/payload/` | GPL-3.0-or-later |
| `selfpager.c`, `selfpager.h` from [ps5-self-pager](https://github.com/idlesauce/ps5-self-pager) by idlesauce | `ps4/payload/` | Unlicense |
| PS4 VR structure layouts after [shadPS4](https://github.com/shadps4-emu/shadPS4)'s HLE (hmd, vr_tracker, camera, move, pad) | `ps4/agent/psvr1.h` | GPL-2.0-or-later |
| `rdna2gcn_tables.h`, opcode numbers from LLVM's `llvm-mc` encodings | `ps4/payload/` | Apache-2.0 WITH LLVM-exception |
| EASU (the optional upscaler), a port of `FsrEasuF` from [AMD FidelityFX Super Resolution 1.0](https://github.com/GPUOpen-Effects/FidelityFX-FSR), Copyright (c) 2021 Advanced Micro Devices, Inc. | `ps4/gpu/compose.c` | MIT |

No Sony software is included. The PS VR2 libraries and the reprojection shader are built on your
console from its own firmware the first time TGOVR runs.

## PC game tiles (`data/kit/shell.zip` in the app)

| Component | License |
| --- | --- |
| [prospero-win](https://github.com/mpereiraesaa/prospero-win) by mpereiraesaa, as Playden's fork [leolobato/prospero-win](https://github.com/leolobato/prospero-win) commit d9c00be: the tile's `eboot.bin` and `bdj.elf` (the second process for Steam online), rebuilt from that source with `pc/vrtitle/patches/0001-prospero-win-vr-title.patch` and `0003-steam-online.patch` | GPL-3.0 |
| [PS5-Lapy-JB-Daemon](https://github.com/mpereiraesaa/PS5-Lapy-JB-Daemon) (Copyright (c) 2026 Arksama, Team PHU), as in Playden's `third_party` (commit 66ab4f1), built for the `FAKE79` title rule | MIT |
| prospero-win's `libc.prx` from Playden's runtime kit 0.28 | as in that kit |

## The PC runtime (`runtime-kit021-*.zip`; `runtime-pw020-*.zip` and `runtime-kit012-*.zip` on v0.9.0-beta)

Downloaded by the app the first time a PC game needs it. Each runtime carries prospero-win's own
`LICENSE`, `LICENSES/`, `THIRD_PARTY.md` and `SOURCES.txt` in `licenses/prospero-win/`, which cover
what its package ships: Wine (LGPL-2.1-or-later), Mesa and RADV (MIT), DXVK (zlib), GnuTLS
(LGPL-2.1-or-later), llvm-mingw's runtime and the rest. `kit021` also carries the runtime kit's own
licence files and `SOURCES.md` in `licenses/runtime-kit/` (vkd3d-proton, GStreamer, FFmpeg, Wine Mono
and the rest of what the kit ships).

| Component | License |
| --- | --- |
| Wine from [Playden](https://github.com/leolobato/playden-ps5)'s runtime kit 0.21 (`kit021`): Playden's prospero-win fork, [leolobato/prospero-win](https://github.com/leolobato/prospero-win) commit 5bd69cc; `ntdll.prx` patched by `pc/vrtitle/patch_ntdll_runs.py` and `patch_ntdll_unregister.py`, `kernel32.dll` by `patch_kernel32.py`, with the kit's [DXVK](https://github.com/doitsujin/dxvk) 2.6.2 and [vkd3d-proton](https://github.com/HansKristian-Work/vkd3d-proton) 3.0.1 | LGPL-2.1-or-later; zlib; LGPL-2.1-or-later |
| Wine from [Playden](https://github.com/leolobato/playden-ps5)'s runtime kit 0.12 (`kit012`, for Quest games): Playden's prospero-win fork, [leolobato/prospero-win](https://github.com/leolobato/prospero-win) commit f729534, on Wine 490f6d5; `ntdll.prx` and `kernel32.dll` patched by `pc/vrtitle/patch_ntdll_runs.py` and `patch_kernel32.py` | LGPL-2.1-or-later |
| [PC5](https://github.com/bornaradusin/PC5) v1.0.0 template prefix, with [DXVK](https://github.com/doitsujin/dxvk) 2.6.2-prospero2 and [VKD3D-Proton](https://github.com/HansKristian-Work/vkd3d-proton) 2.14.1 | LGPL-2.1; zlib; LGPL-2.1-or-later |
| [gbe_fork](https://github.com/Detanup01/gbe_fork) release-2026_09_27, `steam_api64.dll`, unmodified, with its `LICENSE` beside it | LGPL-3.0 |
| wineopenxr (this repo, `pc/wineopenxr/`), after Proton's design; `include/dxvk-interop.h` and `include/vkd3d-proton-interop.h` copied from [Proton](https://github.com/ValveSoftware/Proton) commit e91ca2b | GPL-3.0-or-later; LGPL-2.1-or-later (the two headers) |
| OpenXR loader 1.1.65 (The Khronos Group), `openxr_loader.dll` | Apache-2.0 |
| OpenXR headers (The Khronos Group), `pc/ps5xr/third_party/openxr/include` | Apache-2.0 |
| RE7 VR Reloaded 1.0.3 (`pc/games/re7/reloaded`), the Resident Evil 7 add-on, carried as in RE7-PSVR2 | as its author released it, no licence stated |

## Stream to headset

The PC runtime's `wineopenxr.prx` (in `data/kit/runtime-fix.zip` in the app), the capture helper
`gv-capture.elf` (in the same zip) and the headset app `GhostVR-Wireless-26.9-gv1.apk`. Their source is
`remote/` in this repository: the server and backend, the helper, and the patch series
`remote/patches/wivrn/` that the headset app is built from (`remote/tools/build_apk.sh`).

| Component | Where | License |
| --- | --- | --- |
| [WiVRn](https://github.com/WiVRn/WiVRn) v26.9 by Guillaume Meunier, Patrick Nicolas and WiVRn contributors: its `common/` and server pieces, with `remote/patches/wivrn/` | `wineopenxr.prx` | GPL-3.0 |
| WiVRn's `external/`: `magic_enum.hpp` (Daniil Goncharov), `mdns.h` (Mattias Jansson), `inplace_vector.hpp` (Beman Project) | `wineopenxr.prx` | MIT; Unlicense; Apache-2.0 WITH LLVM-exception |
| [Monado](https://gitlab.freedesktop.org/monado/monado) commit f037264d, math helpers from `src/xrt/auxiliary/math` | `wineopenxr.prx` | BSL-1.0 |
| [Eigen](https://eigen.tuxfamily.org) 3.4.0 headers | `wineopenxr.prx` | MPL-2.0 |
| [Boost.PFR](https://github.com/boostorg/pfr) from Boost 1.89.0 | `wineopenxr.prx` | BSL-1.0 |
| [OpenSSL](https://www.openssl.org) 3.5.2 `libcrypto`, static | `wineopenxr.prx` | Apache-2.0 |
| LLVM's libc++ and libc++abi from the PS5 payload SDK, static | `wineopenxr.prx` | Apache-2.0 WITH LLVM-exception |
| Ghost VR Wireless: the WiVRn v26.9 headset app with `remote/patches/wivrn/` (one side-by-side picture, Ghost VR's name and icon), and what WiVRn's build bundles into it: Boost 1.89.0 (BSL-1.0), OpenXR loader 1.1.58 (Apache-2.0), KTX-Software (Apache-2.0), simdjson 4.2.2 (Apache-2.0 or MIT), fastgltf 0.9.0 (MIT), GLM 1.0.2 (MIT), spdlog 1.16.0 (MIT), stb (MIT or public domain), Dear ImGui 1.92.9 (MIT), ImPlot 1.0 (MIT), uni-algo 1.2.0 (MIT or Unlicense), EnTT 4.0.0 (MIT), SPIRV-Reflect (Apache-2.0), Monado (BSL-1.0), OpenSSL 3.6.0 `libcrypto.so` and `libssl.so` (Apache-2.0), curl 8.17.0 (curl), FreeType 2.14.1 (used under GPL-2.0-or-later), HarfBuzz 12.2.0 (MIT), Font Awesome 7 (OFL-1.1) | the `.apk` | GPL-3.0; each bundled component under its own licence |

## Stream from PC

The "Ghost VR Stream from PC" title, PPSA99492 (`data/kit/sfpc.zip` in the app, installed to
`/data/homebrew/PPSA99492` the first time it's used). Its source is `remote/client/` in this repository
with the patches in `remote/patches/wivrn/client.series` and `remote/patches/wivrn-client/`
(`remote/client/build_ps5.sh`).

| Component | License |
| --- | --- |
| [WiVRn](https://github.com/WiVRn/WiVRn) v26.9 headset app by Guillaume Meunier, Patrick Nicolas and WiVRn contributors, with the patches above, and what WiVRn's build bundles into it: Boost 1.89.0 (BSL-1.0), KTX-Software (Apache-2.0), simdjson 4.2.2 (Apache-2.0 or MIT), fastgltf 0.9.0 (MIT), GLM 1.0.2 (MIT), spdlog 1.16.0 (MIT), stb (MIT or public domain), Dear ImGui 1.92.9 (MIT), ImPlot 1.0 (MIT), uni-algo 1.2.0 (MIT or Unlicense), EnTT 4.0.0 (MIT), SPIRV-Reflect (Apache-2.0), OpenXR loader 1.1.58 (Apache-2.0), WebXR input profiles 1.0.20 (MIT), Font Awesome 7 (OFL-1.1), and WiVRn's `external/` as listed under Stream to headset | GPL-3.0; each bundled component under its own licence |
| PS5XR and Ghost VR's PS5 pieces (this repo: `pc/ps5xr/`, `core/`, `remote/client/`), static | GPL-3.0-or-later |
| The Videodec2 decoder (`remote/client/ps5/gv_vdec.cpp`), after [EVO Player](https://github.com/sainsaji/EVO-PLAYER-PS5) by sainsaji | GPL-3.0 |
| [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan) by mihawk-99: RADV from [Mesa](https://mesa3d.org), static | GPL-3.0; MIT (Mesa) |
| The [PS5_PayloadSDK](https://github.com/mihawk-99/PS5_PayloadSDK) platform layer by mihawk-99, after the [PS5 payload SDK](https://github.com/ps5-payload-dev/sdk) (John Törnblom), with LLVM's libc++ and libc++abi, static | GPL-3.0; Apache-2.0 WITH LLVM-exception |
| From the payload SDK's sysroot, static: curl (curl), libpsl (MIT), OpenSSL 3 `libssl` and `libcrypto` (Apache-2.0), HarfBuzz (MIT), FreeType (used under GPL-2.0-or-later), libpng 1.6.43 (libpng-2.0), bzip2 1.0.8 (bzip2-1.0.6), zstd (BSD-3-Clause), GNU libiconv (LGPL-2.1-or-later) | as listed |
| [Inter](https://github.com/rsms/inter) font by The Inter Project Authors, `share/fonts` | OFL-1.1 |
| `gv-sfpc-helper.elf` (this repo, `remote/client/ps5/gv_sfpc_helper.c`) | GPL-3.0 |
| prospero-win's `libc.prx` from Playden's runtime kit 0.21, `sce_module/libc.prx` | as in that kit |

## Quest games (`venture-0201-*.zip`)

Downloaded by the app the first time a Quest game needs it.

| Component | License |
| --- | --- |
| [Venture](https://github.com/Villagers654/Venture) 0.2.0.1 by Villagers654, its Windows package as released without its launcher (`venture.exe`, `WebView2Loader.dll`); `venture-runtime.exe` patched by `quest/venture_patch.py`. The components it bundles keep the licence files it ships, and its own `licenses/THIRD_PARTY_NOTICES.md` | GPL-3.0; each bundled component as in Venture's notices |
| `ghostvr_jitmap.dll` (this repo, `quest/jitmap/`), loaded by `venture-runtime.exe` | GPL-3.0-or-later |
| `java/ghostvr-quest.jar` (this repo, `quest/java/`): Venture's `ApkCache.java` changed to run its steps in-process, with Ghost VR's own classes | GPL-3.0 |

## Astro Bot's DualSense (`tgovr-mods-astrobot-dualsense.zip`)

The DualSense models are made from "[Playstation 5 Dualsense](https://sketchfab.com/models/878c1f882808477ab81c2fe86d5a3936)"
by AHarmlessPotato and "[DualSense](https://sketchfab.com/models/42a8cf0910324e93a7e3debeb19ce293)" by johnwilliam, both
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Changes: split into ASTRO BOT Rescue Mission's controller
parts, re-meshed, recoloured and placed on the game's joints. The tutorial videos are the game's own with the
controller in them redrawn as the DualSense; the app downloads the set only when you turn it on for your own copy
of the game. `CREDITS.txt` in the zip says the same.

## Installed with games

| Component | License |
| --- | --- |
| [OpenComposite](https://gitlab.com/znixian/OpenOVR) by ZNix and contributors, commit cff07db with `pc/opencomposite/patches/`; carries OpenVR's headers (BSD-3-Clause, Valve), inih (BSD-3-Clause), debugbreak (BSD-2-Clause), the OpenXR loader (Apache-2.0), jsoncpp (public domain or MIT), GLM (MIT) and Microsoft's `d3dx12.h` (MIT) | GPL-3.0 |
| `MFReadWrite.dll` (`pc/nullmedia/`), static [FFmpeg](https://ffmpeg.org) 7.1.1 configured by `pc/nullmedia/ffmpeg-configure.sh` (no GPL or non-free parts); rebuild it with your own FFmpeg with `pc/nullmedia/build.sh` | LGPL-2.1-or-later |

REFramework, UEVR, UEVR profiles, BepInEx and community VR mods are never included. The app downloads
them from their makers' own releases on your console when you install a game, and each game's page
names the mod and its maker.
