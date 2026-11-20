# Native PS5 PKG validation

The native update runtime now passes from installed PKG, folder and read-only
exFAT bootstraps. The installed PKG loaded the replacement engine, rendered,
exited normally and selected the same runtime on a fresh launch. The complete application PKG also ran its embedded engine/assets, and the full
worker installation and automatic restart passed from an installed PKG.

Tests used a small application-owned bootstrap, libc, parameter file and icon,
without embedded game assets or shaders. An independently verified full engine
was available in writable update storage. Automatic injection of `right.sprx`
was disabled. No imported entitlement modules or retail license files are part
of the proposed release.

| Writer/profile | Installation | Launch |
| --- | --- | --- |
| Packwright 1.3.0 | Rejected, `0x80F00101` | Not reached |
| LibProsperoPkg `748eabf1b7d17819528cabf367d8e27109d8fce3` | Passed | Image check rejected, `0x80F00800` |
| Same writer, PS5 application DRM header type 16 | Passed | Same image-check rejection |
| drakmor publisher/PPR branch `5e26c8ae65f4b3cf88dcc7c84226ea9718c38e5e`, type 16 | Passed | Image check passed; PPR superblock integrity rejected, `0x80020060` |
| Same publisher writer, upgradable application metadata | Passed | Same PPR integrity failure |
| Same branch, legacy profile | Host extent check failed | Not reached |
| Legacy profile with nested-writer cursor correction | Rejected, `0x80B21185` | Not reached |
| PS5 UltraPack 2.6.1 released Linux writer, supplied application-owned metadata module | Passed | Passed; updated engine rendered and reopened |

The native PPR failure is before executable entry:

```
verify_ppr_sblock_100: verify error -> EICV
verify_ppr_sblock_bg: error(96)
check_sblock: error=96
sceFsMountPprPkg: 0x80020060
```

The console owner confirms the existing older kstuff environment has launched
native PS5 PKGs. Its version was not available in the kernel log. This evidence
must not be presented as proof that kstuff or the console is at fault.

Read-only inspection of one installed application's structural metadata found
FIH format 3, PFS version 2, mode `0x0d`, 64-KiB blocks, CNT DRM type 16 and
content type `0x20`. It is a structural comparison, not a confirmed reproducer
of the particular working package. No payload, license/key entry or application
identity was needed for that comparison.

The production host tool `ps5/tools/build-native-pkg.py` pins the released Linux
`ffpfsc-pkg-tool` by SHA-256:

```
d6f7ed59effcb23ce37ae96205c7b8ed502858b412828ad76c5cc45780211988
```

Source: [PS5 UltraPack 2.6.1](https://github.com/knutwurst/ps5-ultrapack/releases/tag/v2.6.1).
The packaging program is used on the host and is not redistributed in Encore.
The application supplies its own small MIT-licensed metadata module, built from
`ps5/src/about_module.cpp`, rather than accepting the writer's default module.
The existing Vulkan/RADV startup, renderer and console payload setup are unchanged.

Build after `ps5/tools/build.sh`:

```sh
python3 ps5/tools/build-native-pkg.py dist/PPSA98743 output/Encore.pkg \
    --tool /path/to/ffpfsc-pkg-tool
```

The script checks the writer's identity, refuses userdata/test files, keeps the
source executable/parameters/module unchanged, and runs the writer's structural
validation before publishing the local artifact. These checks supplement the
console tests; they do not establish support on every firmware or launcher.
