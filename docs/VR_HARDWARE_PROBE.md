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
combination is **not verified on 7.61**. The same title ID as the development
game is intentional, so do not mount both builds simultaneously. The standard
build (ENCORE_VR_PROBE=0, default) uses `build/ps5` and `dist/PPSA98743`.

## Test on the console

Only after successful cross-compilation and native linking: close other VR
titles, check PPSA98743 is unused or belongs to this fork, back up any existing
PPSA98743 installation, and mount the result using your existing homebrew
launcher from a separate staging folder. Do not overwrite GhostVR or upstream
Encore. Sit down, put the headset on before opening the probe, and launch.
TV mirroring is intentionally disabled for this initial test.

Expected: a steady small grey cross in both eyes, on dark grey, for at most
180 submitted frames at nominal 90 Hz. Tracking gets up to three seconds to
settle; the loop stops at eight seconds or first failure, wear removal or
tracking loss. SDK calls and display initialization may take additional time;
the loop bound is not a watchdog against a blocked firmware call. Close the
title if uncomfortable or stuck. Do not retry automatically. The headset
backend attempts teardown and asks the shell to exit, without falling through
to the game. If the shell fails to recover, stop testing and use the usual
console recovery procedure.

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
