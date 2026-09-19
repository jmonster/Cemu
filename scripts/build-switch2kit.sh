#!/bin/bash
# Build the checked-out revision and its pinned controller library.
set -euo pipefail
cd "$(dirname "$0")/.."
platform=$(uname -s)
options=(-DCMAKE_BUILD_TYPE=Release -DENABLE_SWITCH2KIT=ON -DENABLE_SDL=ON -DENABLE_VULKAN=ON)
case "$platform" in
  Darwin)
    if [ "$(sysctl -in sysctl.proc_translated 2>/dev/null || true)" = 1 ]; then
      echo "Use a native Terminal, not Rosetta, on Apple silicon." >&2; exit 1
    fi
    command -v xcrun >/dev/null || { echo 'Install and select Xcode 26+.' >&2; exit 1; }
    xcrun swift --version
    options+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 "-DCMAKE_OSX_ARCHITECTURES=$(uname -m)" -DMACOS_BUNDLE=ON)
    ;;
  Linux)
    command -v swift >/dev/null || { echo 'Install the Swift 6.2+ toolchain.' >&2; exit 1; }
    swift --version
    options+=(-DMACOS_BUNDLE=OFF "-DCMAKE_INSTALL_PREFIX=$PWD/build-switch2kit/install")
    ;;
  *) echo 'Use macOS or Linux with this helper; use the PowerShell helper on Windows.' >&2; exit 1 ;;
esac
for tool in cmake ninja git; do command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }; done
export VCPKG_MAX_CONCURRENCY="${VCPKG_MAX_CONCURRENCY:-3}"
git submodule update --init --recursive
if [ ! -x dependencies/vcpkg/vcpkg ]; then bash dependencies/vcpkg/bootstrap-vcpkg.sh; fi
cmake -S . -B build-switch2kit -G Ninja "${options[@]}"
cmake --build build-switch2kit --target CemuBin --parallel "${S2K_BUILD_JOBS:-3}"
if [ "$platform" = Darwin ]; then
  echo "Built: $PWD/bin/Cemu_release.app"
  if [ "${1:-}" = --run ]; then open "$PWD/bin/Cemu_release.app"; fi
else
  cmake --install build-switch2kit
  echo "Built: $PWD/build-switch2kit/install/bin/Cemu_release"
  if [ "${1:-}" = --run ]; then exec "$PWD/build-switch2kit/install/bin/Cemu_release"; fi
fi
