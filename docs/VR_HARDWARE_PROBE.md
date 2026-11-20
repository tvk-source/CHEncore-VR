# Native PSVR2 hardware probe

This opt-in build is a **diagnostic, not a playable port**. It links the Ghost
VR native headset backend using the existing pinned PS5 SDK/RADV link recipe,
but exits before game assets, updater, game controller polling or Vulkan
initialization. It only attempts headset initialization when the system reports
7.61 (0x0761). The export fallback requires the known 7.61 table and executable
anchors; external export files are disabled in this build. No firmware updates,
credential changes, payload injection, gaze, haptics or hand tracking.

## Host tests and native build

Use the pinned dependencies in [BUILDING.md](../BUILDING.md), including the
**built** PS5_Vulkan archive and native tool, not merely cloned repositories.

```bash
git switch vr-development
git pull --ff-only origin vr-development
bash ps5/tools/test-vr-probe.sh
set -o pipefail
ENCORE_VR_PROBE=1 bash ps5/tools/build.sh 2>&1 | tee /tmp/encore-vr-probe-build.log
```

Build cache: `build/ps5-vr-probe`. Output: `dist-vr-probe/PPSA98743`.
The asset staging step selects the same CMake directory as the linker (including
its samples.txt and optional ui.txt). No normal game build is needed first.
The packaging step retains upstream open assets and licence notices; no
user-imported resources are needed to run this probe. The probe metadata uses
Ghost VR's VR app/VR-only bits, without the hand tracking flag; this metadata
combination supported native output initialization on 7.61; visible output is confirmed; binocular fusion remains unverified. The same title ID as the development
game is intentional, so do not mount both builds simultaneously. The standard
build (ENCORE_VR_PROBE=0, default) uses `build/ps5` and `dist/PPSA98743`.

## Test on the console

Only after successful cross-compilation and native linking: close other VR
titles, check PPSA98743 is unused or belongs to this fork, back up any existing
PPSA98743 installation, and mount the result using your existing homebrew
launcher from a separate staging folder. Do not overwrite GhostVR or upstream
Encore. Sit down, put the headset on before opening the probe, and launch.
TV mirroring is intentionally disabled for this initial test.

Expected (v4): a grey cross and ring on a common plane at nominal two metres,
projected separately into each asymmetric eye texture, assuming parallel eye
axes. Up to 2700 frames at nominal 90 Hz (about 30 seconds).
Tracking gets up to three seconds to settle; the loop stops at 34 seconds
or first failure, wear removal or tracking loss. SDK calls and display initialization may take additional time;
the loop bound is not a watchdog against a blocked firmware call. Close the
title if uncomfortable or stuck. Do not retry automatically. The headset
backend attempts teardown, reports the result and then idles without submitting
frames or falling through to gameplay. Close it explicitly via the PS menu.
Do not launch another VR application until this title has been closed. If the
shell fails to recover, stop testing and use the usual console recovery procedure.

Collect klog from launch to exit, including `VR_PROBE` and `[ps5xr]` lines.
A SUBMISSIONS_PASS only means all required frame submissions succeeded; both
eyes showing the pattern and successful shell recovery must be reported
separately. Inspect output stop and shutdown return codes even on PASS.
Report any native SDK import/link errors with the complete build log. The
host tests cannot establish whether the console API imports or ABI work.

Next gate: a Vulkan-generated pattern, with coherent mapped triple buffers
and fenced GPU writes; then cinema, stereo gameplay and instruments.

## First native build evidence
On 2026-11-20 the user's native SDK compiled all 49 C/C++ objects, RADV release
was built at 1ba9874f4786, and the PS5 native linker and SELF signer generated
an eboot.bin (28,491,401 bytes) and engine PRX. Final asset staging failed
because it used build/ps5 instead of build/ps5-vr-probe. The build-directory
fix addresses that packaging failure; rerun the build to verify. No PS5
hardware result has yet been reported.

## First console launch, 2026-11-20
Both launches reached main and klog output, then returned code 2 because
sceKernelGetSystemSwVersion reported 0x1079. PS5 7.61 has a PS4 compatibility
version of 10.790.001, as documented at https://www.psdevwiki.com/ps5/Build_Strings .
The probe now queries sceKernelGetProsperoSystemSwVersion for the strict 7.61
check, following etaHEN's native version query, and logs both raw values.
The compatibility value is diagnostic only and cannot authorize headset access.
The mock test reproduces the 0x1079 compatibility result and checks that 7.60
and 13.60 PS5 versions remain rejected. The corrected native build and headset
initialization still need console verification.

## Second hardware launch, 2026-11-20
Two runs of e87f3f5 detected native 7.61, matched the Hmd2 and VrTracker2 7.61
anchors, initialized both Sense handles and native reprojection, registered
three 2048x1024 buffers, and submitted 180/180 frames in 1.98 seconds (91.0 and
91.1 submissions/s, zero queue-full waits or submission errors). Both runs
returned SUBMISSIONS_PASS. Display output was configured at 89.91 Hz. Visual
confirmation has not been provided. The unregister call returned VO_BUSY
(0x80290009), but close, disable, tracker finalize, reprojection terminate and
Hmd2 close/terminate returned zero. Do not infer that buffer ownership is
fully verified; keep the immutable probe allocations alive until closure.
After completion LoadExec("exit") caused an attempted invalid ELF spawn
(0x80aa001a). The new diagnostic uses manual PS-menu closure instead, and
provides a larger static test pattern with a longer bounded submission period.
No GhostVR-specific payload dependency has been established or ruled out by
this run; the exact set of active background payloads was not identified.

## V4 optical test
Two v3 runs submitted 540/540 frames in 5.98 seconds (90.3 submissions/s),
with no submission errors or queue-full waits. Both reached the idle state.
The tester saw two images, and closing the right eye left one image visible.
The current hypothesis is incorrect binocular alignment rather than incorrect
routing. V4 uses measured asymmetric FOV tangents and half-IPD translations,
with expected horizontal center UVs 0.66567 and 0.33433 at 59 mm IPD.
This static parallel-eye approximation does not establish the actual eye-pose
conventions or world-locked tracking. Sit still and stop immediately if the
images are uncomfortable; do not force fusion. Removing the headset stops
the frame loop. After the test close the idle title through the PS menu.
No changes to firmware, headset initialization, buffer registration or
submission ABI. VO_BUSY on unregister remains unresolved, so no buffers are
rewritten or reused for another session before title closure.

## V4 hardware validation and V5 depth test (2026-11-20)
V4 (956dd44) passed all six host checks and the native build. Clang emitted
18 redundant generic-association warnings from two SDK isfinite macro calls
in probe_stereo.h. V5 replaces these with bounded positive comparisons,
which reject NaN, infinity and out-of-range values without suppressing warnings.
On native firmware 7.61 V4 submitted 2700/2700 frames in 30.01 s (90.0 fps),
zero queue-full waits, repeats or submission errors in the reported intervals.
The tester confirmed one fused centered image, stable head-relative movement,
and expected black output after the test. The log reached SUBMISSIONS_PASS
and idle. VO_BUSY during unregister remains unresolved; other shutdown
return codes were zero. CRASH KILL_END at closure is not a clean-exit proof.
V5 keeps the verified optical reference at two metres and adds a square on
the left at one metre and a diamond on the right at four metres. All three
are head-relative and all three registered buffers remain immutable.
Pose telemetry at frame 1 and every 450 frames records measured eye baseline,
head-local offsets, eye quaternion agreement and device pose for future
world-space rendering. It is not used to modify the framebuffer or submission.
The 30-second limit and existing wear/tracking stop checks remain. Stop
immediately on discomfort or double vision. After the test close the idle
title through the PS menu. V5 is a depth/pose diagnostic, not a world-locked
scene or a Vulkan renderer. No GhostVR payload independence claim.
