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
