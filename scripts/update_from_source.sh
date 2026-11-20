#!/usr/bin/env bash
# HelixEdit user-initiated source updater for native Ubuntu/Debian installs.
# Always build and test unprivileged; obtain system authorization only for installation.
set -Eeuo pipefail
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 /path/to/HelixEdit-source --check|--apply" >&2
  exit 2
fi
SOURCE="$(realpath -- "$1")"
MODE="$2"
case "$MODE" in --check|--apply) ;; *) echo "Unknown mode: $MODE" >&2; exit 2;; esac
if [[ "$(uname -s)" != Linux ]]; then
  echo "This updater currently supports Linux DEB installations only." >&2
  exit 2
fi
if [[ ! -f "$SOURCE/CMakeLists.txt" || ! -e "$SOURCE/.git" ]]; then
  echo "Not a HelixEdit Git checkout: $SOURCE" >&2
  echo "A ZIP download has no .git directory. Clone from GitHub once." >&2
  exit 2
fi
if [[ ! -f "$SOURCE/scripts/build_ubuntu_deb.sh" ]]; then
  echo "Missing the bundled native source build script." >&2
  exit 2
fi
mkdir -p "$SOURCE/build-logs"
if ! command -v git >/dev/null; then
  echo "Install git first: sudo apt install git" >&2;exit 2
fi
ACTUAL_TOP="$(git -C "$SOURCE" rev-parse --show-toplevel)"
if [[ "$(realpath -- "$ACTUAL_TOP")" != "$SOURCE" ]]; then
  echo "Choose the repository root, not a nested folder." >&2;exit 2
fi
if [[ "$(git -C "$SOURCE" symbolic-ref --quiet --short HEAD)" != main ]]; then
  echo "Switch to main before using automatic source updates." >&2;exit 2
fi
ORIGIN="$(git -C "$SOURCE" remote get-url origin)"
case "$ORIGIN" in
  https://github.com/mac860/HelixEdit|https://github.com/mac860/HelixEdit.git|\
  git@github.com:mac860/HelixEdit|git@github.com:mac860/HelixEdit.git|\
  ssh://git@github.com/mac860/HelixEdit|ssh://git@github.com/mac860/HelixEdit.git) ;;
  *) echo "Refusing updater from unexpected origin: $ORIGIN" >&2;exit 2;;
esac
if ! git -C "$SOURCE" diff --quiet || ! git -C "$SOURCE" diff --cached --quiet; then
  echo "Tracked source edits detected. Commit/stash first; no changes discarded." >&2
  exit 2
fi
echo "Checking upstream main at $ORIGIN"
git -C "$SOURCE" fetch --no-tags origin main
LOCAL="$(git -C "$SOURCE" rev-parse HEAD)"
REMOTE="$(git -C "$SOURCE" rev-parse refs/remotes/origin/main)"
if [[ "$LOCAL" == "$REMOTE" ]]; then
  echo "HelixEdit source already up to date: ${LOCAL:0:12}"
  if [[ "$MODE" == --check ]]; then
    exit 0
  fi
  echo "Rebuilding and reinstalling the current source; an installed binary may still be older."
else
  if ! git -C "$SOURCE" merge-base --is-ancestor "$LOCAL" "$REMOTE"; then
    echo "Local and remote histories diverged. Manual review required; no reset." >&2;exit 2
  fi
  CHANGES="$(git -C "$SOURCE" rev-list --count "$LOCAL..$REMOTE")"
  echo "Update available: $CHANGES commits (${LOCAL:0:12} -> ${REMOTE:0:12})"
  git -C "$SOURCE" log --max-count=25 --no-decorate --format='%h %s' "$LOCAL..$REMOTE"
  if [[ "$MODE" == --check ]]; then
    echo "Check complete. Select 'Update & install' to build, test and install."
    exit 0
  fi
  echo "Fast-forwarding source (local files will not be reset)..."
  git -C "$SOURCE" merge --ff-only "$REMOTE"
fi
echo "Building and testing as user $(id -un)..."
bash "$SOURCE/scripts/build_ubuntu_deb.sh"
VERSION="$(sed -n 's/^project(HelixEdit VERSION \([0-9.]*\) .*/\1/p' "$SOURCE/CMakeLists.txt" | head -1)"
ARCH="$(dpkg --print-architecture)"
MACHINE="$(uname -m)"
PACKAGE="$SOURCE/build-ubuntu-$ARCH/HelixEdit-$VERSION-$MACHINE.deb"
if [[ ! -f "$PACKAGE" ]]; then
  echo "Expected .deb was not produced: $PACKAGE" >&2;exit 1
fi
if [[ "$(dpkg-deb --field "$PACKAGE" Package)" != helixedit ||
      "$(dpkg-deb --field "$PACKAGE" Architecture)" != "$ARCH" ]]; then
  echo "Unexpected Debian package name or architecture; installation aborted." >&2;exit 1
fi
echo "Build and tests succeeded. Verified DEB: $PACKAGE"
INSTALL_HELPER="$SOURCE/scripts/install_ubuntu_deb.sh"
if [[ ! -f "$INSTALL_HELPER" ]]; then
  echo "Missing the safe local DEB installer in the source checkout." >&2
  echo "Update the source from main and retry." >&2
  exit 2
fi
if ! command -v pkexec >/dev/null; then
  echo "Graphical privilege helper pkexec not found." >&2
  echo "Install in a terminal with: bash '$INSTALL_HELPER' '$PACKAGE'" >&2
  exit 0
fi
echo "Staging the package outside private source directories so _apt can read it."
echo "Requesting Ubuntu system authentication only for apt installation."
if bash "$INSTALL_HELPER" "$PACKAGE" --pkexec; then
  echo "Upgrade successful. Restart HelixEdit to use the new version."
else
  echo "Installation cancelled or failed. Package retained at: $PACKAGE" >&2
  exit 1
fi
