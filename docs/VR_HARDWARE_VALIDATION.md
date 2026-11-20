# Native PSVR2 hardware validation

Target: a native CHEncore-VR application on PS5 firmware 7.61, experimental title ID PPSA98743. Keep the original game and GhostVR installation separate. Do not update either firmware for this test.

## Gates (not yet completed)

1. Build a dedicated opt-in native probe against the pinned PS5 SDK and RADV dependencies, with GhostVR's headset backend linked. Normal TV builds must remain unchanged.
2. Verify headset readiness, FOV, IPD, frame timing and teardown. Use a steady low-brightness test image for a bounded number of frames, stop on tracking loss or headset removal, and collect klog. Do not retry automatically on failure. A successful submission alone does not prove a visible image; report both eyes and shell recovery.
3. Verify Vulkan-generated images in coherent mapped triple buffers, GPU fence completion, compositor buffer ownership, and native reprojection.
4. Connect the game render path (cinema first, then true stereo). Update scoring and audio timing only once per frame. Test physical instruments and the existing flat renderer for regressions.
5. Add Sense tracking, haptics, virtual drums, air guitar and stage features; verify each independently on hardware.

## Current evidence

The foundation tests and mock-hardware adapter passed on the user's Linux machine. Native PS5 compilation, headset output and all later gates are still unverified. See [PSVR2_PORT.md](PSVR2_PORT.md) for integration details and [BUILDING.md](../BUILDING.md) for pinned build dependencies. The optional ENCORE_VR_PROBE build and mock tests are now implemented; see [VR_HARDWARE_PROBE.md](VR_HARDWARE_PROBE.md). Cross-compilation and hardware validation remain outstanding.
