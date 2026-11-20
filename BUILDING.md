# Building Clone Hero Encore

The native target builds from Linux or WSL using the vendored prospero-clang++ toolchain. CMake, Ninja, Python 3, Git and a host Clang installation are required. There is no GLFW dependency in the native title.

## Pinned dependency revisions

| Component | Revision |
| --- | --- |
| Title | Exact revision recorded by each release tag |
| PS5 Vulkan | `7f492b4c6f67a4e8c8c67a4272cfeef22abea9cc` |
| Payload SDK platform | `611893fc25ef3e138f5bc2c90d829d8b58174cd2` |
| RADV / Mesa | `1ba9874f478657ccf6da72ce55f0260164cc0bd0` |
| RADV SDK | `b83202be73e930050e00de0f1b4d9ec46c0391bf` |
| LLVM runtime SDK | ps5-payload-dev v0.42 |
| Opus | 1.5.2 |

Prepare [PS5 Vulkan](https://github.com/mihawk-99/PS5_Vulkan) and [the payload SDK fork](https://github.com/mihawk-99/PS5_PayloadSDK) at the revisions above, following their build instructions. The Vulkan checkout must supply the compiled RADV archive, `build/host/ps5-native-tool`, `runtime/libc.prx`, and its linking/packaging tools. Merely cloning the dependency repositories does not build those files.

The default RADV archive is `.deps/native/radv-release/lib/libvulkan_radeon.ps5.a` inside the Vulkan checkout. `RADV_ARCHIVE` can select another prepared archive. Source for all linked components accompanies the release; its `SOURCES.txt` and component notices record provenance.

## Build the title

From the title checkout, with the prepared dependencies in sibling directories:

```bash
PS5_VULKAN_DIR="$PWD/../PS5_Vulkan" \
PS5_PAYLOAD_SDK_FORK="$PWD/../PS5_PayloadSDK" \
bash ps5/tools/build.sh
```

`setup-sdk.sh` installs the pinned SDK into `.deps/native/ps5-payload-sdk`. The release build uses C++20 with exceptions and RTTI disabled and retains the native Vulkan dynamic-rendering and direct-to-display setup.

Output is written to `dist/PPSA98743`. Build intermediates are in `build/ps5`. A source archive extracted without Git metadata must first be initialized and committed locally for the notice generator; its local build identity will differ from the published build. A byte-identical rebuild is not guaranteed.

Deployment entry points are `ps5/tools/deploy.sh` and `ps5/tools/run.sh`. Console connection settings belong in the separately configured, ignored environment file used by the console tools. Do not commit console addresses or credentials.
