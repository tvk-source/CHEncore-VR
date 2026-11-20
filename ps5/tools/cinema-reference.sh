#!/usr/bin/env bash
# Render the actual original game to a stereoscopic cinema reference locally.
# No PS5 access; no native firmware or headset changes.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
command -v glslangValidator >/dev/null || { echo "Install glslang-tools first"; exit 2; }
shader_dir="$root/build/cinema-reference-shaders"
mkdir -p "$shader_dir"
for stage in vert frag; do
    glslangValidator -V --target-env vulkan1.3 "ps5/vr/shaders/cinema.$stage" -o "$shader_dir/cinema.$stage.spv"
done
export ENCORE_CINEMA_SHADER_DIR="$shader_dir"
title_id=$(python3 -c 'import json; print(json.load(open("ps5/sce_sys/param.json"))["titleId"])')
if [[ ! -d "$root/dist/$title_id/assets" && -d "$root/dist-vr-probe/$title_id/assets" ]]; then
    export PS5_HOST_ASSET_TITLE="$root/dist-vr-probe/$title_id"
fi
export FRAMES=${FRAMES:-120}
exec bash ps5/tools/host-reference.sh "${@:-cloneheronative}"
