#!/usr/bin/env bash
# PS5 Vulkan Template - build the title into dist/<TITLE_ID>/.
#
#   ps5/tools/build.sh
#
# Configures ps5/ with the payload SDK's CMake toolchain (once), compiles what
# changed, links with RADV and packages the title folder (tools/link-title.sh),
# then lays the assets in (tools/build-assets.py).
#
#   PS5_VULKAN_DIR   the PS5_Vulkan checkout (default ../PS5_Vulkan): the RADV
#                    release archive, the link recipe, the native tool and libc.prx
#
# The payload SDK is this title's own pin of my fork (tools/setup-sdk.sh).
#   PS5_CLANG        the host clang the compiler wrappers drive (default: clang)
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT

set -euo pipefail

ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
sdk=$(bash "$ps5/tools/setup-sdk.sh")
# Separate cache and staging directory; never replace the ordinary build.
probe=${ENCORE_VR_PROBE:-0}
cinema=${ENCORE_VR_CINEMA:-0}
case "$probe" in
    0|OFF) probe=OFF; build="$root/build/ps5"; dist_root=${PS5_DIST_ROOT:-$root/dist} ;;
    1|ON) probe=ON; build="$root/build/ps5-vr-probe"; dist_root=${PS5_DIST_ROOT:-$root/dist-vr-probe} ;;
    *) echo "ENCORE_VR_PROBE must be 0 or 1" >&2; exit 2 ;;
esac
if [[ $cinema == 1 || $cinema == ON ]]; then
    [[ $probe == OFF ]] || { echo "Select either probe or cinema" >&2; exit 2; }
    cinema=ON; build="$root/build/ps5-vr-cinema"; dist_root=${PS5_DIST_ROOT:-$root/dist-vr-cinema}
elif [[ $cinema == 0 || $cinema == OFF ]]; then cinema=OFF
else echo "ENCORE_VR_CINEMA must be 0 or 1" >&2; exit 2; fi
export PS5_DIST_ROOT="$dist_root"
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
export PS5_CLANG=${PS5_CLANG:-$(command -v clang || true)}

if [[ ! -f $build/build.ninja ]] || [[ $probe == ON || $cinema == ON ]]; then
    mkdir -p "$build"
    cmake -S "$ps5" -B "$build" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" \
        -DENCORE_CINEMA_EYE_PIXELS="${ENCORE_CINEMA_EYE_PIXELS:-1920}" -DENCORE_CINEMA_SOURCE_HEIGHT="${ENCORE_CINEMA_SOURCE_HEIGHT:-1440}" -DENCORE_VR_CINEMA="$cinema" -DENCORE_VR_PROBE="$probe" -DPS5_VULKAN_DIR="$vulkan" -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=OFF > "$build.configure.log" 2>&1 \
        || { cat "$build.configure.log" >&2; exit 1; }
fi
"$ninja" -C "$build" -j "${PS5_BUILD_JOBS:-2}" title
app="$dist_root/$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")"
python3 "$ps5/tools/build-assets.py" --build-dir "$build" --install "$app"
# Documentation-only commits do not relink the executable. Always refresh the
# source identity so release archives cannot retain an older/dirty revision.
python3 "$ps5/tools/stage-notices.py" "$app" "$vulkan" \
    "${RADV_ARCHIVE:-$vulkan/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a}"

if [[ $cinema == ON ]]; then
    mkdir -p "$app/shaders/cinema"
    for stage in vert frag; do
        glslangValidator -V --target-env vulkan1.3 "$ps5/vr/shaders/cinema.$stage" -o "$app/shaders/cinema/cinema.$stage.spv"
    done
    pack=${ENCORE_ASSET_PACK:-$root/local-assets/EncoreAssets-v1.encore-assets}
    [[ -s $pack ]] || { echo "Missing personal asset pack: $pack" >&2; exit 2; }
    cp -- "$pack" "$app/assets/EncoreAssets-v1.encore-assets"
    echo "Cinema alpha staged: $app (private personal assets included)"
fi
