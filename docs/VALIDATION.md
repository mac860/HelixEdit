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

- Extracted the generated Debian package and launched its installed-path executable successfully in offscreen preview mode; desktop entry and icon were present.

## Small rendering benchmark

A synthetic FASTA with 1,000 sequences × 2,000 residues (2 million alignment cells) was opened and rendered offscreen to a PNG by the release executable.

| Metric | Observed |
|---|---:|
| Whole process elapsed time, including a deliberate 200 ms screenshot delay | 0.63 seconds |
| Peak resident set size (`/usr/bin/time`, includes shared Qt pages) | 62,312 KiB / 60.9 MiB |

This is a single smoke measurement on the development host, not an ARM64 performance guarantee or an interactive editing benchmark. It excludes undo-history growth, external analysis jobs and prolonged use. The architecture-specific `.deb` size excludes shared runtime dependencies; a self-contained macOS bundle will be larger.

## Native platform verification

[GitHub Actions run 44783359673](https://github.com/mac860/HelixEdit/actions/runs/44783359673) verified the following on native hardware:

| Target | Build | Core + editor + real-aligner tests | Preview render | Package |
|---|---|---|---|---|
| Ubuntu 24.04 x86_64 / Qt 6.4.2 | Passed | Passed | Passed | `.deb` uploaded |
| Ubuntu 24.04 ARM64 / Qt 6.4.2 | Passed | Passed | Passed | `.deb` uploaded |
| macOS ARM64 | Runner queued | Pending | Not run | Pending |

The first Ubuntu ARM64 run passed the editor and MAFFT tests but failed with the distribution's MUSCLE 5.1 package. Upstream documents an ARM64 pointer-width detection bug. The corrected workflow builds pinned MUSCLE 5.3 with the upstream platform-detection fix; the complete ARM64 test suite then passed in 0.85 seconds. The helper does not bundle or modify MUSCLE in the HelixEdit `.deb`; users who want MUSCLE on Linux ARM64 can build it separately using the documented helper.

The Apple Silicon job is waiting for GitHub runner allocation. No successful macOS compilation or app package is claimed yet. Actual Mac M1 / Ubuntu ARM64 desktop interaction still needs target-device testing.

The macOS artifact is ad-hoc signed, not Developer ID signed or notarized. No Windows binary compatibility, BioEdit binary-project compatibility, direct SnapGene editing or full GenBank metadata round-trip is claimed.
