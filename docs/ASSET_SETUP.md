# Personal install builder

The public CH: Encore download contains the desktop builder and an **asset-free game template**. It does not contain original PC artwork, fonts, effects or songs. The builder reads your own local resources and creates a personal install in the format you choose.

Use only resources you have permission to extract and use. Possession of a PC installation does not itself grant that permission or resolve the original licence restrictions. See [rights and distribution notes](RIGHTS_AND_DISTRIBUTION.md).

## Build on Windows

Download **CH_Encore_1.5.2_Install_Builder.exe** and double-click it. It bundles Python 3.12, the importer and its dependencies, the asset-free game template, and offline exFAT tools. **No ZIP extraction, Python installation or compiler is required.** Folder and exFAT builds work offline on Windows x64. PKG creation automatically downloads and verifies its native writer on first use, then uses its cache.

1. Choose a detected installation, or **Browse** to your standalone/portable installation or launcher-managed game folder. You can select its `*_Data` folder directly.
2. Choose an output folder and **Game folder**, **exFAT image**, or **Native homebrew PKG / fPKG**.
3. Confirm permission to use the local resources, then select **Build install**.

The included Packwright exFAT backend is built from pinned open source and runs as a separate process. Its source/licence/notices are embedded alongside the tool; no modules are injected into the game. Native PKG installation and a 240-frame launch were checked on a PS5 Pro using the proven writer. Broader firmware/launcher testing is still needed.

## Source builder / Linux and macOS

The source builder remains in `tools/install-builder`. On Linux/macOS, use Python 3.11+ with Tk and venv, then run **Start-Builder.sh**. For Windows source development, **Start-Builder.cmd** explicitly selects Python 3.12 x64 and uses `.venv-py312`; the failed Python 3.15 `.venv` is ignored. The source workflow installs pinned dependencies and downloads its packaging tools on first use. It is separate from the self-contained Windows executable.

Detection and compatibility are separate: **only the resource fingerprints in `asset-importer/recipes.json` are supported**. An unsupported PC version is rejected before extraction or packaging-tool download. Selecting a launcher executable does not select a game installation; choose its installed game folder instead.

The builder reads plain asset files and explicitly listed UI sound files. It never runs the PC game, reads decompiled scripts, accepts decryption keys, or reads/decrypts protected song bundles. The original installation is unchanged. An existing output is not overwritten; failed/cancelled builds are cleaned up rather than presented as completed installs.

## Install on PS5

Use the resulting folder or container with your compatible PS5 homebrew setup. These are native homebrew/debug packages, not retail-signed applications.

**Your personal assets are already inside the output.** First launch shows a preparation progress display and automatically installs them into persistent storage. No separate asset file copy, FTP upload or console import selection is needed. Later launches use the prepared assets.

Assets are kept at `/data/homebrew/PPSA98743/userdata/pc-assets/`, outside executable/runtime updates. Songs, profiles and scores remain intact. If a future build requires a different asset catalog, rebuild with its matching desktop builder. The game never downloads original PC assets from GitHub or another service.

**Keep generated personal installs private.** They contain your imported resources. Do not upload them to GitHub, share them as releases, or attach them to issue reports. Share the unmodified public builder ZIP instead.

## Existing v1.5.0 installs

An already imported asset pack remains usable through normal code updates; rebuilding is unnecessary. The standalone `create_asset_pack.py` tool and manual USB import remain available as an advanced fallback. It creates `EncoreAssets-v1.encore-assets`, which the existing setup screen can import from an exFAT USB root or `/data/homebrew/EncoreAssets-v1.encore-assets`.

## Songs

No songs are imported from the PC game or included in personal installs. Add only charts/audio you are authorized to use through Encore Browser, `/data/homebrew/SONGS`, or an exFAT root-level `SONGS` folder. The protected bundled-song conversion tool is not included.
