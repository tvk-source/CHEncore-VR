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
# Verify pure, portable Sense-to-gamepad mappings without touching the console.
cc -std=c11 -DENCORE_VR_CINEMA=1 -O2 -Wall -Wextra -Werror -I"$root" tests/encore_cinema_pad_tests.c -lm -o /tmp/encore-cinema-pad-tests
/tmp/encore-cinema-pad-tests
# Prove world-space left/right eye disparity and chart geometry without a
# headset or GPU. Rendering proceeds only after this pure CPU test passes.
${CXX:-c++} -std=c++20 -O2 -Wall -Wextra -Werror -I"$root" tests/encore_stereo_highway_tests.cpp -o /tmp/encore-stereo-highway-tests
/tmp/encore-stereo-highway-tests
# One combined host and native build batch; stop before native on host failure.
bash ps5/tools/cinema-reference.sh
ENCORE_VR_PROBE=0 ENCORE_VR_CINEMA=1 bash ps5/tools/build.sh
app="$root/dist-vr-cinema/PPSA98743"
for item in eboot.bin sce_module/encore-engine.prx sce_sys/param.json shaders/cinema/cinema.vert.spv shaders/cinema/cinema.frag.spv shaders/cinema/highway3d.frag.spv shaders/cinema/atlas.vert.spv shaders/cinema/atlas.frag.spv assets/EncoreAssets-v1.encore-assets; do
    [[ -s "$app/$item" ]] || { echo "Missing alpha file: $item" >&2; exit 1; }
done
echo "Native alpha: $app"
echo "Do not deploy the personal assets to GitHub."
