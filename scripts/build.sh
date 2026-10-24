#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CONFIG="${CONFIG:-Release}"
GENERATOR="${GENERATOR:-Unix Makefiles}"

"$ROOT/scripts/setup-linux.sh" >/dev/null

# Rootless Ubuntu dev headers and linker stubs live in the offline sysroot.
if [ -d "$ROOT/deps/sysroot/usr" ]; then
    export PKG_CONFIG_PATH="$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$ROOT/deps/sysroot/usr/share/pkgconfig:${PKG_CONFIG_PATH:-}"
    export PKG_CONFIG_SYSROOT_DIR="$ROOT/deps/sysroot"
    CMAKE_EXTRA_ARGS=(
        "-DCMAKE_INCLUDE_PATH=$ROOT/deps/sysroot/usr/include"
        "-DCMAKE_LIBRARY_PATH=$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu"
    )
else
    CMAKE_EXTRA_ARGS=()
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DMIDSIDE670_BUILD_TESTS=ON \
    -DMIDSIDE670_BUILD_SCREENSHOT=ON \
    -DMIDSIDE670_BUILD_HOST_TEST=ON \
    "${CMAKE_EXTRA_ARGS[@]}"

if [ -z "${JOBS:-}" ]; then
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$JOBS"

VST3="$BUILD_DIR/MidSide670_artefacts/$CONFIG/VST3/MidSide-670.vst3"
STANDALONE="$BUILD_DIR/MidSide670_artefacts/$CONFIG/Standalone/MidSide-670"

mkdir -p "$ROOT/dist"
if [ -d "$VST3" ]; then
    rm -rf "$ROOT/dist/MidSide-670.vst3"
    cp -a "$VST3" "$ROOT/dist/"
fi
if [ -x "$STANDALONE" ]; then
    cp -a "$STANDALONE" "$ROOT/dist/MidSide-670"
fi

echo
echo "Built artefacts:"
[ -d "$ROOT/dist/MidSide-670.vst3" ] && echo "  VST3        $ROOT/dist/MidSide-670.vst3"
[ -x "$ROOT/dist/MidSide-670" ] && echo "  Standalone  $ROOT/dist/MidSide-670"
echo "  Tests       $BUILD_DIR/MidSide670Tests_artefacts/$CONFIG/MidSide670Tests"
if [ -x "$BUILD_DIR/MidSide670HostSmokeTest_artefacts/$CONFIG/MidSide670HostSmokeTest" ]; then
    echo "  Host test   $BUILD_DIR/MidSide670HostSmokeTest_artefacts/$CONFIG/MidSide670HostSmokeTest"
fi
if [ -x "$BUILD_DIR/MidSide670Screenshot_artefacts/$CONFIG/MidSide-670 Screenshot" ]; then
    echo "  Screenshot  $BUILD_DIR/MidSide670Screenshot_artefacts/$CONFIG/MidSide-670 Screenshot"
fi
