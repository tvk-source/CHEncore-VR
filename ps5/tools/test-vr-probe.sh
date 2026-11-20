#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cd "$root"
for test in encore_vr_core_tests encore_ghost_runtime_tests encore_native_probe_tests encore_probe_raster_tests; do
    "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic "tests/$test.cpp" -o "$tmp/$test"
    "$tmp/$test"
done
# The Ghost backend declares console functions itself. Check C syntax only;
# the native SDK build is still necessary to verify linkage and ABI.
"${CC:-cc}" -std=gnu11 -fsyntax-only -DENCORE_VR_PROBE=1 ps5/vr/ghost/{psvr2,xr_log,gvr_stats}.c
echo "PASS GhostVR C syntax (not a native link)"

python3 tests/encore_asset_build_dir_tests.py

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_stereo_tests.c -lm -o "$tmp/encore_probe_stereo_tests"
"$tmp/encore_probe_stereo_tests"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_timing_tests.c -lm -o "$tmp/encore_probe_timing_tests"
"$tmp/encore_probe_timing_tests"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_world_tests.c -lm -o "$tmp/encore_probe_world_tests"
"$tmp/encore_probe_world_tests"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_highway_tests.c -lm -o "$tmp/encore_probe_highway_tests"
"$tmp/encore_probe_highway_tests"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_rhythm_tests.c -lm -o "$tmp/encore_probe_rhythm_tests"
"$tmp/encore_probe_rhythm_tests"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic tests/encore_probe_retirement_tests.c -o "$tmp/encore_probe_retirement_tests"
"$tmp/encore_probe_retirement_tests"
