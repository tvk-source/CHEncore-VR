// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_probe.hpp"
#include "probe_stereo.h"
#include "probe_vulkan.h"
#include "ghost/psvr2.h"
#include "../src/platform.h"
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cmath>

// PS5-specific version query, not the PS4 compatibility version.
// Match the layout used by etaHEN's native firmware detection.
struct ProbeSoftwareVersion { uint64_t size; char text[0x1c]; uint32_t version; uint64_t reserved; };
static_assert(offsetof(ProbeSoftwareVersion, version) == 0x24);
extern "C" int sceKernelGetProsperoSystemSwVersion(ProbeSoftwareVersion*);
extern "C" int sceKernelGetSystemSwVersion(ProbeSoftwareVersion*);
namespace {
constexpr int Frames = ENCORE_PROBE_FRAMES;
int finish(int status) {
    say("VR_PROBE result=%s code=%d", status == 0 ? "SUBMISSIONS_PASS" : "FAIL_OR_INCOMPLETE", status);
    std::fflush(stderr);
    // Native main owns process lifetime; LoadExec("exit") attempted an invalid spawn on 7.61.
    return status;
}
}
int encore_vr_native_probe() {
    say("VR_PROBE begin: PPSA98743, native headset, offscreen Vulkan raster, no game assets");
    ProbeSoftwareVersion version{};
    version.size = sizeof(version);
    const int rc = sceKernelGetProsperoSystemSwVersion(&version);
    const unsigned fw = version.version >> 16;
    ProbeSoftwareVersion compat{};
    compat.size = sizeof(compat);
    const int compatRc = sceKernelGetSystemSwVersion(&compat);
    say("VR_PROBE PS5 firmware rc=0x%08x raw=0x%08x version=0x%04x", unsigned(rc), unsigned(version.version), fw);
    say("VR_PROBE PS4 compatibility rc=0x%08x raw=0x%08x", unsigned(compatRc), unsigned(compat.version));
    if (rc != 0 || fw != 0x0761u) {
        say("VR_PROBE stopped: only firmware 7.61 is enabled");
        return finish(2);
    }
    // RADV must initialize AGC before headset reprojection, as in hosted GhostVR.
    if (encore_probe_gpu_init() != 0) {
        encore_probe_gpu_stop();
        return finish(5);
    }
    // Isolate Hmd2 output from TV mirroring and DualSense ownership.
    psvr2_set_dualsense(0);
    psvr2_set_mirror(0);
    if (psvr2_init_output() != 0) {
        say("VR_PROBE initialization failed");
        psvr2_shutdown();
        encore_probe_gpu_stop();
        return finish(3);
    }
    psvr2_info info{};
    psvr2_get_info(&info);
    say("VR_PROBE status=%d worn=%d panel=%ux%u ipd=%.4f fov=%.4f %.4f %.4f %.4f",
        info.status, info.worn, info.panel_w, info.panel_h, double(info.ipd),
        double(info.fov[0]), double(info.fov[1]), double(info.fov[2]), double(info.fov[3]));
    bool validFov = true;
    for (float t : info.fov) validFov = validFov && std::isfinite(t) && t > 0.0f;
    int submitted = -1;
    if (info.status == 0 && info.panel_w && info.panel_h && validFov &&
        std::isfinite(info.ipd) && info.ipd > 0.0f && psvr2_worn())
        submitted = psvr2_selftest("encore-native-761-v9-highway", Frames);
    else
        say("VR_PROBE skipped: headset not ready, not worn, or invalid optical data");
    say("VR_PROBE submissions=%d requested=%d", submitted, Frames);
    psvr2_shutdown();
    say("VR_PROBE shutdown returned; inspect teardown codes above");
    encore_probe_gpu_stop();
    return finish(submitted == Frames ? 0 : 4);
}
