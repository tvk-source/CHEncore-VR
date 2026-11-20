CH: Encore v1.5.0 — personal asset importer

No PC artwork, fonts, sound effects or songs are included here.
Use this tool only with resources you are authorized to extract and use.
Possessing a PC installation does not by itself establish that permission.
This does not grant rights or override the original developer's licence.

Windows source fallback: install Python 3.12 x64, extract this folder outside the PC
installation, and double-click Create-Asset-Pack.cmd. Enter the installation
folder when asked. It installs the pinned dependencies into a local .venv.

Linux/macOS (Python 3.11+):
  python3 -m venv .venv
  .venv/bin/python -m pip install -r requirements.txt
  .venv/bin/python create_asset_pack.py /path/to/your/PC/game

The tool supports the original resource fingerprints in recipes.json and the
exact Linux reference fingerprint set in create_asset_pack.py. The Linux
profile is tied to the original catalog; all output checks remain mandatory.
Other versions and mixed fingerprint sets are rejected before extraction.
The PC installation is read only. No game executable, DLL, decompiler output,
encryption key, access-control bypass or protected bundled-song extraction is
used. Only the listed resource files and interface sounds are read.

Copy EncoreAssets-v1.encore-assets to the root of an exFAT USB drive. Open
CH: Encore on PS5 and use a DualSense to select Import. Circle cancels.
The asset pack is kept separately from code updates, songs, profiles and
scores. Do not redistribute the generated pack or upload it with bug reports.

Code: Copyright (C) 2026 HOWL, MIT. Dependencies retain their own licences.
https://github.com/MyNameIsAwoo/CHEncore-PS5
