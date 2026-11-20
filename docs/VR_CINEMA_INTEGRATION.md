# Original renderer integration: cinema first

V12 proved CHEncore's original generic note evaluator alongside the
experimental stereo presentation path, not the full game renderer.
Use the existing game renderer and audio/song loader as the only source
of truth; do not duplicate its full chart parser or game rules.

## Host-side groundwork

`ps5/vr/cinema_geometry.hpp` generates a world-fixed, yaw-aligned
aspect-preserving screen with per-eye homogeneous clip vertices and
source-image UVs. Default proposal: 2.4m wide, 16:9, 2.5m distance.
These are adjustable design defaults, not validated comfort limits.
No runtime call site yet. The unit test exercises orientation, stereo
projection, motion parallax, horizontal screen alignment and invalid poses.

## Remaining integration

- `examples/cloneheronative/cloneheronative.cpp` loadChart (line 6816
  at V12) already handles chart metadata, tempo, notes, modifiers, audio,
  instrument type, and difficulty. Preserve it rather than parse again.
- render (line 8373) updates gameplay once, prepares UI, writes vertex
  and uniform buffers for currentBuffer, records commands, submits,
  then signals songStartGate. A cinema path needs one game update, not
  independent game simulations for each eye.
- Introduce a separate offscreen game target with color-attachment and
  sampled usage, its own framebuffer and extent, compatible render pass
  and depth buffer. Retain the existing normal TV path unchanged.
- Record original game/UI into the game target once. Synchronize the
  image transition and dependency to fragment sampling; draw the same
  texture to each eye using cinema geometry, and keep private GPU
  work fenced. Explicitly validate renderer and texture orientation
  in the existing host reference build before a native test.
- Adapt the existing game input abstraction to Sense menu/controls;
  use original game audio clock and calibration, not the practice clock.
- Preserve the V12 rollback package. Reuse its experimental bounded
  presentation only after an independent compositor lifetime review.
  Flip age is not a release fence; VO_BUSY remains unresolved.

First combined hardware milestone should show the original menu and a
locally supplied song in cinema mode. Full immersive geometry can then
reuse game state from a single simulation step. The host geometry helper
alone is not a playable cinema build, and no native implementation or
console validation is claimed here.

## V12 visual/audio validation and source-copy preparation

Tester confirmed audible audio, moving notes and stable targets. Do not
repeat V12. Audio latency and exact world-lock accuracy are unmeasured.

The existing `VulkanExampleBase::submitFrame` renders once, then optionally
captures before queue presentation. `ps5CaptureSwapchainImage` already
copies the acquired game color image, but waits the whole queue idle and
converts it on the CPU for screenshots. Do not use that slow screenshot
routine as a per-frame cinema path. Instead, the new
`ps5/vr/cinema_source.hpp` records a same-size/same-format image copy
and transitions the independent destination texture for fragment sampling.
It takes explicit layouts, requires prior source completion and destination
retirement, restores source layout and never touches headset slots.
`ps5/vr/shaders/cinema.vert` and `cinema.frag` consume the homogeneous
clip vertices from cinema_geometry.hpp and sample that same game texture
for each eye. Preserve clip w, use positive-height Vulkan viewport and
matching color formats/color-space handling; no extra vertical flip.

These are reusable source-copy and shader building blocks, not a wired
renderer. No Vulkan compilation, shader compilation, native build, or
GPU execution has been performed for them. The next integration must
own texture allocation, descriptors, pipeline/render pass, command/fence
lifetime, and explicit image usage validation before calling the helper.
A headless game render target is still preferable for the production VR
path; a capture hook on the existing renderer is useful for host proof.
Keep normal TV presentation unchanged. No hardware build requested yet.

## Integrated host cinema reference

The new host-only `cinema_reference.cpp` is called from the original
renderer screenshot hook in `ps5/src/example_ps5.cpp`. In reference mode
only, force existing host offscreen swapchain (TRANSFER_SRC usage).
After the game frame has finished, allocate a sampled copy of the actual
color image, a private RGBA8 stereo target, upload per-eye screen vertices,
create the shader pipeline and descriptors, draw one textured quad for each
eye, wait on a fence and export 2048x1024 side-by-side PPM/PNG. The same
source frame is used for both eyes. This is intentionally a once-per-run
reference, not a per-frame production design. No Hmd2, live VR buffer,
or native build behavior changes. The source is restored before presentation.
The output raster non-background check is a smoke test, not a proof of exact
projection or pixel-perfect sampling. Inspect stereo output against the
ordinary screenshot. `bash ps5/tools/cinema-reference.sh` compiles the new
shaders and drives the existing host reference build; reuses assets from
normal dist or dist-vr-probe. The script reports failure if no cinema PASS
or any cinema FAIL is logged. Build and GPU run remain unverified until
executed on a machine with repo dependencies. No console test requested.

## Linux reference compilation fix
The initial cinema reference build compiled both shaders, but failed in
cloneheronative.cpp: shared gameplay/UI paths referred to songAudio and
AudioStemMixer::UiSound although PS5_HOST_REFERENCE excluded them. The
silent HostReferenceAudio adapter reuses the existing cue enum and preserves
shared gameplay side effects without linking the native mixer. Guard native
startup audio and native audio QA actions; finish the intro in the silent
host reference so menu widgets can appear. Native PS5 audio remains unchanged.
Added an isolated adapter linkage test. Full host build and run still need
verification; successful shader compilation is not a successful game build.

The df90041 host audio fix compiled successfully on the tester's machine.
The only reported remaining link error was Downloader::coverPath(size_t).
Move the existing pure accessor out of the native-only conditional so
both builds share exactly the same implementation. Native download and
cover-fetch behavior are unchanged; the host browser remains offline.
Full host link and GPU reference execution still need verification.

The 2fc9dfd host executable compiled and linked on the tester's system.
The test-run resolver incorrectly looked only in build/ps5/samples.txt,
and mapfile masked its failure. Use build/ps5-host/samples.txt explicitly,
check resolver exit status, preserve the process exit status and require
both reference images before success. No native rebuild required.
Note that the original game requires the user's own EncoreAssets-v1.encore-assets
pack, or an already installed host PC asset root. The distributed probe
assets do not necessarily contain it. A setup screen is not the actual
CHEncore menu/gameplay reference.

## First successful host GPU composition (2026-11-20)
User's Linux reference run at 5c784a5 built and linked, selected
cloneheronative, captured a 3840x2160 original frame and produced
2048x1024 stereo output (73424 non-background pixels per eye). Both
images were inspected: the source was the PC asset import UI, and both
eye images showed the same upright UI correctly. This verifies actual
Vulkan composition, not the full game menu, live world lock, or native
cinema performance. V12 remains the playable practice baseline.
The new automated reference path uses existing validated asset pack import
when ENCORE_ASSET_PACK points to a personal pack, or finds one in the host
app directory/assets. Existing installed packs are reused. Missing/invalid
assets fail before rendering instead of passing on the setup UI. No native
asset behavior change. Host import goes into build/ps5-host/app/userdata/pc-assets
and skips native persistent storage migration in asset setup. The pack is
not committed or redistributed. To run with an authorized pack:

    ENCORE_ASSET_PACK="$HOME/Downloads/EncoreAssets-v1.encore-assets" bash ps5/tools/cinema-reference.sh

The host reference fix is untested. Native cinema compositor integration,
real menu input, and full song play remain outstanding.
