#!/bin/bash
# Build the checked-out revision and its pinned controller library.
set -euo pipefail
cd "$(dirname "$0")/.."
if [ "$(uname -s)" != Darwin ]; then echo "This build requires macOS 15 or newer." >&2; exit 1; fi
if [ "$(sysctl -in sysctl.proc_translated 2>/dev/null || true)" = 1 ]; then
  echo "Use a native Terminal, not Rosetta, on Apple silicon." >&2; exit 1
fi
for tool in cmake ninja git xcrun; do command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }; done
xcrun swift --version
export VCPKG_MAX_CONCURRENCY="${VCPKG_MAX_CONCURRENCY:-3}"
git submodule update --init --recursive
if [ ! -x dependencies/vcpkg/vcpkg ]; then bash dependencies/vcpkg/bootstrap-vcpkg.sh; fi
cmake -S . -B build-switch2kit -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
  -DENABLE_SWITCH2KIT=ON -DENABLE_SDL=ON -DMACOS_BUNDLE=ON -DENABLE_VULKAN=ON
cmake --build build-switch2kit --target CemuBin --parallel 3
echo "Built: $PWD/bin/Cemu_release.app"
if [ "${1:-}" = --run ]; then open "$PWD/bin/Cemu_release.app"; fi
