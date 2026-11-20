#!/usr/bin/env bash
# PS5 Vulkan Template - the reference pictures: the title's code on this PC.
#
#   ps5/tools/host-reference.sh [all|menu|ID...]     FRAMES=300 by default
#   ps5/tools/host-reference.sh --save [samples]     also keep them as the references
#                                                     (ps5/reference/<id>.png, 480x270)
#
# Builds the title's code for Linux (PS5_HOST_REFERENCE: the same samples, base
# class, launcher and test runs, with a headless surface instead of the
# console's display and no pad), runs a test run on the PC's Vulkan driver, and
# leaves each sample's last frame in klog/host-<time>/<id>.png, at the size
# the console's screenshots have. A picture from another driver tells a RADV
# bug on the console from a sample or asset problem. Run ps5/tools/build.sh
# first: the assets come from the console build's title folder.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
build="$root/build/ps5-host"
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
asset_title="${PS5_HOST_ASSET_TITLE:-$root/dist/$title_id}"
[[ -d $asset_title/assets ]] || { echo "no $asset_title/assets: run ps5/tools/build.sh first" >&2; exit 2; }

# libc++ 18.1.8, as on the console (the payload SDK's is 18.1): the samples'
# random scenes come from std:: distributions, which libstdc++ computes otherwise.
# conda-forge's packages, at pinned hashes, unpacked in the build folder.
libcxx="$build/libcxx/root"
if [[ ! -f $libcxx/lib/libc++abi.so ]]; then
    mkdir -p "$build/libcxx/dl" "$libcxx"
    for package in "libcxx 18.1.8 libcxx-18.1.8-h719d109_8.conda 382b4d8208ba5d207a85697630effc92616d883ed7bff2b6b6b0ac686c3326b3" \
            "libcxx-devel 18.1.8 libcxx-devel-18.1.8-h2c0bd0d_8.conda b9a6477ddd8f1dd388f2747a98847a61585a06a1eda0612eb30bc8e1dcda14be" \
            "libcxxabi 18.1.8 libcxxabi-18.1.8-hc655929_8.conda 655f5b2e69cc7995e6d0e9d96da8d677ef71c56cdeedc65473d80da076a86aaa"; do
        read -r name version file sha <<< "$package"
        curl -sfL "https://api.anaconda.org/download/conda-forge/$name/$version/linux-64/$file" -o "$build/libcxx/dl/$file"
        echo "$sha  $build/libcxx/dl/$file" | sha256sum -c --quiet
        python3 - "$build/libcxx/dl/$file" "$libcxx" <<'PY'
import subprocess, sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as conda:
    for member in conda.namelist():
        if member.startswith("pkg-") and member.endswith(".tar.zst"):
            data = subprocess.run(["zstd", "-qdc"], input=conda.read(member), capture_output=True, check=True).stdout
            subprocess.run(["tar", "-x", "-C", sys.argv[2]], input=data, check=True)
PY
    done
fi
if [[ ! -f $build/build.ninja ]]; then
    mkdir -p "$build"
    cmake -S "$ps5" -B "$build" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DPS5_HOST_REFERENCE=ON \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_CXX_FLAGS="-stdlib=libc++ -nostdinc++ -isystem $libcxx/include/c++/v1" \
        -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -L$libcxx/lib -Wl,-rpath,$libcxx/lib" \
        -DPS5_HOST_APP_ROOT="$build/app" > "$build.configure.log" 2>&1 || { cat "$build.configure.log" >&2; exit 1; }
fi
# Retain the full compiler output; the former tail -1 hid the actual error.
build_log="$build/build.log"
jobs="${PS5_HOST_JOBS:-2}"
if [[ ! $jobs =~ ^[1-9][0-9]*$ ]]; then
    echo "PS5_HOST_JOBS must be a positive integer" >&2
    exit 2
fi
if ! "$ninja" -C "$build" -j "$jobs" ps5-samples-host 2>&1 | tee "$build_log"; then
    echo "Host build failed; full compiler output: $build_log" >&2
    exit 1
fi
mkdir -p "$build/app"
ln -sfn "$asset_title/assets" "$build/app/assets"
ln -sfn "$root/shaders" "$build/app/shaders"

save=false
if [[ ${1:-} == --save ]]; then
    save=true
    shift
    [[ ${FRAMES:-300} == 300 ]] || { echo "references are made at 300 frames, the test runs' default" >&2; exit 2; }
fi
(( $# )) || set -- all
mapfile -t samples < <(python3 "$ps5/tools/check-run.py" --resolve "$@")
if [[ -n ${ENCORE_CINEMA_SHADER_DIR:-} ]]; then
    for sample in "${samples[@]}"; do rm -f "$build/app/screenshots/$sample.ppm.cinema.ppm"; done
fi
printf 'frames %s\nscreenshot\nsamples %s\n' "${FRAMES:-300}" "${samples[*]}" > "$build/app/test-run.txt"
out="$root/klog/host-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$out"
status=0
"$build/ps5-samples-host" 2>&1 | tee "$out/log.txt" | grep -E "\] (sample|samples|test run|ends)|CINEMA_REFERENCE|Fatal|rror" || true
grep -q "ends: status 0" "$out/log.txt" || status=1
if [[ -n ${ENCORE_CINEMA_SHADER_DIR:-} ]]; then
    grep -q "CINEMA_REFERENCE PASS" "$out/log.txt" || status=1
    if grep -q "CINEMA_REFERENCE FAIL" "$out/log.txt"; then status=1; fi
fi
for sample in "${samples[@]}"; do
    ppm="$build/app/screenshots/$sample.ppm"
    [[ -f $ppm ]] || continue
    if [[ -f $ppm.cinema.ppm ]]; then
        python3 -c 'import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[2])' "$ppm.cinema.ppm" "$out/$sample.cinema.png"
    fi
    python3 -c 'import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[2])' "$ppm" "$out/$sample.png"
    if $save; then
        mkdir -p "$ps5/reference"
        python3 -c 'import sys; from PIL import Image; Image.open(sys.argv[1]).convert("RGB").resize((480, 270), Image.LANCZOS).save(sys.argv[2], optimize=True)' \
            "$ppm" "$ps5/reference/$sample.png"
    fi
done
if $save; then echo "references: $ps5/reference/"; fi
echo "pictures: $out/*.png"
exit $status
