# Release-readiness and integration checklist (2026-11-20)

## Branch reconciliation
- `main` contains all commits from `feature/sequence-studio-improvements` and `feature/0.1.1-themes`; both branches are **behind** main with **zero** unique commits according to GitHub compare.
- No merge was needed. Do not force-merge or reset main to either stale branch.

## Implemented and wired
| Workflow | Status | Verification |
|---|---|---|
| DNA/protein alignment (2+ sequences) | MAFFT 7 and MUSCLE 5 asynchronous adapters | Existing test suite; new CI smoke test pending |
| BLAST+ local database | blastn/blastp local database query; 60s preview | CI nucleotide smoke test pending |
| Sequence statistics | per-record length, gaps, ambiguous residues, GC | `analysis_statistics` test pending |
| Distance matrices | aligned identity and uncorrected p-distance | `analysis_statistics` test pending |
| Reproducible logs | local append-only plain-text history with UTC, SHA256, args, results | UI/manual review pending |
| Phylogenetic interoperability | strict sequential PHYLIP export | `analysis_statistics` test pending |
| Plasmid annotations | simple GenBank export/import | `academic_workflows` test pending |
| Primer Studio | primer CSV library, exact mapping, optional Primer3 | `plasmid_primers` test pending |

## CI
Workflow: `.github/workflows/build.yml`. On each push to main:
1. Compile and run Qt tests on Ubuntu x86_64 and ARM64.
2. Run MAFFT, MUSCLE 5 and BLAST+ smoke tests on both Linux architectures.
3. Render headless UI screenshots and build Debian packages.
4. Compile/test on Apple Silicon, run scientific integration smoke tests and bundle/sign ad hoc.

### Local verification
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
bash scripts/integration_smoke.sh
```

## Outstanding quality gates
- **Not verified here:** actual latest CI results, successful compilation on supported targets, BLAST+ UI interaction and target-device glass-style rendering.
- BLAST+ synchronous preview blocks the UI for up to 60 seconds; migrate to a cancellable asynchronous job before stable release.
- PHYLIP uses generated HX identifiers; keep the original project for name mapping.
- Distance is uncorrected and not a phylogenetic evolutionary model.
- Primer3 and cloning need validated thermodynamic/experimental test cases.
- Complex GenBank locations, full metadata and SnapGene feature parity remain incomplete.
- Validate real-world datasets, tool version reporting, reproducibility, user-controlled export of logs, and cross-platform packaging.
- No unlicensed BioEdit/SnapGene source is imported.

**Do not label this build error-free or release-ready until CI passes and desktop testing is completed.**
