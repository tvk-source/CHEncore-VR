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

## V5 confirmed; V6 offscreen Vulkan bridge
On 2026-11-20, V5 (ff8224b) delivered 2700/2700 frames in 30.00 seconds
at 90.0 submissions/s. Errors and queue-full waits zero. The tester confirmed
all three objects fused, correct depth order and stable head-relative motion.
Every pose sample reported 59 mm baseline, head-local eye offsets
(-0.0295,0.001,0.012) and (0.0295,0.001,0.012) m, eye quaternion absdot 1.
The frame-1 head pose was (-0.0808,-2.1355,-0.0023) m, while frame 450
was (0.0158,1.1644,-0.0480) m. System tracking stable at 1.09 s.
Never anchor a world to the first returned pose. Future dynamic rendering
needs stable pose gating, recentering and buffer ownership verification.
V6 initializes linked RADV without surface or swapchain before Hmd2, records
a render pass for each 1024x1024 eye and GPU color clears covering the V5
pattern. It copies both images to an independent coherent readback buffer
using vulkan_linear.hpp, waits on a bounded fence and checks all pixels
against the CPU reference. Only on exact agreement is that image copied
to the three existing immutable direct-memory buffers before registration.
This is a GPU render/copy/readback bridge, not a shader-driven scene or a
zero-copy or per-frame renderer. No buffer rewriting after registration,
no move to 120 Hz, no world-lock or credentials changes. GPU resources
are separate from display buffers; stop after headset teardown. A failed
fence leaves GPU resources allocated until title closure. No CPU fallback
so a green submission result cannot disguise a failed Vulkan test.
V6 uses the same shapes, optical geometry and 30-second wear-gated test.
If GPU validation fails, capture the error and close the title. No output
should be registered. VO_BUSY at unregister remains unresolved.
Host tests check the raster coverage plus native init order in mocks;
native build and physical GPU execution remain to be verified.

## V6 hardware pass and V7 concurrent GPU/flip telemetry (2026-11-20)
V6 f3107f9 selected native PlayStation 5 GPU (RADV NAVI21), graphics family 0.
698 raster runs per eye and 2097152 readback pixels, zero mismatches,
7156/7076 lit pixels. 2700/2700 headset frames in 30.01 seconds, 90 FPS,
queue-full waits and errors zero. GPU shutdown complete. V6 log alone
does not establish a new visual observation. PCI UUID warning nonfatal.
Startup pose jumped again; system tracking stable at 1.246 seconds.
V7 replays the verified offscreen command buffer serially each frame
after three seconds, poisoning private readback before raster/copy and
checking six predetermined samples after a bounded 100ms fence. This measures
ongoing GPU work alongside Hmd2; no registered display buffer is rewritten,
and the visual reference remains unchanged. Timing is CPU wall time around
submit/fence/sample, not a GPU timestamp. No implication of sustained
shader-based gameplay or zero-copy. A failed replay stops the diagnostic;
unconfirmed GPU resources survive until title closure.
V7 records read-only sceVideoOutGetFlipStatus samples, interpreting word
3 tentatively as shown flip marker based on pinned PS5_Vulkan
driver/ps5vk_private.h and driver/ps5vk_wsi.c (7f492b4c). This is not proof
of VR compositor retirement. No reuse decision depends on it. The origin
candidate waits >=3s and 45 consecutive low-delta valid pose samples;
this is a heuristic, not a runtime SLAM-quality flag; no world anchoring
yet. Look forward and stay still during the first four seconds. The image
should remain as V6. VO_BUSY and manual closure remain unchanged.

## V8 integrated batch (2026-11-20)
V7 passed twice: 2700/2700 submissions each, 2428 GPU replays each,
mean 4.289/4.277 ms, max 4.923/4.927 ms, zero over 11ms, zero errors.
Both runs acquired origin at frame 317 (~3.5s); 2699 valid flip samples,
zero invalid/regressed or observed lag. VO_BUSY remains; flip status is
not sufficient proof of Hmd2 retirement. V8 does not rewrite any registered buffers.
Run bash ps5/tools/build-vr-batch.sh to pull, test and build once. Deploy
both executable and engine PRX using existing method. A single 30s nominal
run tests 900 submissions at 90Hz (gray), 1200 at 120Hz (green), and
600 at 60-to-120 (blue). A failed mode transition stops; no fallback that
hides a failure. The 45s overall frame-loop deadline includes transitions.
Mode reports are configured targets, measured port vblank count and
measured submit rate; not proof of generated reprojection frames.
After origin acquisition, the alternate modes exercise actual tracked
world point projection using the two measured eye poses and private
Vulkan targets, with fence and GPU pixel checks. The visible three shapes
are still head-relative; tint changes distinguish actual immutable
buffer selections. No claim of displayed world lock. No live reuse.
Sense polling logs connected/tracked counts, button mask and trigger
samples. Press each trigger once after four seconds for one low-amplitude
80ms controller pulse per side; user confirmation required, requests
are not proof of haptics. Stop on discomfort or loss of tracking.
No gaze, adaptive triggers, hand tracking, firmware or payload changes.
Return combined log plus perceived gray/green/blue phase changes and
whether each pulse was felt. Tests include world projection and phase boundaries.

## V9 input-driven highway preview (2026-11-20)
V8 passed all modes visually and via native calls, 900/1200/600 submissions
at 90.09/120.03/60.13 FPS, 1800 dynamic private world renders, zero
errors or budget overruns. At half rate the port vblank runs at 60Hz;
this is not independent proof of 120 distinct display refreshes.
Both Sense controllers connected and tracked 2700/2700 samples, trigger
samples L185/R133; button masks 0xb3/0x8f. Vibration failed because the
module lookup found no libScePad handle. V9 attempts the existing native
sandbox module path and exact module basenames, tries names/NIDs only,
and refuses unverified offsets in the native probe. Mode call failure
prevents intensity call; logs API success separately from user perception.
Four immutable 1024x1024-per-eye highway preview variants replace the
visible reference pattern: neutral, left target white, right target white,
both white. This is a CPU-generated mock-up with stationary notes, not
CHEncore gameplay or GPU rendered live presentation. The GPU continues
the private Vulkan tests from V8. Three refresh modes still run for
~10s each, with logs rather than tint as phase markers. After four
seconds, squeeze L2, R2, then both, hold about a second each; note white
target highlights and any one-time 100ms controller pulses. No firmware,
credential, payload or live registered memory change. V8's VO_BUSY
remains unresolved and must not be treated as successful retirement.

## V9 hardware results and V10 private rhythm foundation (2026-11-20)
V9 user confirmed white target feedback during final 60fps phase.
2700/2700 submitted, zero queue-full/errors, four input states
2535 neutral/19 left/79 right/67 both. Both controllers 2700 connected
and tracked samples. 90/120/60 submissions 90.09/120.03/60.13fps.
Pad sandbox load succeeded (handle 0x72); both named/NID lookups
failed 0x80020003. Haptics remain unavailable; no unverified offsets.
Unregister still VO_BUSY. Flip status is not a compositor release fence.
V10 adds a host-testable 5-lane monotonic rhythm clock (±120ms inclusive,
rising edges, missed beats), real Sense lane mapping (L2/square/triangle/
cross/R2), world anchored target/note positions and controller markers,
up to 12 per eye in private Vulkan images. Private marker readback is
checked; no live registered memory writes, no audio, no real game
chart, no visible moving notes yet. The diagnostic counters are NOT a
score for the user because the notes are not visible. Existing visible
V9 immutable targets remain unchanged. Do not request repeated
hardware tests until a meaningful presentation milestone is ready.
Host rhythm tests passed GCC and Clang; native integration still needs
build and physical verification. No change to firmware or credentials.
