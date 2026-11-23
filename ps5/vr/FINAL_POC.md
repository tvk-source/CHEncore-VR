# CH: Encore VR - Final proof-of-concept test (Alpha 7)

This is a **bounded final POC build**, not a complete production-quality native VR port. The existing working Sony headset firmware and Ghost VR output path are unchanged.

## What the final build tests
- The original PC atlas artwork, with note caps scaled to recover their approximate round silhouette.
- Original two-frame animated notes, lane-tint and tap sheen.
- Source-authored sustain trails and guitar hit effects (including their original atlases and colors) rendered into each HMD eye.
- A full-length world-anchored stereo highway with near/far depth and perspective.
- Original **live** score, power/health and song-widget pixels sampled into isolated side/top VR HUD panels. The HUD shader masks out dark source background to avoid obvious black rectangular UI panels.
- Existing Sense controller mapping and actual chart judgment reporting.
- The regular 2D cinema remains for menus, pause and results, with no duplicate black track during five-fret gameplay.

## Build, stage and verify
```bash
cd "$HOME/Downloads/CHEncore-VR-ghost-integration/CHEncore-VR" || exit 1
git switch vr-development
git pull --ff-only origin vr-development
bash ps5/tools/build-cinema-alpha.sh
python3 ps5/tools/verify-cinema-deploy.py --host 192.168.86.25
```

Deploy the **entire** `dist-vr-cinema/PPSA98743/` directory. FTP hashing only works for an unpacked homebrew-folder deployment.

## Klog for one headset test
```bash
nc 192.168.86.25 3232 | tee "$HOME/encore-final-poc-klog.log" | \
    grep --line-buffered -Ei 'VR_BUILD|VR_STEREO|VR_JUDGE|VR_SENSE|VR_CINEMA|gvr: stats|GPU_FAULT|CRASH'
```

Expected strings: `VR_BUILD_FINAL_POC_STEREO`, `VR_STEREO mode=SOURCE_ATLAS hud=KEYED_SIDE_AND_TOP ... sustain=... fx=...`. A change in `VR_JUDGE hits` proves a successful input judgment; `VR_SENSE mapped_frets` alone only proves button mapping.

## What remains imperfect
- This is not a full 3D world/stage. The original game art is projected onto a world-space highway, with stereo eyes and head motion.
- Animated note caps can still look different from the original PC perspective and lighting; the original track sorting, all specialized additive blending and transparent-particle effects aren't fully reconstructed.
- HUD panels are cropped from the original game's live 1440p framebuffer, so fine print and some side widgets will still be resolution-limited; dark original art is keyed out by design. HUD is not independent vector text.
- Drums, six-fret, multiplayer, controller haptics, and full 3D lyrics/menus are not part of this stereo port. They stay in cinema mode where applicable.
- No native PS5 cross-build or headset test can be executed by the GitHub edits themselves. The repository includes a host CI workflow for the pure CPU tests and GLSL shader compiler, but full PS5 device validation must be performed on your console.

This is intended as a single final validation build so the POC can be evaluated without committing to another lengthy development cycle.
