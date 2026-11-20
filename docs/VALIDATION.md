# Validation — 20 November 2026

## Locally verified

Environment: Ubuntu 26.04 x86_64, GCC 15.2.0, Qt 6.10.2, CMake release build.

- Application compiled successfully.
- CTest: both suites passed (sequence core and editor workflows).
- QtTest: 16 passes including suite setup/cleanup; 2 optional native-aligner cases skipped because MAFFT and MUSCLE were not installed locally.
- The standard-code test independently checks all 64 codons, plus ambiguous codons, frames, partial codons and rejected gapped input.
- Parser, feature-coordinate, project round-trip and preservation-of-last-valid-save checks passed.
- Editor residue edit, undo/redo, gaps, reverse-feature transforms, SVG export and project reload checks passed.
- A fake executable verified asynchronous aligner acceptance of reordered output, restoration of original names and rejection of changed residues. This tests the adapter, not MAFFT/MUSCLE algorithms.
- Rendered the alignment and circular-map interfaces offscreen; visually inspected both screenshots.
- Created `HelixEdit-0.1.0-x86_64.deb` with CPack's dependency discovery. The compressed package was approximately 157 KiB; it depends on separately installed system Qt libraries.
- Optional Python scripts passed syntax checks; Biopython conversion was not executed because Biopython was not installed locally.

## Small rendering benchmark

A synthetic FASTA with 1,000 sequences × 2,000 residues (2 million alignment cells) was opened and rendered offscreen to a PNG by the release executable.

| Metric | Observed |
|---|---:|
| Whole process elapsed time, including a deliberate 200 ms screenshot delay | 0.63 seconds |
| Peak resident set size (`/usr/bin/time`, includes shared Qt pages) | 62,312 KiB / 60.9 MiB |

This is a single smoke measurement on the development host, not an ARM64 performance guarantee or an interactive editing benchmark. It excludes undo-history growth, external analysis jobs and prolonged use. The architecture-specific `.deb` size excludes shared runtime dependencies; a self-contained macOS bundle will be larger.

## Native platform verification

The GitHub Actions workflow provisions native Ubuntu x86_64, Ubuntu ARM64 and macOS ARM64 runners, installs the local alignment packages, runs tests and uploads installable build artifacts. Those native jobs must complete successfully before the corresponding packages are described as tested. Actual Mac M1 / Ubuntu ARM64 desktop interaction still needs target-device testing.

The macOS artifact is ad-hoc signed, not Developer ID signed or notarized. No Windows binary compatibility, BioEdit binary-project compatibility, direct SnapGene editing or full GenBank metadata round-trip is claimed.
