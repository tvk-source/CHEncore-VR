#!/usr/bin/env bash
# PS5 Vulkan Template - link the title and package its folder.
#
#   link-title.sh BUILD_DIR PS5_VULKAN_DIR SDK OBJECT...
#
# Run by the build (ps5/CMakeLists.txt) once the objects are compiled. The link
# is PS5_Vulkan's: its RADV release archive (RADV, ACO, NIR and Mesa's runtime,
# built from PS5_Mesa), its link recipe (tools/radv-link.sh: the platform layer
# of this title's SDK pin, the heap and thread wraps, the libc names bound to ps5_*),
# its CRT, its native tool (ELF -> the console's fake SELF) and its libc.prx,
# as its CTS title links them. The title folder, dist/<TITLE_ID>/, gets the
# signed eboot.bin, sce_sys/, the shaders of the samples linked in and the
# assets (tools/build-assets.py).
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT

set -euo pipefail
[[ -n ${LINK_TRACE:-} ]] && set -x

work=$1 vulkan=$2 sdk=$3
shift 3
objects=("$@")
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
archive=${RADV_ARCHIVE:-$vulkan/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a}
export PS5_CLANG=${PS5_CLANG:-$(command -v clang || true)}
tool="$vulkan/build/host/ps5-native-tool"
native="$vulkan/tooling/native"
param="$ps5/sce_sys/param.json"

for file in "$archive" "$tool" "$param" "$sdk/bin/prospero-lld" "$vulkan/tools/radv-link.sh" \
        "$vulkan/runtime/libc.prx" "$work/samples.txt"; do
    [[ -e $file ]] || { echo "missing: $file" >&2; exit 2; }
done
mkdir -p "$work/link/obj" "$work/link/stubs"
cc() { PS5_PAYLOAD_SDK="$sdk" sh "$vulkan/tooling/prospero-clang18" "$@"; }

# Song audio's Opus decoder is statically linked from the pinned PacBrew
# prefix. Encore network requests use the native PS5 HTTP/2 imports.
pacbrew_json=$(bash "$vulkan/tools/setup-pacbrew-dependencies.sh" --resolve opus)
mapfile -d '' -t pacbrew_link_args < <(python3 -c \
	'import json,sys; [print(v, end="\0") for v in json.loads(sys.argv[1])["libs"] if v != "-lz"]' \
    "$pacbrew_json")

# The CRT. The C++ runtime is libc++abi's (the samples throw and catch), as in
# PS5_Vulkan's CTS title.
cc -std=c++20 -O2 -fno-exceptions -fno-rtti -c "$native/app_crt.cpp" -o "$work/link/obj/app_crt.o"
# RADV calls AGC, which the SDK has no stubs for: these name its imports.
stub() {
    local library=$1 source=$2
    cc -std=c11 -O2 -fPIC -c "$vulkan/$source" -o "$work/link/obj/${library}_stub.o"
    "$sdk/bin/prospero-lld" --shared -soname "${library}.prx" \
        -o "$work/link/stubs/${library}.so" "$work/link/obj/${library}_stub.o"
}
stub libSceAgc vendor/ps5/sdk/stubs/agc_canary_link_stub.c
stub libSceAgcDriver vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c
cc -std=c11 -O2 -fPIC -c "$ps5/src/common_dialog_link_stub.c" -o "$work/link/obj/libSceCommonDialog_stub.o"
"$sdk/bin/prospero-lld" --shared -soname libSceCommonDialog.sprx \
	-o "$work/link/stubs/libSceCommonDialog.so" "$work/link/obj/libSceCommonDialog_stub.o"
cc -std=c11 -O2 -fPIC -c "$ps5/src/ime_link_stub.c" -o "$work/link/obj/libSceIme_stub.o"
"$sdk/bin/prospero-lld" --shared -soname libSceIme.sprx \
	-o "$work/link/stubs/libSceIme.so" "$work/link/obj/libSceIme_stub.o"

# shellcheck source=/dev/null
source "$vulkan/tools/radv-link.sh"
radv_link_recipe "$vulkan" "$sdk" "$archive" || exit 2
# localeconv in the C locale, '.' the decimal point (the console's gives none, and
# tinygltf's JSON parser then read 0.62 as 0): the platform layer's since SDK fork
# 6b63a2a (tools/setup-sdk.sh's pin is never older), bound by PS5_Vulkan's recipe
# since its 6a6dfa6. With an older recipe the title binds it here, kept local as
# the recipe keeps its bound names. The symbol listing is read whole before it
# is searched: piped into grep -q under pipefail, llvm-nm's SIGPIPE made the
# found symbol a miss about half the time once the archive grew (SDK 611893f).
platform_has_localeconv=false
grep -q " T ps5_localeconv$" <<<"$("$sdk/bin/llvm-nm" --defined-only "$sdk/target/lib/libps5platform.a" 2>/dev/null)" &&
    platform_has_localeconv=true
if $platform_has_localeconv && [[ " ${radv_link_flags[*]} " != *" --defsym=localeconv=ps5_localeconv "* ]]; then
    printf '{\n    local:\n        localeconv;\n};\n' > "$work/link/localeconv-local.map"
    radv_link_flags+=(--defsym=localeconv=ps5_localeconv --version-script "$work/link/localeconv-local.map")
fi
# Both entry paths use the same PIC game/driver objects. The embedded engine
# remains the fallback; an update loads the shared engine before Vulkan starts.
cc -std=c++20 -O2 -fPIC -fno-exceptions -fno-rtti -c "$ps5/src/runtime_module_crt.cpp" -o "$work/link/obj/runtime_module_crt.o"
printf '{ local: *; };\n' > "$work/link/runtime-local.map"
link_engine() {
    local crt=$1 visibility=$2 output=$3
    shift 3
    "$sdk/bin/prospero-lld" "$@" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" \
        --version-script "$visibility" --exclude-libs=ALL \
        -e _start -o "$output" "$crt" "${objects[@]}" \
        "${pacbrew_link_args[@]}" \
        "$work/link/stubs/libSceAgc.so" "$work/link/stubs/libSceAgcDriver.so" \
        "$work/link/stubs/libSceCommonDialog.so" "$work/link/stubs/libSceIme.so" \
        "${radv_link_inputs[@]}" --as-needed "$sdk"/target/lib/*.so
}
link_engine "$work/link/obj/app_crt.o" "$native/app-symbols.map" "$work/link/llvm-pie.elf"
link_engine "$work/link/obj/runtime_module_crt.o" "$work/link/runtime-local.map" "$work/link/engine-pic.elf" --shared -Bsymbolic
cc -std=c++20 -O2 -fPIC -fno-exceptions -fno-rtti -c "$ps5/src/about_module.cpp" -o "$work/link/obj/about_module.o"
"$sdk/bin/prospero-lld" --shared -Bsymbolic -e _start --version-script "$work/link/runtime-local.map" \
    -o "$work/link/about-pic.elf" "$work/link/obj/about_module.o"
"$tool" link --in "$work/link/about-pic.elf" --out "$work/link/about-native.elf" \
    --stub-dir "$sdk/target/lib" --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name encore-about.prx
# A title whose localeconv is still the console's reads every glTF fraction as 0
# (black materials, models scaled to nothing): refuse it here, not on the console
if $platform_has_localeconv &&
        grep -q " U localeconv$" <<<"$("$sdk/bin/llvm-nm" "$work/link/llvm-pie.elf" 2>/dev/null)"; then
    echo "link-title.sh: localeconv is not bound to the platform layer's ps5_localeconv" >&2
    exit 1
fi
# A title loads neither libkernel_sys's exports nor libScePosixForWebKit's: an
# import only their stubs define links, and is null at run time, so its first call
# jumps to address 0 (PS5_RetroArch's strcasestr did; RADV imports readlink and
# mkstemp). PS5_Vulkan's recipe binds those the platform layer has: refuse a
# title that still imports one, here rather than on the console.
null_imports=$(comm -23 \
    <("$sdk/bin/llvm-nm" -D --undefined-only "$work/link/llvm-pie.elf" |
        awk '$1 == "U" { sub(/@.*/, "", $2); print $2 }' | sort -u) \
    <(for library in "$sdk"/target/lib/*.so "$work/link/stubs/libSceAgc.so" "$work/link/stubs/libSceAgcDriver.so" "$work/link/stubs/libSceCommonDialog.so" "$work/link/stubs/libSceIme.so"; do
        case ${library##*/} in libkernel_sys.so | libScePosixForWebKit.so) continue ;; esac
        "$sdk/bin/llvm-nm" -D --defined-only "$library" 2>/dev/null | awk '{ print $NF }'
    done | sort -u))
if [[ -n $null_imports ]]; then
    echo "link-title.sh: imports that no module a title loads exports (null at run time): ${null_imports//$'\n'/ }" >&2
    echo "link-title.sh: bind them to the platform layer (PS5_Vulkan's tools/radv-link.sh)" >&2
    exit 1
fi
"$tool" link --in "$work/link/llvm-pie.elf" --out "$work/eboot.elf.new" \
    --stub-dir "$sdk/target/lib" --stub "$work/link/stubs/libSceAgc.so" \
    --stub "$work/link/stubs/libSceAgcDriver.so" --stub "$work/link/stubs/libSceCommonDialog.so" \
    --stub "$work/link/stubs/libSceIme.so" \
    --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name eboot.elf

"$tool" link --in "$work/link/engine-pic.elf" --out "$work/link/engine-native.elf" \
    --stub-dir "$sdk/target/lib" --stub "$work/link/stubs/libSceAgc.so" \
    --stub "$work/link/stubs/libSceAgcDriver.so" --stub "$work/link/stubs/libSceCommonDialog.so" \
    --stub "$work/link/stubs/libSceIme.so" --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name encore-engine.prx

# The title folder
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$param")
if [[ -f $work/encore-vr-cinema.txt ]]; then
    dist_root=${PS5_DIST_ROOT:-$root/dist-vr-cinema}
elif [[ -f $work/encore-vr-probe.txt ]]; then
    dist_root=${PS5_DIST_ROOT:-$root/dist-vr-probe}
else
    dist_root=${PS5_DIST_ROOT:-$root/dist}
fi
app="$dist_root/$title_id"
mkdir -p "$app/sce_sys/about" "$app/sce_module" "$app/shaders/glsl"
python3 "$ps5/tools/convert-runtime-module.py" "$work/link/about-native.elf" "$app/sce_sys/about/right.sprx" --tool "$tool"
python3 "$ps5/tools/convert-runtime-module.py" "$work/link/engine-native.elf" "$app/sce_module/encore-engine.prx" --tool "$tool"
"$tool" self --sign --in "$work/eboot.elf.new" --out "$app/eboot.bin" --magic 0x1D3D154F
"$tool" self --inspect --file "$app/eboot.bin" > /dev/null
cp "$param" "$app/sce_sys/param.json"
if [[ -f $work/encore-vr-probe.txt || -f $work/encore-vr-cinema.txt ]]; then
    # Native VR title bits from GhostVR make_title.py; no hand-tracking bit.
    python3 - "$app/sce_sys/param.json" "$work" <<'PY'
import json, sys
from pathlib import Path
p = Path(sys.argv[1])
d = json.loads(p.read_text())
d["attribute"] = 0x62000000
d["attribute3"] |= 0x400 | 0x800
d["localizedParameters"]["en-US"]["titleName"] = "CH: Encore VR Cinema Alpha" if (Path(sys.argv[2])/"encore-vr-cinema.txt").exists() else "CH: Encore VR Probe"
p.write_text(json.dumps(d, indent=2) + "\n")
PY
fi
for asset in icon0.png pic0.dds pic1.dds snd0.at9; do
    if [[ -f $ps5/assets/ch-branding/$asset ]]; then cp "$ps5/assets/ch-branding/$asset" "$app/sce_sys/$asset"; continue; fi
    [[ -f $ps5/sce_sys/$asset ]] && cp "$ps5/sce_sys/$asset" "$app/sce_sys/$asset"
done
(cd "$vulkan/runtime" && sha256sum --check --strict --quiet libc.prx.sha256)
cp "$vulkan/runtime/libc.prx" "$app/sce_module/libc.prx"
# The precompiled SPIR-V of the base class and of each sample linked in
# (CMake writes the list without a final newline: read then reports end of file).
# A sample that draws only with the UI kit has none: the kit carries its own.
IFS=';' read -r -a samples < "$work/samples.txt" || (( ${#samples[@]} ))
rm -rf "$app/shaders/glsl"
mkdir -p "$app/shaders/glsl"
for dir in base "${samples[@]}"; do
    compgen -G "$root/shaders/glsl/$dir/*.spv" > /dev/null || continue
    mkdir -p "$app/shaders/glsl/$dir"
    cp "$root/shaders/glsl/$dir/"*.spv "$app/shaders/glsl/$dir/"
done
mv "$work/eboot.elf.new" "$work/eboot.elf"
archive_revision=$(sed -n 's/^revision: //p' "$(dirname "$(dirname "$archive")")/PROVENANCE.txt" 2>/dev/null || true)
printf '==> %s: %s (eboot.bin %s bytes; %d samples; RADV %s)\n' "$title_id" "$app" \
    "$(stat -c %s "$app/eboot.bin")" "${#samples[@]}" "${archive_revision:0:11}"
# The notices beside eboot.bin: LEGAL.txt and licenses/, each part with its revision
python3 "$ps5/tools/stage-notices.py" "$app" "$vulkan" "$archive"
