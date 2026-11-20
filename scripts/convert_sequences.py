#!/usr/bin/env python3
"""Optional sequence-only conversion with Biopython. Does not run inside the GUI.

Examples:
  python scripts/convert_sequences.py sample.ab1 sample.fasta --input-format abi
  python scripts/convert_sequences.py sample.dna sample.fasta --input-format snapgene
  python scripts/convert_sequences.py alignment.phy alignment.fasta --input-format phylip

The output intentionally omits annotations, topology, qualities and chromatogram signals.
Use the original file for those data. No files are overwritten.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import os
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--input-format', required=True, help='Biopython SeqIO format, e.g. abi, abi-trim, snapgene, phylip')
    args = parser.parse_args()
    try:
        from Bio import SeqIO
    except ImportError:
        parser.exit(2, 'Biopython is optional. Install scripts/requirements.txt in a virtual environment first.\n')
    if args.output.exists():
        parser.exit(2, 'Output already exists; choose a new filename.\n')
    temporary = None
    try:
        # Stream records to a sibling temp file so conversion errors never leave partial output.
        with tempfile.NamedTemporaryFile('w', encoding='utf-8', dir=args.output.parent, delete=False) as target:
            temporary = Path(target.name)
            count = SeqIO.write(SeqIO.parse(str(args.input), args.input_format), target, 'fasta')
        if not count:
            raise ValueError('No sequence records found')
        # Atomic, no-clobber publication on macOS/Linux; a concurrently created target is protected.
        os.link(temporary, args.output)
        print(f'Converted {count} record(s) to {args.output}. Sequence only; retain the original.')
        return 0
    except (OSError, ValueError) as exc:
        print(f'Conversion failed: {exc}', file=sys.stderr)
        return 1
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == '__main__':
    raise SystemExit(main())
