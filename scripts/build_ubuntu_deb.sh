#!/usr/bin/env bash
# HelixEdit native Ubuntu/Debian build and .deb packaging
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
if [[ "$(uname -s)" != Linux ]]; then
  echo "This script is intended for Linux; see README for macOS." >&2
  exit 2
fi
if ! command -v cmake >/dev/null || ! command -v cpack >/dev/null; then
  echo "CMake/CPack missing. Install: sudo apt install cmake" >&2
  exit 2
fi
if ! command -v pkg-config >/dev/null || ! pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6Svg; then
  echo "Qt6 development dependencies missing." >&2
  echo "sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev qt6-svg-dev dpkg-dev" >&2
  exit 2
fi
if ! command -v dpkg-deb >/dev/null; then
  echo "dpkg-deb missing. Install: sudo apt install dpkg-dev" >&2
  exit 2
fi
ARCH="$(dpkg --print-architecture)"
BUILD_DIR="${HELIX_BUILD_DIR:-$ROOT/build-ubuntu-$ARCH}"
LOG_DIR="$ROOT/build-logs"
mkdir -p "$LOG_DIR"
LOG="$LOG_DIR/build-$ARCH-$(date -u +%Y%m%dT%H%M%SZ).log"
echo "Build log: $LOG"
exec > >(tee -a "$LOG") 2>&1
trap 'echo "Build failed at line $LINENO (exit $?). See $LOG" >&2' ERR
echo "System: $(uname -a)"
echo "Architecture: $ARCH"
echo "CMake: $(cmake --version | head -1)"
echo "Qt6: $(pkg-config --modversion Qt6Core)"
echo "Source: $ROOT"
echo "Build: $BUILD_DIR"
echo "A fresh build directory avoids stale Qt/CMake architecture settings."
cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
  -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" --parallel "${HELIX_JOBS:-2}"
QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD_DIR" --output-on-failure
(
  cd "$BUILD_DIR"
  cpack -G DEB --verbose
)
echo "Generated packages:"
find "$BUILD_DIR" -maxdepth 1 -type f -name '*.deb' -print
shopt -s nullglob
packages=("$BUILD_DIR"/*.deb)
if (( ${#packages[@]} == 0 )); then
  echo "CPack reported success but no .deb was generated." >&2
  exit 1
fi
for package in "${packages[@]}"; do
  dpkg-deb --info "$package"
  dpkg-deb --contents "$package" | grep -E '/usr/(bin/helixedit|share/applications/org.helixedit.desktop)' || {
    echo "Package missing executable or desktop launcher: $package" >&2
    exit 1
  }
done
echo "Inspect with: dpkg-deb --info '$BUILD_DIR'/HelixEdit-*.deb"
VERSION="$(sed -n 's/^project(HelixEdit VERSION \([0-9.]*\) .*/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
EXPECTED_PACKAGE="$BUILD_DIR/HelixEdit-$VERSION-$(uname -m).deb"
if [[ -f "$EXPECTED_PACKAGE" ]]; then
  echo "Install with: sudo apt install --reinstall '$EXPECTED_PACKAGE'"
else
  echo "Install with: sudo apt install <exact-package-path-from-the-list-above>"
fi
