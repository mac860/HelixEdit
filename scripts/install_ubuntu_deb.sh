#!/usr/bin/env bash
# Install a locally built HelixEdit DEB without exposing private source paths to
# the APT sandbox user. Build/test remain unprivileged; only apt is elevated.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE=""
MODE="--sudo"
usage() {
  echo "Usage: bash scripts/install_ubuntu_deb.sh [path/to/HelixEdit.deb] [--sudo|--pkexec]" >&2
  echo "Omit the path to install the current version from build-ubuntu-<arch>." >&2
}

for argument in "$@"; do
  case "$argument" in
    --sudo|--pkexec)
      MODE="$argument"
      ;;
    -*)
      usage; echo "Unknown option: $argument" >&2; exit 2
      ;;
    *)
      if [[ -n "$PACKAGE" ]]; then
        usage; exit 2
      fi
      PACKAGE="$argument"
      ;;
  esac
done

if [[ "$(uname -s)" != Linux ]] || ! command -v dpkg-deb >/dev/null 2>&1; then
  echo "Native Debian/Ubuntu with dpkg-deb is required." >&2
  exit 2
fi
if [[ $EUID -eq 0 ]]; then
  echo "Run this helper as your normal user (without sudo). Only apt is elevated." >&2
  exit 2
fi
ARCH="$(dpkg --print-architecture)"
if [[ -z "$PACKAGE" ]]; then
  VERSION="$(sed -n 's/^project(HelixEdit VERSION \([0-9.]*\) .*/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
  if [[ -z "$VERSION" ]]; then
    echo "Cannot determine HelixEdit version from CMakeLists.txt." >&2
    exit 2
  fi
  PACKAGE="$ROOT/build-ubuntu-$ARCH/HelixEdit-$VERSION-$(uname -m).deb"
fi
if [[ ! -f "$PACKAGE" || ! -r "$PACKAGE" ]]; then
  echo "DEB not found/readable: $PACKAGE" >&2
  echo "Build first with: bash scripts/build_ubuntu_deb.sh" >&2
  exit 2
fi
PACKAGE="$(realpath -- "$PACKAGE")"

# Refuse unrelated or wrong-architecture files before elevation.
DEB_NAME="$(dpkg-deb --field "$PACKAGE" Package)"
DEB_ARCH="$(dpkg-deb --field "$PACKAGE" Architecture)"
DEB_VERSION="$(dpkg-deb --field "$PACKAGE" Version)"
if [[ "$DEB_NAME" != "helixedit" || "$DEB_ARCH" != "$ARCH" || -z "$DEB_VERSION" ]]; then
  echo "Refusing unexpected package: $DEB_NAME $DEB_ARCH $DEB_VERSION" >&2
  exit 2
fi

# The _apt sandbox typically cannot traverse a private source tree in ~/Downloads.
# Do NOT chmod the home directory, project tree, or apt sandbox settings.
# A securely created /tmp directory, traversable by _apt, solves this warning.
STAGING_DIR="$(mktemp -d /tmp/helixedit-apt.XXXXXXXX)"
cleanup() { rm -rf -- "$STAGING_DIR"; }
trap cleanup EXIT
chmod 0755 "$STAGING_DIR"
STAGED="$STAGING_DIR/HelixEdit-$DEB_VERSION-$ARCH.deb"
install -m 0644 -- "$PACKAGE" "$STAGED"

echo "Package: helixedit $DEB_VERSION ($ARCH)"
echo "Temporary APT-readable package: $STAGED"
echo "Private source directory permissions are left unchanged."

if [[ "$MODE" == "--pkexec" ]]; then
  if ! command -v pkexec >/dev/null 2>&1; then
    echo "pkexec is not available. Use --sudo in a terminal." >&2
    exit 2
  fi
  pkexec /usr/bin/apt-get install --reinstall -y -- "$STAGED"
else
  if ! command -v sudo >/dev/null 2>&1; then
    echo "sudo is unavailable. Install through your distribution's administrator tools." >&2
    exit 2
  fi
  sudo /usr/bin/apt-get install --reinstall -- "$STAGED"
fi
echo "HelixEdit $DEB_VERSION installed; restart the app to use this build."
