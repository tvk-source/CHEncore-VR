# PC guitar input timing audit

The subsequent [scoring edge audit](pc-scoring-edge-audit.md) adds exact late
deadline arithmetic, buffered-strum ordering/protection, raw health and exact
sustain-release recovery checks. It supersedes the earlier edge behavior where
those details differ, while retaining all 43 input-mechanics golden passes.

I compare the portable guitar session and the PS5 gameplay adapter with the
original PC engine executing the same chart and sampled input. Ghidra supplies
the control flow; the dedicated, headless PC process supplies measured results.
These are five-fret fixtures, not a certification of every PC gameplay mechanic.

## Source checkpoints

The locally owned PC `GameAssembly.dll` has SHA256
`c528a4e00fd14392e9c436ccd2c5a7ed5ff51907295c4d8cf2434b5175a0122f`.
Addresses below are preferred-image virtual addresses in that build.

- `0x1820F0840`, `0x1820F3390`: scheduled updates before a sampled endpoint,
  with up to three queue regeneration passes. Intermediate steps retain the
  previous sampled guitar state.
- `0x1820F2DD0`: early/late window scheduling, including floating-point boundary
  corrections. Misses retain their logical deadlines.
- `0x1820EFE00`, `0x1820F35B0`, `0x180FBB7F0`: Precision windows use adjacent
  chronological heads, clamped half-gaps and the PC arithmetic constants
  0.55238 and 42. The native modifier selector now exposes Precision Mode.
- `0x1820F1AD0`, `0x180FCB010`: current candidate timing refresh. Precision fret
  and strum eligibility allowances use the current full window divided by three,
  rather than retaining the constructor's initial 42ms value.
- `0x180FC9DB0`: same-direction strum debounce (default 5ms), buffer eligibility,
  and separate normal/Precision handling of protected re-strums. Alternating
  strum directions bypass the same-direction debounce.
- `0x180FCD9E0`, `0x180FCC230`: legato judgement, ghost threshold, timer
  cancellation and protection following an assisted or unassisted legato hit.
- `0x180FD1FB0`: unused-fret ghost handling excludes an upcoming ordinary strum
  and a HOPO with no recovery streak. A release must survive 7ms; late re-presses
  cannot cancel an already expired release. Meter deduction uses the expanded
  legato candidate range, rather than deducting only at a successful hit.
- `0x180FC8E20`, `0x1820F0FC0`: overstrum gating and streak reset. Strums before
  play and after the final head without a surviving tail are ignored.

The gameplay adapter settles sustained score, Star Power and judgement events
at each intermediate timestamp. Rendering and controller polling still run at
frame cadence; the replay input supplies logical guitar states independently
of the physical bridge. This does not measure physical USB or audio latency.

## Reproduction

`tools/pc-parity/mechanics_cases.py` generates synthetic charts and inputs.
`probe.py` and `pc_engine_probe.js` execute the original PC parser and engine in
an isolated process, closing that process afterwards. Only the owned fixture
objects receive injected input. The original binary and game data remain local.

`guitar_probe.cpp` executes the same C++ `GuitarSession` used on the console.
`compare_guitar.py` compares every sampled score, streak, hit count, ghost count,
charge, per-note window and logical hit timestamp with no tolerance.

The 43 cases contain 5,996 sampled frames and cover:

- 1/2/4/5/9 unused-fret releases with normal, No Fret Ghosting, Precision, and
  combined modifiers;
- held HOPO entry at 30/60/120Hz and during a long nonuniform frame gap;
- dense Precision windows and exact/adjacent 70ms hit boundaries;
- gamepad partial chord entry, open release, Classic Gamepad and Auto Strum in
  the portable engine (test flags do not add new modifier UI options);
- rapid same-direction versus alternating strums;
- deterministic mixed press/release/strum streams with normal, No Fret Ghosting
  and Precision options.

All 43 host cases pass against measured PC output. Existing gameplay, sustain
rules and sustain timeline host checks also pass with C++20, disabled exceptions
and RTTI, strict warnings, AddressSanitizer and UndefinedBehaviorSanitizer.
Old host assertions expecting pre-play/post-chart overstrums were corrected to
match the measured PC behavior.

`console_replay.py --mechanics` feeds those same inputs through the PS5 title's
real gameplay update and records `native.tsv`. `compare.py` checks frame input,
time, parsed gems, HOPO threshold, score, streak, hits, solo subtotal and ghost
state. `--case NAME` selects a bounded subset. The optional `.replay-options`
file and additional engine flags are read only by the test-run replay entry.
No profile or settings save is required. Temporary jobs and fixture files are
removed on completion.

## Console results

All 43 mechanics cases passed on the PS5 adapter: 5,996 sampled input frames,
6,512 rendered frames including the replay harness allowance. Each run reported
59.9fps after warmup, status 0 and title-owned exit. After the final dynamic
Precision allowance refinement, I reran all 14 affected Precision cases and
again obtained zero differences. The seven sustain/solo/HOPO regression cases
also passed against their measured PC traces (1,327 sampled frames).

The final link also updates the optional hit-window readout to show the next
head's calculated Precision window. Its eboot SHA256 is
`9cabc6032e901c081587ad0a2637074a4a591a4021fc48a648fbc0b19b6ce73d`.
It was built with this repository's vendored `prospero-clang++` and deployed.
The subsequent 4:00 AM bot smoke test rendered 3,600 frames in 63.37 seconds,
reported 59.9fps after warmup, and returned status 0 through the title's exit.
I inspected `klog/gameplay-combined/normal/cloneheronative.png`. The harness
restored and verified the original saved profile bytes and removed the test job.
Kernel logs were unavailable; these results use the title's own results file
and screenshots, without claiming a clean kernel log.

## Scope

Whammy and tilt are explicitly deferred until the guitar bridge has an analog
mapping. No bridge or analog input changes are part of this pass.

These fixtures do not establish parity for drum/pro-drum/six-fret engines,
all modifier combinations, arbitrary candidate overlaps, full-song physical
input/audio synchronization, MIDI/SNG ingestion, or solo/results presentation.
Those remain separate work. The native Vulkan foundation and existing artwork
are preserved; no generated assets are introduced. Upstream disposition: local
title gameplay and test work, with no SDK, driver or foundation change.

## Portable controller foundation (November 2026)

I separated controller decoding from the PS5 backend. `controller_profile.*`
contains exception-free C++20 JSON parsing, native/raw locators and logical
instrument state; it contains no scePad types. Native button polling still uses
`scePadReadState`. The bundled DIY profile follows the actual bridge report:
Yellow maps to Square and Blue to Triangle. Whammy and tilt remain deferred.

Device defaults live in `assets/controller_profiles/<vid>_<pid>.json`; writable
raw-device defaults take precedence in `/app0/userdata/controllers/`. Existing
shared defaults in `/data/cloneheronative/controllers/` are also imported when
visible to the title.
Player overrides, device identities, controller types and player settings live
in `/app0/userdata/clonehero-profiles.json`. Legacy profiles are imported without
removing the original configuration. Saves validate before replacing the file.
Native per-player mapping works without knowing a VID/PID; saving a native device
wide default is disabled until a reliable native-device identity is available.
I did not invent a Riffmaster identity or mapping.

The direct Remap Controller row opens a staged capture, conflict/swap and review
flow for each player and instrument. Five-fret, six-fret and drum classes have
separate binding sets. Pro drums retain separate pad/cymbal bits from the PC
input table (`0x1820D41E0`): kick/red/yellow/blue/green/yellow cymbal/blue cymbal/
green cymbal are 1/2/4/8/16/32/64/128. Velocity locators can be decoded; dynamics
bonuses are not enabled without hardware evidence. Six-fret mapping support does
not establish a six-fret gameplay engine. Five-lane input still uses the existing
four-pad chart compatibility conversion rather than a dedicated five-lane board.

A native USB worker discovers non-boot HID interrupt-IN interfaces and prompts
for an unknown device's instrument before opening its endpoint. It excludes
keyboard/mouse boot interfaces and does not detach storage or audio interfaces.
The native probe successfully opened `/dev/ugen` nodes and read descriptors for
this guitar. No credential elevation was needed for those operations; the
payload credential helpers cannot link as public native-title imports. Descriptor
access alone does not prove interrupt transfers work. The worker leaves the
12ba:0100 endpoint alone while the existing bridge owns its lock.

The optional `payloads/rhythm_vdi` copy now keeps its virtual pad/user assignment
alive across physical USB disconnect and feeds neutral input while rescanning.
It builds with the vendored payload SDK. I could not safely identify the running
bridge process: process-FD inspection is unsupported on this setup and the
payload's process name is generic. I did not signal an unidentified process or
reboot. The new bridge needs a controlled restart and a real unplug/replug test.
The portable profile layer does not require this optional PS5 payload.

Host verification: `tools/test-controller-storage-host.sh` passed with ASan/UBSan,
no exceptions or RTTI. Coverage includes all instrument mappings, raw reports,
hat diagonals, malformed/invalid UTF-8 JSON, atomic persistence, multiple device
and player overrides, join ordering/compaction, and drum edge/reset/bot timing.
These tests do not establish physical Riffmaster, drum kit, velocity, whammy or
USB hotplug behavior. Upstream disposition: title input/backends and an optional
payload integration; no SDK or RADV fix identified by this work.

### Final native USB and source-routing evidence

The native interrupt reader received complete 27-byte reports from 12ba:0100,
recorded in `klog/controller-stage1/direct-usb-discovery`. A separate HID
GET_REPORT request failed with EIO; interrupt transfers are the working path.
The title could not establish bridge ownership from the payload lock, so I
installed a title-owned `userdata/controllers/legacy-bridge` marker for this
console's existing setup. With that marker, the final native run reports
`bridge_owned 1`, no raw stream, and the existing scePad connection remains
available. This prevents duplicate input without stopping an unidentified
payload. After a controlled bridge stop, removing that marker enables the
built-in reader. Direct physical strum/fret timing and unplug/replug still need
a hands-on check; receiving idle reports alone does not prove those behaviors.

The source-slot console fixture joins physical slot 4 before slot 1, disconnects
and reconnects slot 1, compacts seats after slot 4 leaves, and opens the remapper
for the surviving P1. Disconnecting that player closes the unsaved wizard.
Every asserted transition passed. These are synthetic source-slot events
through the real seat-routing code, not a claim of physically unplugging pads.
The freshly compiled drum adapter also passed all 61 stored PC-engine traces.

### Final build and instrument wizard coverage

Vendored `prospero-clang++` built and deployed eboot SHA-256
`3acadb78887d41ee35e0902727e9d50dce5704de360db1c0ed0681aa0b9b6681`.
The normal Vulkan bring-up and original menu/gameplay artwork are preserved.
Native synthetic-source captures for Pro Drums (10 actions), five-lane drums
(8 actions) and six-fret guitar (10 actions) all reached the correct review
page with distinct mappings. I inspected the Pro Drums review capture. Evidence:
`klog/controller-stage1/{pro-drum-wizard,five-lane-wizard,six-fret-wizard}`.
The earlier five-fret and standard drum cases pass too. Every bounded run exited
through the title with status 0. These are wizard/input-routing checks, not
hardware or full gameplay parity for the additional instrument engines.

The final main-menu smoke run also exited with status 0. Its capture was inspected,
and the compatibility marker kept the raw guitar stream disabled while the
native pad remained connected. All temporary launch/trace files and the owned
storage fixture were removed. A subsequent plain launch reported PPSA98743
running; the console was left on the normal build, not a bounded QA launch.

Outstanding hands-on checks: controlled bridge stop/restart, direct guitar
strum/fret and unplug timing, an actual external SONGS drive and unplug/copy,
a licensed guitar's real native identity, and real drum/pro-drum pad/cymbal
reports. Whammy/tilt remain intentionally deferred. The port still does not
provide independent multiplayer highways/scoring or a six-fret gameplay engine.

### Direct USB after disabling GameBridge, 2026-11-17

After a console restart with GameBridge disabled, I removed this title's
`userdata/controllers/legacy-bridge` reservation (backup in the local run folder).
I reloaded only the console-control service. The existing native input reader
recognized 12ba:0100 using its bundled five-fret profile and received 27-byte
interrupt reports on each sampled frame, with `streaming 1`, `unknown 0`, and
`bridge_owned 0`. The 180-frame run exited normally at 59.9 fps; evidence is in
`klog/controller-stage1/direct-usb-no-payload/`. No player profiles or fret
bindings were changed. The normal game was then launched for a physical
Start/strum/fret check. Button response, actual play, and unplug/replug remain
unverified until that hands-on test is completed. Klog was not captured.

This confirms the direct transport for this specific guitar on this console;
it does not establish automatic mappings or native OS support for every USB
rhythm controller. The native reader uses in-game seats instead of creating
virtual DualSense devices. GameBridge coexistence still uses endpoint ownership
or the explicit compatibility reservation; the reservation is now absent for
this direct-input test.

## Alpha v25 regression, 2026-11-17

Build `5971e07a355f9fdc7ee159c45070dfdf51d415ffdfe313b48a1e34416449662a`
passed the seven on-console sustain/solo/HOPO fixtures without differences
from the recorded PC frames. Six additional console fixtures covered 30/120Hz
logical sampling, Precision, held HOPO across a frame gap, and early/late
window boundaries. The portable comparisons also passed all 43 guitar cases
(5,996 frames) and 61 drum cases. Console drum fixtures `singles-0`,
`partial-chord-0`, and `activation-earned` passed with zero differences.

Judgement searches now start at pending heads within the applicable early
window, and live sustains are processed separately from the full chart.
USB histories retain up to 64 changed, timestamped reports per device between
frames. The host queue test verifies a complete strum pulse between frames
that a latest-state-only reader misses. Console discovery again found the
12ba:0100 guitar streaming 27-byte reports with no bridge ownership. Physical
fret/strum play and unplug/replug still need a hands-on test.

Seven keyboard/display console cases passed, including holding Start after
text acceptance without opening/joining the underlying player card. Existing
profile bytes were unchanged. A complete USB song reached Results with
885/885 gems, zero misses, score 260564 and nine stems; pause stopped the audio
clock and resume advanced it again. This is a bot smoke test, not a PC score
reference for that song or a listening test. Evidence: `klog/input-keyboards`,
`klog/pc-engine-parity`, `klog/mechanics-console`, `klog/drums-console`, and
`klog/first-edition/full-song-usb`.

The subsequent display-query build
`2b0303658f5ef170bb681c82526d43f7b17a5aabae3e2baec97278b1a7ba1143`
passed a separate bounded console launch. Before existing Vulkan prepare,
the title queries and closes its own temporary VideoOut handle; it does not
register buffers or replace WSI presentation. On the user's confirmed 2160p
setting, `sceVideoOutGetOutputStatus` returned success and selector 2; the
advertised 3840x2160 mode was selected. Presentation reported 59940mHz;
the saved 120 preference fell back to 60 with 120 selection disabled.
Evidence: `klog/controller-stage1/output-query-4k/`.

The exported 48-byte status ABI and selector 2 are documented in
[John Tornblom's public VideoOut example](https://github.com/john-tornblom/ps5-payload-hbldr/blob/master/test.c).
The public [AnyPS5 driver header](https://github.com/boykopovar/AnyPS5/blob/main/core/libs/prx/libSceVideoOut/include/VideoOutDriver.hpp)
also identifies selector 1 as 1080p. Only recognized sizes advertised by Vulkan
are selected; unknown selectors preserve the existing size. 1080p output,
the unresolved 1440p selector, a physical 120Hz display, and changing screens
while the process remains running are not verified by this 4K/60Hz run.

No end-to-end latency measurement is claimed. Fixed-memory diagnostics record
acquisition-to-processing age and judgement errors, writing on pause/results.
Native scePad input still samples at frame cadence. Whammy and tilt remain
explicitly deferred at the user's request. All changes here are title policy
and gameplay behavior; no foundation, SDK or driver implementation was changed.

## Alpha v26 keyboard and song-start work, 2026-11-17

The title now translates the left stick into cardinal menu input, with separate
engage/release thresholds and vertical-only main-menu navigation. Gameplay
controller axes and the remap capture remain unchanged. The built-in keyboard
uses normal D-pad/stick grid movement for DualSense assistance. Five-fret
guitars retain linear strum navigation with yellow/blue row shortcuts, keeping
every key reachable without extra controller buttons. A helper DualSense edits
the initiating player's draft, without assigning that helper a player profile.
The compact panel is 920x416 at (500,592) in the 1920x1080 canvas, with 16-pixel
side margins and a translucent panel over a dimmer; errors add one line only when present.
This is the port's own text-entry UI, not a pixel-certified copy of system IME.

Ten native keyboard checks passed on build
`5791a7344e10b4f45bb500e879d24060e407139dc8c0c8616bc2d4a9e0c233aa`,
including grid movement from D-pad/left stick, a second-controller helper,
held-Start confirmation, cancel ownership, opacity, and original system IME
accept/cancel. Host ASan/UBSan checks cover wide action-key row navigation,
stick hysteresis/vertical restrictions, and exclusion of 3/8/90-second loading
delays from the new song timeline.

Ghidra `GameManager.Start` at `0x180282E30` starts the asynchronous state machine
at `0x1802A9880`, which awaits preparation before continuing into the song
routine. The audio/chart preparation iterator at `0x1802ACE60` calls the
BassAudioManager loading method, validates preparation, and constructs the
song timeline. The native bug was different: the base reports render duration
as the next frame's delta. Synchronous decoding could therefore become time
advanced by the newly reset clock.

`SongStartGate` now holds a restart until its first prepared gameplay frame
has been submitted, then substitutes only the real time since that submission
for the inherited loading delta. Normal song-start decoding uses the existing
worker and fixed PCM budgets. Full preview PCM can be reused; an in-flight full
preview job can also satisfy the same request. Dedicated preview clips are not
used as full songs. The selected song is prefetched while players choose
instruments/difficulties; the actual gameplay chart is parsed once at Play.

The final Ready-screen implementation is build
`1fb5acf46474378a20d6676ff2ffb6fa586d18e9508e3db9828ec0a477aaf98f`.
The native cold-load capture remains on PlayerSelect with Ready visible,
audio paused and judgement stopped. Its thin moving bar matches the preview
bar colors and makes no percentage claim: the codecs expose completion, not
incremental byte progress. Red cancellation stays on selection and cannot
auto-launch when the outstanding worker completes. In the nine-stem USB
prefetch run, audio decode took 2256.225ms, chart parsing took 2.221ms and the
wait after Ready was 5.039ms with a cache hit. This measures one song on this
console; it does not establish a general decoding speedup or Switch timing.
Evidence is in `klog/ready-loading/`, with stages in `song-load-status.txt`.

The preceding build `8f57dc08edfafe7a097ddc23e48ec2a5ff656b25140cff321ced63a0b53b8483`
completed the 885-gem/nine-stem song at score 260564 with no misses, passed
pause/resume, and matched all seven recorded console scoring fixtures. Its
injected 8-second inherited frame advanced the new song by 15 microseconds,
from -3 to -2.999985 while audio remained at zero. The later Ready-screen
revision is tested separately; do not attribute earlier captures to its hash.

The Ready-screen revision also completed that whole song with 885/885 gems,
zero misses, nine stems, working pause/resume and a clean Results transition.
Its injected 8-second frame became a 29-microsecond first step, with audio
still paused at zero. Its final score was 260552, versus 260564 in the earlier
run. These runs use the live audio clock rather than identical replay frame
timestamps; neither score is an established PC reference for this song. A
full-song PC replay with the same timestamps is still needed before claiming
frame-cadence-independent bot scoring or exact whole-song PC score parity.

Final Alpha v26 build
`55c0c9b31d7e4ca895bfac3cc849e182b42460ba58d3a25584172560e6b78a05`
keeps the main mixer's menu-cue output open while the staging mixer loads,
holds the Ready clock at pre-roll, and shows only Red: Cancel loading during
the wait. All three Ready/cancel/prefetch cases passed again, including live
audio output with no song playback during cold decode. The measured prefetch
case was 2232.063ms audio decode, 2.227ms chart parsing and 5.256ms Ready wait.
Final native captures verified VERSION NOTES / ENCORE FEATURES switching,
the corrected fixed banner title, and opaque keyboard input from a second
DualSense while retaining owner 0. `klog/alpha26-final-verdict.json` records
the exact build and evidence paths. No foundation, driver, SDK or asset
implementation changed in this batch.

Keyboard backdrop follow-up: build
`d1c7ed836cc0020257534ec29ee5fe3ced4156d67bab52e6f747d11539a9ebed`
uses a translucent black dimmer (alpha 112) behind the solid keyboard panel,
so the game remains visible. The extra navigation sentence is removed.
Done and Cancel now use the original `006_start` and `007_select` sprites;
the staged Start PNG is byte-identical to the extracted source. The native
capture and pixel checks passed in
`klog/controller-stage1/keyboard-backdrop-icons/`.

The subsequent visual-only keyboard revision
`c85420e629096688d21fa2c7bc2a0d2324cf3709a4b9da7ac929b8c03bae2121`
matches the main menu's panel color/alpha `(9,14,25,128)`, retains the dimmer,
and renders the empty placeholder in gray `(116,120,128)` with the caret at
the actual empty-text insertion position. Entered text remains white. The
native screenshot's placeholder luminance maximum was 120; capture evidence
is in `klog/controller-stage1/keyboard-translucent/`.

## Directional input and player-color follow-up, 2026-11-17

The title cleared `sourcePads[slot]` immediately before calling `pad_player`.
The platform's `pad_fill` computes `pressed = held & ~before`, where `before`
is the caller's retained `held` mask. Clearing the caller's state turned every
held native button into a new edge, defeating repeat delays and affecting the
keyboard's auxiliary DualSense path. The title now preserves that state between
successful reads and clears it on a disconnected read. No platform or Vulkan
bring-up change was needed.

The built-in keyboard now uses the same visible-grid directions for every input
source: Up/Down (including strum) change rows, Left/Right change columns, and
Yellow/Blue move left/right. A source's instrument classification no longer
reinterprets Up/Down as previous/next character. Stick navigation and timed repeats
are also enabled in controller-remap review; capture still receives physical
buttons without synthesized stick directions. A helper DualSense can navigate a
raw instrument's review using its stick without changing binding ownership.
The main menu remains vertical-only.

Guest labels follow join-order seats: green, red, yellow, blue. The slot's guest
sentinel controls color, so saved profiles merely named Guest retain normal text.
The same guest identity is retained for gameplay/results labels. Orange is reserved
for vocals. Both storage-copy modals use alpha 128 for their title/body background,
matching menu-item translucency; text, button prompts and selected rows remain
opaque.

Ghidra MCP was available: `PlayerProfileMenu$$FixedUpdate` at `0x180173730`
was rechecked for the PC per-player menu path. Guest color choices and controller
keyboard navigation here are the requested console UI behavior.

The subsequent requested UI revision replaces the separate remapping scene with
an inline wizard inside the owning Press Start card. Each seat now owns its draft,
capture/release state, duplicate-binding conflict, review selection, message,
previous-button mask, and optional helper-controller reservation. Mapping P2 no
longer changes the shared screen or input owner. Native helpers are selected only
from unjoined controllers and are reserved until that player's remapper closes;
joined players' inputs are never borrowed. Remap state follows seat compaction,
and Ready cannot launch a song while any setup is still open. The card uses the
same alpha 217 background as every other Press Start panel. Existing colored fret
sprites identify capture targets and confirm/back actions; Start/Select use the
existing source sprites at their proper wide aspect.

The USB copy body remains alpha 128. Following the later visual request, the
copy title strip instead uses the dark Settings banner fill (8,8,8,255), so
bright selected settings behind it cannot lighten the title.
The copy-capture harness now opens the dialog after frame zero so the shared menu
trace exists; the earlier missing trace was a harness assumption, while the title
itself completed all 120 frames and exited with status zero.

Validation: the input fix passed twelve bounded native keyboard/display checks
on `58c46b846e1cdb51c625601d30f8c2509b7fad28cec392f662669a7bf185f9d3`.
The inline revision passed seven guest/remap/copy checks plus separate pro-drum,
five-lane and six-fret capture/review runs on
`69e2b55fe9dc1bc5675ebeb09fa591d7a5400125da07858552c48e63b5a8fe9d`.
These include simultaneous independent remappers, P1 menu input while P2 maps,
capture isolation from stick movement, and screenshot checks of the color prompts.
The last product change only darkened the copy title strip; the final deployed
build `6b3c4b1a95c8ab94d6cb7f15fc9745f6a9918290f87470f29202d17b13204538`
passed Guest save, native slot/seat disconnect during remapping, and both copy
confirmation captures. Persistent saved-profile bytes were unchanged and no song
copy was confirmed. Final screenshots were inspected; tests were removed before
normal launch. Evidence: `klog/input-keyboards/verdict.json`,
`klog/controller-stage1/guest-input-copy-verdict.json`, and
`klog/controller-stage1/input-remap-copy-final-verdict.json`.
These are scripted native UI checks, not a claim that four physical instruments
were attached and tested simultaneously.

Remap alignment follow-up: removed the input-number counter. Headings, wrapped
messages, review choices, and icon/label groups now center within each 435-unit
player card. Paired confirm/back and swap/retry controls center in equal-width
columns. Native capture/conflict/review captures passed and were inspected on
build `2c55b6d48bfec0f345695f07089139d18134be7eb71bafd0b8419f99190c3b2b`;
see `klog/controller-stage1/remap-centered-verdict.json`. The test configuration
was removed before the deployed title's normal launch.

## Menu bounce protection and gameplay setting, 2026-11-17

Ghidra recheck: PC `BaseMenu` input helper `0x1802568F0` handles button-down,
release, and the existing repeat timers; it does not supply the requested extra
75 ms physical-bounce suppression. PC guitar `0x180FC9DB0` compares the same
strum direction against the stored timestamp plus threshold at engine `+0x3F8`.
Changing direction bypasses the comparison and starts the timer again. Constructor
`0x180FD2230` initializes that threshold and clears it for the gamepad/autostrum
modifier flags `0x104000`. Existing measured default is 5 ms. The extracted
`assets/assets/TextAsset/en.txt` lines 803–807 confirm the PC option's name,
**Double Strum Prevention**, and describe the same-direction/changed-direction
behavior. This setting is now under Gameplay, with Off/0 through 50 ms and the
5 ms default. It uses song time like the PC engine; frets and drum strikes do
not receive the menu filter. Settings record v25 appends the value, leaving old
records valid with the PC default. Slider cancel does not commit a new value.

Menu filtering is separate: first edges are accepted immediately, subsequent
edges of the same action inside 75 ms are suppressed, and vertical/horizontal
navigation groups also suppress quick opposite-direction rebound. Different
actions and different players retain separate histories. Held-navigation repeats
are generated after filtering using the PC .3/.026/.013 second timers. P1's
screen and player-card consumers share one accepted result per frame; keyboard
assistants retain their own history. Pause/Resume Start edges use menu protection
without routing gameplay note inputs through it.

Remap review/navigation also uses menu filtering. Native binding capture filters
only the same physical button, allowing a different button or strum direction
immediately. Raw USB capture uses an independent per-player locator history to
ignore repeated edges from the same byte/bit or hat value; different fret/pad
locators remain eligible. Captured drafts, duplicate resolution and player
ownership remain separate. No Vulkan, platform bring-up, or extracted artwork
changes were involved.

Host checks passed under ASan/UBSan: menu filtering plus held-repeat behavior,
native gameplay default/custom/off thresholds and alternating bypass, raw remap
locator filtering, and the existing gameplay-rules suite. All 43 previously
measured PC guitar reference cases still matched using the default setting.

Seven bounded native checks passed on
`8127659c27299595ec6fc2d0e9d5d60e46d991b776f32eb831847b743e4ab1ae`:
main-menu bounce, profile Start/confirm bounce, held scrolling, keyboard typing,
remap capture/review bounce, gameplay setting save, and fresh-process reload at
6 ms. The exact original preference bytes were restored afterwards. Evidence:
`klog/controller-stage1/menu-filters-verdict.json`. Test backup/restore locates the
running title's mounted `/download0`; `/user/download/PPSA98743/download0.dat`
is the backing container, not a directory to edit.

The subsequent label-only revision adopts the PC's exact Double Strum Prevention
caption; its native screenshot/check shows the restored 5 ms default. The final
keyboard header change gives the title row the Settings banner fill (8,8,8,255)
while retaining body alpha 128 and the existing fullscreen dimmer. Final deployed
build `00fd9fef32b24e1553f979b36c28f475e7ee6a0a94d2444ca991e63b7f3531b3`
passed a native keyboard bounce check and dark-header pixel check; the screenshot
was inspected. Evidence: `keyboard-dark-title-final-verdict.json` and
`double-strum-prevention-final-verdict.json` under `klog/controller-stage1/`.
Normal launch and byte-for-byte preservation of original preferences were
confirmed in `debounce-final-launch.json`. No physical switch-bounce capture is
claimed by the scripted checks.

## Encore packed-note recovery

The owned PC engine was probed again with a four-head 30 ms HOPO run. Its
chronological legato gate rejects physical strums directed at later matching
heads while an earlier unplayed HOPO remains in the overlapping window. The
local reference is retained in `klog/library-parity/packed-recovery-pc.log` and
the private Windows probe workspace. Encore now permits only a fresh physical
strum to recover onto that later matching head, marking skipped heads missed
before awarding the recovered hit. Held and buffered legato keep PC priority.
This is an intentional playability adjustment, not a claim of identical PC
behavior in this edge case. The existing 43-case PC guitar comparison still
passes after the change.

`tests/guitar_frame_timing_tests.cpp` independently schedules 30/60/120/240 fps
and long frames over the same acquisition timestamps. Packed 30/40/62.5/83.333
ms runs, deliberate misses, alternating strums and complete 2 ms pulses retain
identical hit indices, miss counts and event timestamps. This verifies the
timestamped raw-input path, not the physical polling latency of every device.

### Fast tap recovery

Ghidra `0x180FCD9E0` distinguishes tap (streak-independent) from HOPO
(recovery strum required after a broken chain). The native candidate-zero
restriction could still block a new tap press while an earlier, differently
fretted tap remained pending in the overlapping late window. A minimal four
head, 30 ms run reproduced one hit instead of three.

Normal tap play now allows a fresh fret change, including a pull-off, to
select the next matching in-window tap and settle skipped heads as misses.
This extends Encore's explicit physical-strum recovery adjustment to taps;
it is not a claim of matching the PC candidate gate in this edge case. It
does not reuse held input to award multiple taps, widen any further windows,
remove the HOPO recovery strum, or change Precision/All Strums constraints.

The PC-parsed first 240 tap heads of the installed Through the Fire and Flames
chart pass with deliberately skipped heads at 30/60/120/240 fps, 1000 Hz
reference cadence and injected frame stalls. Synthetic tests cover 30–83 ms
spacing, early recovery, anchored pull-offs, repeated same-fret taps and
ordered misses/hits. ASan/UBSan timing-boundary and strum-filter suites pass,
as does the existing 43-case original-PC guitar comparison. Physical guitar
play remains the final perceptual check.

The native console prefix check also passes: all 218 intended fret-only taps
hit, and the 22 deliberately skipped heads miss, across the 240-head opening
run. It exercises the actual chart loader and gameplay adapter, with bounded
QA batching rather than a physical guitar. `klog/tap-recovery/verdict.json`
records the deployed binary and result. The title exited normally and was
reopened without test configuration for physical play.


## Alpha v.33 four-player scoring boundary

The four-player replay comparison exposed one guitar/bass sustain point being
paid after a streak break instead of before it. Excluding the input instant
previously subtracted a whole chart tick even when that tick started before a
fractional input timestamp. The score adapter now queries the immediately prior
representable time, preserving every earlier tick while excluding an exact
boundary. `tests/sustain_input_boundary_tests.cpp` covers both cases.
Ghidra's sustain routine at `0x180FCE040` supplies the raw-tick reference.

With equivalent timing, all 4,804 unique player/frame samples (1,201 frames,
four instruments) now match the owned PC engine, including score, fret and solo
bonuses, health, Star Power and shared band score. Evidence is in
`klog/multiplayer-release-audit/`. The isolated PC probe configures its normal
half-window to match the native setting; stock 70 ms references remain separate.
The default PC probe still uses stock timing. This comparison certifies the
recorded inputs and charts, not universal parity or unmeasured hardware latency.


## Alpha v.34 additional sustain investigation

The new overlapping-tail replay distinguishes missed heads from extra strums.
The measured PC trace keeps two held tails after a plain miss at 3.33 seconds,
but retires the unfinished tail on an overstrum at 7.4506666667 seconds. Native
code previously continued paying that tail, first adding two extra points at
sample448. An overstrum now retires only unfinished scoring tails. Fully paid
tails must retain their recent-fret allowance; clearing that allowance made a
later valid note miss in the original four-part regression. The final checks
exercise both cases.

Relevant Ghidra inspections: `0x180FCB670` for extended/recent fret matching,
`0x180FCE040` for sustain tick settlement and `0x180FCE500` for scheduled sustain
updates. The live PC frames provide the observed tail-retirement boundary.
`0x180FCA440` is a multiplier-change settlement wrapper, not a universal tail
clear; `0x180FCAD50` wraps Star Power activation. Do not infer their purpose from
obfuscated symbol resemblance alone. Evidence resides in the original and
`pc-compatible-overlapping-sustains` replay records and the private sustain-debug
probe output.

The initial 219BPM tempo fixture also exercises Encore's intentional fresh-input
recovery difference after a missed overlapping head. Its mismatch record is
preserved at `klog/release-engine-suite/tempo-boundaries/`; it is not relabeled as
PC-identical. Additional tempo/scoring cases isolate calculation boundaries from
that documented input policy. No universal parity claim is made.
