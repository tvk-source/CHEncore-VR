#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
pack=${ENCORE_ASSET_PACK:-$HOME/Downloads/EncoreAssets-v1.encore-assets}
[[ -s $pack ]] || { echo "Missing personal pack: $pack" >&2; exit 2; }
mkdir -p local-assets
if [[ $(realpath "$pack") != $(realpath -m local-assets/EncoreAssets-v1.encore-assets) ]]; then
    cp -- "$pack" local-assets/EncoreAssets-v1.encore-assets
fi
export ENCORE_ASSET_PACK="$root/local-assets/EncoreAssets-v1.encore-assets"
export PS5_DIST_ROOT="$root/dist-vr-cinema"
# One combined host and native build batch; stop before native on host failure.
bash ps5/tools/cinema-reference.sh
ENCORE_VR_PROBE=0 ENCORE_VR_CINEMA=1 bash ps5/tools/build.sh
app="$root/dist-vr-cinema/PPSA98743"
for item in eboot.bin sce_module/encore-engine.prx sce_sys/param.json shaders/cinema/cinema.vert.spv shaders/cinema/cinema.frag.spv assets/EncoreAssets-v1.encore-assets; do
    [[ -s "$app/$item" ]] || { echo "Missing alpha file: $item" >&2; exit 1; }
done
echo "Native alpha: $app"
echo "Do not deploy the personal assets to GitHub."
