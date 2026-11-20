#!/usr/bin/env bash
# Non-privileged smoke test: intercept sudo and inspect the DEB staging area.
# No package is installed and no administrator authentication is requested.
set -Eeuo pipefail
if [[ $# -ne 1 || ! -f "$1" ]]; then
  echo "Usage: bash scripts/test_install_staging.sh /path/to/HelixEdit.deb" >&2
  exit 2
fi
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE="$(realpath -- "$1")"
MOCK_DIR="$(mktemp -d /tmp/helixedit-install-test.XXXXXXXX)"
cleanup() { rm -rf -- "$MOCK_DIR"; }
trap cleanup EXIT
mkdir -p "$MOCK_DIR/bin"
cat > "$MOCK_DIR/bin/sudo" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
if [[ $# -lt 5 || "$1" != "/usr/bin/apt-get" ||
      "$2" != "install" || "$3" != "--reinstall" || "$4" != "--" ]]; then
  echo "Installer did not invoke apt-get with expected arguments." >&2
  exit 1
fi
DEB="${!#}"
if [[ ! -f "$DEB" ]]; then
  echo "Staged package was not found." >&2
  exit 1
fi
if [[ "$(stat -c '%a' -- "$DEB")" != 644 ||
      "$(stat -c '%a' -- "$(dirname -- "$DEB")")" != 755 ]]; then
  echo "Temporary staging permissions are not sandbox-readable." >&2
  exit 1
fi
if [[ "$(dpkg-deb --field "$DEB" Package)" != helixedit ]]; then
  echo "Staged package has an unexpected identity." >&2
  exit 1
fi
echo "PASS: apt command intercepted, package identity checked, staging 0755/0644."
MOCK
chmod 0755 "$MOCK_DIR/bin/sudo"
OUTPUT="$(PATH="$MOCK_DIR/bin:$PATH" bash "$ROOT/scripts/install_ubuntu_deb.sh" "$PACKAGE" --sudo)"
printf '%s\n' "$OUTPUT"
grep -q 'PASS: apt command intercepted' <<< "$OUTPUT"
STAGED="$(sed -n 's/^Temporary APT-readable package: //p' <<< "$OUTPUT")"
if [[ -z "$STAGED" || -e "$STAGED" ]]; then
  echo "Installer did not delete its temporary staging package." >&2
  exit 1
fi
echo "PASS: package staging is cleaned after installation completes."
