# Encore VR Alpha 6: source atlas gameplay + original HUD panels

## What is different from Alpha 5

- Keeps the original PC atlas highway, notes, receptors, and scrolling board as separate world-space stereo projections. No restored Alpha 4 black-board overlay.
- Expands the near-to-far highway's vertical rise from 0.59 m to 1.37 m and its depth spread from 0.68 m to 1.12 m, so the original far end reaches the upper view instead of stopping half-way up.
- Draws only two cropped sections from the game-owned 2560x1440 source frame on stereo-projected panels alongside the board. They preserve the original game-rendered health/star-power meter, score/star/progress sprites and live text. No second flat gameplay screen.
- The source screenshot is **not** published to GitHub. It stays in the user's on-device game framebuffer.
- Menus and paused gameplay remain on the original flat cinema, so navigation retains its proven behavior.
- Prints `VR_BUILD_ALPHA6_STEREO_HUD source=<git revision>` on startup and `VR_STEREO mode=SOURCE_ATLAS hud=PC_SIDE_PANELS` when playing.
- Retains `VR_SENSE mapped_frets` bridge logging for diagnosing button mapping.

## Build and test

```bash
git switch vr-development
git pull --ff-only origin vr-development
bash ps5/tools/build-cinema-alpha.sh
python3 ps5/tools/verify-cinema-deploy.py --host 192.168.86.25
```

Deploy all of `dist-vr-cinema/PPSA98743/` to the test PS5, including `eboot.bin`, `sce_module/encore-engine.prx`, shaders and personal asset pack. The FTP verification command works only with the standard homebrew folder deployment; it is a read-only hash check.

## Acceptance checklist

1. During five-fret gameplay, the board continues up toward the distant top instead of ending at mid-height.
2. The left panel shows original health and star-power UI; the right panel shows the original score/star/progress UI, dynamically changing during the chart.
3. The original flat gameplay screen does not appear under the stereo track.
4. The Score/HUD panels have sensible scale and remain legible after recentering.
5. No GPU faults or reprojection stalls. Menus and pauses still work.
6. Sense inputs still register; the presence of `VR_SENSE` mappings alone does **not** establish successful gameplay judgments.

## Known limitations

- The peripheral HUD uses the existing flat game's live pixels rather than reimplementing every original HUD prefab as independent stereo sprites. The two panels are deliberately flat surfaces, which is appropriate for a VR HUD.
- Only the peripheral 29% of the original screen is used for each panel. Central lyrics, countdowns and optional player labels are not yet independently migrated; neither are all sustain/hit-effect material passes.
- Some dark background pixels from those peripheral regions may appear in the panels. Crop and position should be tuned from an actual in-headset screenshot.
- The color-tint atlas renderer does not yet reproduce every specialized original PC additive/meter shader effect, so the HUD panels deliberately take their animated output from the already composited 2D frame.
- The native PS5 build and in-headset placement must be tested on the physical console; GitHub commits alone cannot validate the presentation.
