# Source audit: supplied archives

## Ghost-VR-main.zip
| Paths | Relevance | Decision |
|---|---|---|
| pc/ps5xr/runtime/src/psvr2.c, psvr2.h, vr_exports.h | Native Hmd2/VrTracker2, Sense, haptics, gaze, frame pacing, 7.61 tables | Selected, unmodified; adapter uses documented calls |
| pc/ps5xr/runtime/src/xr_vulkan.c | Native Vulkan linear RGBA8 triple-buffer composition; coherent mapped pointers, copy and fence | Adapted copy helper; use as allocation/synchronization reference |
| pc/ps5xr/runtime/src/xr_session.c | Frame timing and repeat on omitted images, pose submitted in tracker space | Runtime adapter follows frame lifecycle; no complete OpenXR import |
| pc/ps5xr/runtime/src/ps5xr_host.h | Hosted native Vulkan runtime, display and DualSense ownership | Reference for future integration |
| pc/ps5xr/runtime/src/xr_input.c | Action mappings/pose handling | Reference for Sense controls |
| pc/ps5xr/runtime/shaders/layer.comp | World-space compositor layers | Candidate for cinema and UI |
| pc/ps5xr/tools/make_title.py | Native VR title metadata (attribute3 VR support bit 0x400; VR-only bit 0x800) | Test separately; original dual-mode title metadata stays unchanged for now |
| core/gvr_stats.c,h | Runtime stats and diagnostics | Included, required dependency |
| ps4/agent, ps4/payload, core/fw/firmware.h | PS4 compatibility, firmware-specific support | Reference only; no injection or compatibility payload needed |
| remote/, pc/wine*, pc/vrtitle, quest/, engine/, app/ | Streaming, Windows/Quest compatibility, library and UI | Not needed for native rhythm game; do not import |

GhostVR claims PS5XR linear output on native PS5 RADV, using CPU mapping as GPU address. This is a driver-specific property, not generally true for Vulkan. Verify CHEncore's pinned RADV build on console. Native runtime has 90, 120 and half-rate 120 output modes. Its 7.61 tables are a source-supported starting point, not proof that CHEncore's app context has access to every call. The project readme separately says PC games are reported working on 9.00 and later; do not conflate PS4 compatibility with native PC title support on 7.61. Optional hand tracking has additional requirements; leave disabled. Gaze not wired.

## AstroBotVR2-main (1).zip
| Paths | Relevance |
|---|---|
| agent/vr2.c, vr2.h | Reference for tracking, reprojection, refresh modes and clean teardown in PS4 compatibility context |
| agent/pads.c, hand.c | Controller pose and hand tracking references |
| agent/mirror.c, agent/gen1.c | Mirror/legacy graphics references |
| common/firmware.h, docs/FIRMWARE-11.60.md | Firmware-specific checks, including 7.61 |
| docs/HOW-IT-WORKS.md | Documents 90 Hz or 60->120, display ownership, headset audio, pause on removal, FOV and IPD |
| payload/, agent/hooks.c, agent/ps5compat.c | PS4 process compatibility only; excluded from native CHEncore integration |

Both projects declare GPL-3 licensing (Ghost VR notices specify GPL-3.0-or-later); preserve notices and appropriate terms in any combined distribution. Only chosen native Ghost source is copied; AstroBotVR2 is a source reference. Unverified: 7.61 native title launch, HMD support with current headset firmware, audio routing, frame presentation and Vulkan integration, Sense feedback strength, 90/120 performance. No firmware changes recommended.
