#!/usr/bin/env python3
"""Preserve license files shipped with Qt/Homebrew in a bundled distribution."""
import shutil
import sys
from pathlib import Path

prefix, target = map(Path, sys.argv[1:])
target.mkdir(parents=True, exist_ok=True)
candidates = list(prefix.glob('**/LICENSES')) + list(prefix.glob('**/licenses'))
copied = 0
for source in candidates:
    if source.is_dir():
        for file in source.rglob('*'):
            if file.is_file():
                relative = file.relative_to(prefix)
                destination = target / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(file, destination)
                copied += 1
for pattern in ('LICENSE*', 'COPYING*'):
    for file in prefix.glob(pattern):
        if file.is_file():
            shutil.copy2(file, target / file.name)
            copied += 1
if not copied:
    raise SystemExit('No Qt license files found. Inspect the installed Qt package before distribution.')
print(f'Preserved {copied} license files from {prefix}')
