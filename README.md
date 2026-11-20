# CH: Encore — PS5

Huge thanks to my cousin, [Eddie Rubio](https://music.youtube.com/channel/UCmQMUczMFC3UQH1eaTNe1Bw), for the awesome main menu riff.

<p align="center"><img src="ps5/assets/ch-branding/icon0.png" alt="CH: Encore" width="180"></p>

A native PS5 rhythm game with guitar/bass, drums and local multiplayer.

**v1.5.2 — Standalone Windows builder** includes Python and its packaging tools in one executable. The public download contains an asset-free game template and a desktop builder. Your own authorized PC resources are added locally to a folder install, exFAT image or native homebrew PKG. No original PC resources or songs are supplied by this project.

## Install on PS5

**[Download the Windows Install Builder (.exe)](https://github.com/MyNameIsAwoo/CHEncore-PS5/releases/download/v1.5.2/CH_Encore_1.5.2_Install_Builder.exe)** · [Release notes](https://github.com/MyNameIsAwoo/CHEncore-PS5/releases/tag/v1.5.2) · [Detailed setup guide](docs/ASSET_SETUP.md)

**One executable. No ZIP extraction or Python installation.** It includes the asset-free game template and everything needed to create your personal install on Windows x64. Your PC assets are read locally and are never supplied by this project.

1. Download **CH_Encore_1.5.2_Install_Builder.exe** and double-click it.
2. Select a detected PC game installation, or choose **Browse** for a custom location. Select the installed game folder or its `*_Data` folder. Only the resource version supported by the importer is accepted.
3. Choose an output folder and **Game folder**, **exFAT image**, or **Native homebrew PKG / fPKG**. Confirm permission to use your local resources, then select **Build install**.
4. Install the resulting `PPSA98743` folder, `.exfat` image or `.pkg` using your compatible PS5 homebrew setup. Keep the complete title folder intact. **First launch prepares your included assets automatically**; no separate console asset transfer or import selection is needed.

Folder and exFAT builds work offline once the executable is downloaded. PKG creation automatically downloads and verifies the proven native packaging tool on first use, then reuses its cache. It does not install another desktop app or require administrator access. Native PKGs are homebrew/debug packages, not retail-signed applications; the native packaging path was checked on a PS5 Pro with a bounded 240-frame launch; other setups still need testing.

**Songs are separate.** No songs are included or imported from the PC installation. Add authorized charts through Encore Browser, `/data/homebrew/SONGS`, or a root-level `SONGS` folder on an exFAT drive. Nested collections are supported.

For Linux/macOS and developers, the Python source builder remains available in `tools/install-builder`; see the setup guide. Windows source launchers explicitly use Python 3.12 x64 to avoid unsupported default interpreters.

Read [Asset setup](docs/ASSET_SETUP.md) for requirements and supported platforms. Generated personal installs contain your imported resources and must stay private. Only the unmodified public builder is shareable. No permission to extract/reuse the original resources or developer approval is claimed; see [rights and distribution](docs/RIGHTS_AND_DISTRIBUTION.md).

Already imported assets, songs, profiles and scores remain in persistent storage, outside code/runtime updates. Existing installations can continue using normal in-game updates without rebuilding.

I wish I could supply a fully built game, but have been asked not to include any assets in the project. Good news, after initial install updates are automatic in-game!

## Features

- Five-fret guitar/bass, six-fret, drums, Pro Drums and five-lane layouts.
- Up to four local players, profiles, per-instrument remapping and controller navigation.
- Encore Browser for chart search/downloads; USB/internal song scanning, copying and confirmed removal.
- Local records, lyrics, solos, pause/restart, previews and highway/color options.
- Display-aware resolution and supported 60/120 FPS modes.
- [Per-player, per-instrument timing calibration](docs/CONTROLLER_CALIBRATION.md), plus MIDI pad/cymbal velocity recalibration without remapping kicks.
- USB MIDI drums with **experimental/intermittent support**; keep a DualSense for menus and pause.
- Verified updates with persistent user resources and startup recovery.

## Updating

**Already installed and your assets are working? Use the normal in-game update; you do not need to rebuild.** Existing installations keep their imported resources through the runtime update. Older clients can first receive the v1.5.0 migration from the retired repository address; future checks then use this repository. For a fresh install, use the desktop builder above. The `_update.encore` download is for the in-game updater, not a standalone installer. Original PC assets are never downloaded by the updater.

The game-folder, exFAT and native homebrew/debug PKG formats are supported by compatible homebrew setups. These packages are not retail-signed. Preserve the complete title folder and your userdata when manually installing.

## Status and rights

This remains a work in progress. Physical four-instrument, five-lane, six-fret and MIDI testing needs broader reports. The reported RB4 Stratocaster problem still needs a tester log/retest. Online play, whammy and tilt are unfinished. PS4/Switch ports have no release date.

This project is not affiliated with, approved by or endorsed by the original PC developers. Supplying your own files does not itself grant permission to extract, modify or reuse them. The project does not claim that changing its name or removing its assets resolves all copyright, contractual or anti-circumvention questions. See [rights and distribution notes](docs/RIGHTS_AND_DISTRIBUTION.md).

## Building and reports

Native builds use the vendored prospero toolchain and the existing Vulkan 1.3/RADV renderer. See `ps5/README.md`, the corresponding-source archives, and `BUILDING.txt` in each release. Personal resources are never native code build inputs or public release attachments. The desktop builder adds them only to the user's private install output.

Read [known issues](KNOWN_ISSUES.md) and [diagnostics](docs/beta-diagnostics.md) before reporting a problem. Include console/firmware, launcher/version, controller/mode and reproduction steps. Discord: **mynameisawooo**.

## Credits and licences

Native code and third-party components retain their individual licence notices. The linked executable and corresponding source follow their applicable combined-work obligations. The asset importer contains metadata and code, not the original PC assets. Roboto, dependency certificates and user-supplied main-menu audio retain their stated licences/rights.

PlayStation and PS5 are trademarks of Sony Interactive Entertainment Inc. This is an independent homebrew project.
