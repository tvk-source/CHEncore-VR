# Desktop builder releases

Public downloads use the single Windows desktop-builder EXE for new installations and the asset-free `_update.encore` payload for existing installations. Do not upload the personal folder, PKG or exFAT outputs produced by the desktop builder. They contain the user's imported resources.

After committing all changes and building the native title with the vendored toolchain:

```sh
python3 ps5/tools/check-no-pc-assets.py
python3 tests/install_builder_tests.py
python3 tests/asset_importer_paths_tests.py
bash ps5/tools/test-asset-pack.sh
bash ps5/tools/test-updates-and-storage.sh
python3 ps5/tools/package-desktop-builder.py --output dist/CH_Encore_VERSION_desktop-builder.zip
python3 ps5/tools/build-update-package.py dist/PPSA98743 dist/CH_Encore_VERSION_update.encore
```

`package-desktop-builder.py` requires a clean source tree and matching built-source notices. It audits the application and builder, checks the automatic-setup capability and catalog, then creates `base-game.zip` with a complete file-hash manifest. This is the only template to put inside the public builder. An already prepared personal install is rejected by the unchanged public resource audit.

Attach the complete corresponding source/licence archives and checksums alongside the builder/update. The native title has the same component/source obligations inside the desktop template as a directly packaged release. Retain the exact built tag and download/read-back verification. Never modify published artifacts in place.

The optional Python source builder downloads pinned PS5 UltraPack 2.6.1 releases/source from GitHub. Its MkPFS exFAT serializer/reader and standalone native package writer run as separate processes. The exact archive and executable hashes are in `tools/install-builder/backends.py`. The user's tool cache retains complete upstream archives/notices, and no third-party metadata or entitlement module is inserted into CH: Encore. Audit tool updates independently; do not silently substitute an arbitrary package writer.

Tests must distinguish native Windows desktop packaging, Linux desktop packaging, host Vulkan setup, and physical console runs. A container validation success is not proof of a fresh console launch. A generated pack can differ in PNG compression bytes across platforms; its pixels, asset recipe/catalog and own file checksums must validate on each platform.

## Windows single-file executable

Run `ps5/tools/build-windows-builder.py` with native Windows CPython 3.12 x64 and PyInstaller 6.16.0. Supply the audited intermediate builder folder, an unmodified self-contained win-x64 Packwright 1.3.0 publish from source commit `7039684af5c978ae69ee619d44e98102a61104a6`, and its complete source ZIP. The executable embeds the template, importer, dependency data/native extensions, Packwright runtime, complete corresponding source and licence notices. Folder/exFAT must not request additional downloads. PKG downloads the verified native writer on first use; no manual tool setup is needed.

Test the frozen executable rather than only its Python sources: `--self-check --offline --log-file CHECK.log`, then actual folder/exFAT builds with Python removed from PATH and `--offline`. Test PKG with a fresh cache online, then a cached repeat offline. Verify the resulting containers and record fresh console test limits. Source scripts remain an optional fallback, not a requirement for Windows users.
