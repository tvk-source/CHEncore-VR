#!/usr/bin/env bash
# One pull/test/build command; never deploy automatically.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
[[ $(git branch --show-current) == vr-development ]] || { echo "Switch to vr-development first"; exit 2; }
git pull --ff-only origin vr-development
git log -1 --oneline
bash ps5/tools/test-vr-probe.sh
log="$HOME/encore-vr-batch-build.log"
ENCORE_VR_PROBE=1 bash ps5/tools/build.sh 2>&1 | tee "$log"
dir="$root/dist-vr-probe/PPSA98743"
test -s "$dir/eboot.bin" && test -s "$dir/sce_module/encore-engine.prx"
sha256sum "$dir/eboot.bin" "$dir/sce_module/encore-engine.prx"
printf '\nBuild complete: %s\nClose the PS5 title, then transfer both files using your working method.\n' "$dir"
