# HelixEdit — Product plan and scientific development roadmap

Updated: 2026-11-20. Status: planning document; **not** a claim that all features are implemented.

## Product vision
A compact, colorful, offline-first native C++17 / Qt 6 alternative to BioEdit for Linux x86_64, Linux ARM64 and macOS Apple Silicon. Independent of BioEdit and SnapGene proprietary assets. Preserve scientific accuracy, original input files and low resource usage.

## Implemented or under review in the 0.1.1 development branch
- Light, Dark and Dracula palettes with saved appearance preference.
- File → New project / workspace: select parent directory and name, create a separate project folder with a blank sequence and a .helix project file. Never overwrite existing folders.
- Save modified current document before switching; cancellation keeps current work.
- Periodic 30-second save for named .helix projects and recovery copy; imported sequence files are not overwritten.
- Version metadata updated to 0.1.1. This is a beta candidate, not a published release.
- Important: a new project contains an empty placeholder sequence because the current document model requires at least one record; a fully zero-row editor requires a subsequent model refactor.

## P0 — Beta release gates
1. Build, CTest, GUI smoke tests and packages on Ubuntu 24.04+ x86_64 and ARM64 and macOS ARM64.
2. Validate actual Mac M1 interaction, file dialogs, keyboard, graphics and app bundle.
3. Verify no data loss on project switching, autosave, crash recovery and Save As.
4. Verify all three themes across table, dialogs, maps, selection and export; make map and residue palettes theme-aware.
5. Review license notices for Qt and optional executables; package .deb and macOS ZIP, later sign/notarize macOS.
6. Release notes, changelog, SHA-256 checksums, install instructions and beta limitations.
7. Test empty placeholder behavior and update automated editor tests.

## P1 — Sequence Studio
- Keyboard-first residue editing, insert/delete, gap drag/slide, block editing, advanced undo/redo.
- Multiple documents and project tabs; project browser and recent workspaces.
- Alignment ruler, conserved-site tracks, consensus and quality visualization.
- IUPAC-aware searches, regex search, circular wrap-around, ORF finder and configurable translation tables.
- MAFFT and MUSCLE parameters, logs, reproducibility manifests and safe cancellation.
- FASTA, CLUSTAL, PHYLIP and rich GenBank import/export with round-trip tests.
- Alignment similarity/distance matrices with explicit gap and ambiguity policies.

## P2 — Plasmid Studio
- Accurate linear and circular maps, selectable labels, feature tracks and SVG/PDF export.
- Compound GenBank locations (join/complement/order), qualifiers, CDS, source and topology.
- Restriction enzyme database with recognition and cut positions, methylation caveats, overhangs.
- Primer binding, PCR simulation, cloning plans, digest simulation and validation tests.
- Optional SnapGene sequence-only import with clear metadata-loss warnings; no proprietary code.

## P3 — Analysis Studio
- BLAST+ local search interface with database selection, parameter capture, results and export.
- Optional phylogenetics integrations (e.g. IQ-TREE), model-selection metadata and provenance.
- AB1/ABI chromatogram viewer, basecall quality and trace-preserving workflows.
- Protein properties, motif analysis, multiple genetic codes, domain annotations.
- Optional integrations only: avoid embedding large Python runtimes or analysis suites in the core app.

## Architecture and performance
- Keep native C++17/Qt 6 and offline-first workflow; Qt table model remains virtualized.
- Move expensive parsing, statistics and import to background workers with cancellation.
- Replace whole-document undo snapshots with bounded edit deltas.
- Use QSaveFile atomic persistence and independent recovery backups; avoid treating autosave as versioned backup.
- Add real interaction benchmarks (scroll latency, edit latency, memory growth) on x86_64 and ARM64.
- Validate malformed input, zip/path traversal if future archives are supported, file-size limits, safe external processes.
- Accessibility: keyboard navigation, high contrast, scalable fonts and colorblind-safe nucleotide schemes.

## Version targets
| Version | Goal |
|---|---|
| 0.1.1 beta | Project creation, autosave, Light/Dark/Dracula, verified builds |
| 0.2.0 | Editing ergonomics and multi-document workspace |
| 0.3.0 | Rich GenBank and plasmid annotations |
| 0.4.0 | AB1 chromatograms, BLAST+ UI, scientific export |
| 0.5.0 | Performance, accessibility and robust compatibility |
| 1.0.0 | Stable documented scientifically validated release |

## Scientific validation checklist
- Compare codon translation against reference tables including alternative codes.
- Test reverse complements and annotation coordinate transforms.
- Round-trip compound GenBank features and document unsupported qualifiers.
- Compare restriction cuts and circular digests to independent trusted references.
- Test ambiguity and gaps in consensus, identity and distance calculations.
- Test external aligners with real datasets, not only mocked processes.
- Publish datasets, expected outputs, supported formats and limitations.

## Suggested release description
**HelixEdit 0.1.1 Beta** — An experimental offline-first sequence editor for Ubuntu x86_64/ARM64 and macOS Apple Silicon, with colorful Light/Dark/Dracula themes, project workspaces, alignment and map tools. Platform support must be qualified by actual CI and device results. Not yet a complete BioEdit or SnapGene replacement.
