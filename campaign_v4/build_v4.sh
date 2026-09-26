#!/bin/bash
# Build CRY (if needed) and the v4 `gc4` binary.
#
#   GC_ENV=../env_nuccomp1.sh BUILD_JOBS=8 campaign_v4/build_v4.sh
#
# Run from ground-carbon/. GC_ENV must set up Geant4 (source geant4.sh) and
# export CRYHOME (see env_nuccomp1.sh.example). One binary serves every soil:
# compositions are read at run time, so nothing is rebuilt per composition.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/.." && pwd)"
GC_ENV="${GC_ENV:-$SRC/env_nuccomp1.sh}"
BUILD_JOBS="${BUILD_JOBS:-4}"

echo "=== environment: $GC_ENV"
# shellcheck disable=SC1090
. "$GC_ENV"
command -v geant4-config >/dev/null || { echo "PROBLEM: geant4-config not on PATH"; exit 2; }
echo "Geant4 $(geant4-config --version) at $(geant4-config --prefix)"
[ -n "${CRYHOME:-}" ] && [ -d "$CRYHOME/src" ] || { echo "PROBLEM: CRYHOME not set or has no src/"; exit 2; }

if [ ! -f "$CRYHOME/lib/libCRY.a" ]; then
    echo "=== building CRY in $CRYHOME"
    mkdir -p "$CRYHOME/lib"
    (cd "$CRYHOME" && make > "$SRC/cry_build.log" 2>&1) || { echo "PROBLEM: CRY build failed, see cry_build.log"; exit 3; }
fi
ls -l "$CRYHOME/lib/libCRY.a"

echo "=== configuring build-v4"
mkdir -p "$SRC/build-v4"
cd "$SRC/build-v4"
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_GEANT4_UIVIS=OFF > cmake_v4.log 2>&1 || { echo "PROBLEM: cmake failed, see build-v4/cmake_v4.log"; tail -20 cmake_v4.log; exit 4; }
echo "=== compiling gc4"
make -j"$BUILD_JOBS" gc4 > make_v4.log 2>&1 || { echo "PROBLEM: compile failed, see build-v4/make_v4.log"; grep -E "error" make_v4.log | head -20; exit 5; }
ls -l "$SRC/build-v4/gc4"
"$SRC/build-v4/gc4" help | head -3
echo "=== build ok"
