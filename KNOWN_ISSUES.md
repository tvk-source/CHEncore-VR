# Known issues and test coverage

Applies to 1.5.0 (pursuaded). This is a beta test release, not a finished game.

## Installation

The game-folder executable has run on PS5. Each release's `TESTING.json` records
checks against its exact executable and package files. Container integrity and
extraction checks do not establish that a PKG installs or an exFAT image mounts
on every launcher. A compatible homebrew installer or mounter is required;
include its name and version when reporting startup problems.

## Clean-install startup fix

Alpha v.39 fixes a startup dependency on a development song's cover that was
absent from the release bundle. The fix passed main-menu, Quickplay and Settings
launches from an external USB folder on a base PS5 (reported firmware 11.20), at
4K and approximately 59.9 FPS. Firmware 13.00 still needs tester confirmation.
The new startup checkpoint log also covers failures before the gameplay logger.

## Controllers and multiplayer

Four-player gameplay and the five-lane visual layout have PS5 bot-run coverage. A physical five-lane kit and four simultaneous physical instruments remain unverified. Controller presets are not a guarantee for every model, firmware or operating mode.

Xbox 360 wired and wireless-receiver support is experimental; hardware was unavailable for validation. Whammy and tilt are unfinished. Online play is unavailable. Six-fret USB presets and gameplay are enabled; physical Guitar Hero Live dongles have not been validated. See [six-fret support](docs/six-fret-support.md).

The source adds an experimental native PS4 RB4 Stratocaster Bluetooth path; a tester reported no input from the original Beta 1.0.1 build. The refreshed build improves standard/special-port discovery and delayed input handling; physical retesting is pending. RetroCultMods Wii adapters must use a supported guitar report mode. See [input paths and verification](docs/rb4-bluetooth-and-rcm.md).

## USB MIDI drums

Enabled with **experimental/intermittent support** in Beta 1.0.1. The new-controller popup guides class-compliant USB MIDI 1.0 kits through 4-Lane Pro or 5-Lane setup, one/two kick pads and optional soft/hard pad/cymbal velocity calibration. Kicks have no velocity calibration. Keep a DualSense connected for menus and pause. Remap Controller reopens setup; mappings save by device and named player.

Host tests cover rapid kicks at 30/60/120 FPS and four isolated mappings. Native PS5 tests complete setup, JSON saving and injected-event gameplay. **No physical MIDI kit has been validated.** Proprietary USB protocols, Bluetooth/DIN without a compatible class-compliant USB adapter and MIDI 2.0 are not supported by this backend. A disconnected kit pauses play; reconnect/setup is available from the main menu. Identical adapters can share device defaults; use player overrides for different kits.

## Gameplay

Host regression and sanitizer checks cover documented scoring, drum input, profile persistence and chart conversion cases. These checks do not establish universal parity across every chart, audio format or controller. Detailed reference work is recorded in the gameplay audit.

## Reporting a problem

Use the repository bug-report form. Include the build, installation format, console/launcher, controller model and mode, instrument, player count and reproduction steps. Attach the diagnostic log and relevant screenshots after reviewing them for personal information.

Logs are stored in `/data/homebrew/PPSA98743/userdata/logs/encore-test.log`, with `.1` and `.2` rotated files when present.

## Updating and song management

Automatic update checks and verified runtime downloads start with Beta 1.0; older builds need a manual update first. Activation, replacement rendering and a fresh launch passed on the folder install. The updated engine also rendered from a read-only exFAT image. Installed native PKG startup and replacement rendering also pass with the pinned released writer. The complete application PKG also passed embedded-game and updated-engine checks. A replacement that fails before its first healthy render is restored on the next startup. Internal songs use `/data/homebrew/SONGS`; migration preserves existing destinations. Bundled songs in read-only images are hidden on removal. See [update and storage behavior](docs/beta1-updates-and-storage.md).

## Encore 2.0 targets

Leaderboards and online multiplayer are planned targets for 2.0, after the PS4 port is finalized. There is no release date yet.

- Leaderboards: identify charts consistently, separate instrument/difficulty/modifier categories, and validate submitted scores, accuracy and star ranks. Bot runs must remain separate from player records.
- Online play: lobbies, compatible-song checks, synchronized starts and resilient disconnect handling, while retaining instrument-controller navigation and local play.
- Review cross-platform compatibility as the console ports mature. Accounts, hosting, transport, moderation and score verification need design and testing before implementation commitments.

These are future targets; The next build is 1.5.0 (pursuaded).
