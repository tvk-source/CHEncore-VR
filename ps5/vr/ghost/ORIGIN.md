Unmodified files from the user-provided Ghost-VR-main.zip, `pc/ps5xr/runtime/src` and `core`. Copyright (C) 2026 theghostonline, GPL-3.0-or-later per THIRD_PARTY_NOTICES.md. Upstream: https://theghostonline.dev/theghostonline/Ghost-VR
The separate CHEncore sources retain their MIT notices; a distributed combined VR build must meet GPL-3.0-or-later and other linked dependency obligations. No Sony libraries or original game assets are included. No PS4 payload, game injection, credential modifications, hand-tracking enablement or streaming stack is needed for this adapter.

CHEncore-VR modifications: exact 7.61 table selection and no external export override for the probe; validate executable offsets, partial shutdown, display startup error handling and a bounded steady-cross self-test. Original licence retained.

The second diagnostic revision enlarges the static pattern with a grey ring,
allows 540 frames with a 12-second loop deadline, and warns if unregistration
is not confirmed. No changes to native export offsets or cleanup call order.
