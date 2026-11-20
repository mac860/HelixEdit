#!/usr/bin/env bash
# Optional local MUSCLE 5.3 build for Linux ARM64. Not part of HelixEdit's runtime.
# The v5.3 source misses GCC's __aarch64__ macro in its pointer-width detection.
# See upstream rcedgar/muscle issues 103/104 and pull request 73.
set -euo pipefail
if [[ "$(uname -s)" != Linux || "$(uname -m)" != aarch64 ]]; then
  echo "This helper targets Linux ARM64 only." >&2
  exit 1
fi
helix_prefix="${1:-$HOME/.local}"
helix_build_dir="$(mktemp -d)"
trap 'rm -rf "$helix_build_dir"' EXIT
git clone --quiet --depth 1 --branch v5.3 https://github.com/rcedgar/muscle.git "$helix_build_dir/muscle"
test "$(git -C "$helix_build_dir/muscle" rev-parse HEAD)" = 2cf9d33078c9a85697e38a6f3ad827e6862420df
curl --fail --silent --show-error --location \
  https://raw.githubusercontent.com/rcedgar/vcxproj_make/806d016/vcxproj_make.py \
  -o "$helix_build_dir/muscle/src/vcxproj_make.py"
python3 - "$helix_build_dir/muscle/src" <<'PY'
import hashlib
from pathlib import Path
import sys
src = Path(sys.argv[1])
generator = src / 'vcxproj_make.py'
if hashlib.sha256(generator.read_bytes()).hexdigest() != '902735703004c47705ffa389329378f237fecb154945b489edf6abe260c6694f':
    raise SystemExit('Unexpected upstream build-generator checksum')
p = src / 'myutils.h'
text = p.read_text()
needle = 'defined(__arm64__)'
if text.count(needle) != 1:
    raise SystemExit('Unexpected upstream platform-detection layout')
p.write_text(text.replace(needle, 'defined(__arm64__) || defined(__aarch64__)'))
text = generator.read_text().replace('-march=native', '').replace('-static', '')
text = text.replace('make 2> make.stderr', 'make -j2 2> make.stderr')
generator.write_text(text)
PY
(
  cd "$helix_build_dir/muscle/src"
  python3 vcxproj_make.py --openmp --std c++20
)
helix_binary="$(find "$helix_build_dir/muscle/bin" -maxdepth 1 -type f -name 'muscle*' -perm /111 | head -n1)"
test -n "$helix_binary"
"$helix_binary" -version
mkdir -p "$helix_prefix/bin" "$helix_prefix/share/licenses/muscle"
install -m755 "$helix_binary" "$helix_prefix/bin/muscle"
cp "$helix_build_dir/muscle/LICENSE" "$helix_prefix/share/licenses/muscle/"
printf 'Installed MUSCLE 5.3 at %s/bin/muscle. Set that path in HelixEdit Tools.\n' "$helix_prefix"
