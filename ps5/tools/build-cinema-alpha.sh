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
for item in eboot.bin sce_module/encore-engine.prx sce_sys/param.json shaders/cinema/cinema.vert.spv shaders/cinema/cinema.frag.spv shaders/cinema/atlas.vert.spv shaders/cinema/atlas.frag.spv assets/EncoreAssets-v1.encore-assets; do
    [[ -s "$app/$item" ]] || { echo "Missing alpha file: $item" >&2; exit 1; }
done
# Confirm that the native link genuinely contains the current Alpha 5
# renderer. The old Alpha 4 binary also has VR_STEREO logs, so merely seeing
# that prefix in klog is insufficient evidence that deployment was updated.
native_elf="$root/build/ps5-vr-cinema/link/llvm-pie.elf"
[[ -s "$native_elf" ]] || { echo "Missing linked native engine: $native_elf" >&2; exit 1; }
if ! LC_ALL=C grep -aqF 'VR_BUILD_ALPHA6_STEREO_HUD' "$native_elf"; then
    echo "ERROR: native ELF lacks the Alpha 5 identity marker. Refusing stale deployment." >&2
    exit 1
fi
printf '\n==> Alpha 5 source revision: %s\n' "$(git rev-parse --short=12 HEAD)"
echo "==> Native Alpha 5 identity marker: PASS"
echo "==> Staged files to deploy together:"
sha256sum "$app/eboot.bin" "$app/sce_module/encore-engine.prx" \
    "$app/shaders/cinema/atlas.vert.spv" "$app/shaders/cinema/atlas.frag.spv"
echo "Native alpha: $app"
echo "Deploy the ENTIRE directory above; do not use dist/PPSA98743 or an older PKG."
echo "For a homebrew-folder install, verify the deployed hashes with:"
echo "  python3 ps5/tools/verify-cinema-deploy.py"
echo "Do not deploy the personal assets to GitHub."
