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

## Three integrated scientific workspaces — refined specifications

HelixEdit is one native offline-first application with three complementary workspaces sharing the same versioned project document, undo/redo, annotation coordinate model and provenance metadata. Workspace switching must **not** discard edits or silently replace the current alignment. Distinguish features already present in 0.1.x from proposals below.

### 1. Sequence Studio — alignment, translation and sequence editing

**Goal:** a fast, keyboard-friendly DNA/RNA/protein editor for routine wet-lab and comparative-sequence work.

**Core workflow:** create/open a project → import FASTA/CLUSTAL/GenBank → edit or align sequences → inspect conserved regions → translate, find ORFs or primers → save the editable project and export results.

**Capabilities and acceptance criteria:**
- **Multiple-sequence alignment:** run local MAFFT 7 and MUSCLE 5 asynchronously, expose safe presets and advanced arguments, preview results before replacement, preserve sequence identifiers, show job logs, allow cancellation, and save tool version/parameters.
- **Residue coloring:** nucleotide A/C/G/T/U, ambiguous bases, gaps, amino-acid chemistry, conservation and identity modes; accessible high-contrast palettes in Light, Dark and Dracula. Selection highlight must remain legible.
- **Editing:** keyboard residue entry, gap insertion/deletion, rectangular selection, move/slide alignment blocks, row reordering, reversible changes, Find/Replace and project-level undo.
- **Protein translation:** six reading frames, selectable genetic code, start/stop handling, ambiguous codon reporting and alignment of translated regions to source nucleotides; default standard code remains available.
- **ORF discovery:** forward/reverse strand scanning with user-defined start codons, minimum length, genetic code and circular-origin handling; list coordinates and translate selected ORFs.
- **Primer mapping:** exact and mismatch-aware forward/reverse binding sites, orientation, length, GC%, approximate Tm with method stated, ambiguous bases and potential off-target sites; results linked to sequence coordinates.
- **Annotation editing:** add/edit/delete CDS, gene, promoter, primer and custom features; validate 1-based inclusive UI positions against internal zero-based half-open coordinates; preserve qualifiers where supported.
- **Export:** FASTA/CLUSTAL, translated FASTA, feature tables, alignment SVG/PNG and a reproducible run report.

**Verification:** codon-table tests, both-strand ORF fixtures, primer orientation/mismatch tests, IUPAC behavior, aligned-output invariants and large-alignment responsiveness.

### 2. Plasmid Studio — maps, restriction analysis and cloning

**Goal:** an interactive plasmid and construct-design environment that preserves biological feature metadata.

**Core workflow:** import/create circular or linear DNA → annotate features → inspect restriction and primer sites → simulate digest/PCR/cloning → validate construct → save/export map and sequence.

**Capabilities and acceptance criteria:**
- **Maps:** zoomable linear/circular vector maps with orientation arrows, overlapping tracks, base-pair ruler, selectable labels, drag-free precise coordinate editing, hover details and high-resolution SVG/PDF output.
- **Restriction analysis:** configurable enzyme catalog with recognition motifs, cut offsets, strand direction, overhangs and circular wrap-around; distinguish recognition from cleavage; report fragments and single/double digests with enzyme database provenance.
- **Cloning workflows:** restriction/ligation and overlap-based assembly planning as simulations, with junction sequences, orientation checks, reading-frame checks, warnings and non-destructive construct previews.
- **GenBank compatibility:** import/export of compound locations, complement/join, CDS qualifiers, references and topology where supported; explicitly warn on unsupported metadata and never silently discard it during round-trip.
- **Primer binding sites:** mapped primer arrows, mismatches, product lengths, circular PCR and amplicon previews; cross-link to Sequence Studio.
- **Interactive feature tracks:** visibility filters, category colors, track reordering, feature inspector, search and synchronization between sequence selection and map highlighting.
- **Project safety:** maintain original imported files, create derived constructs as new records, and keep audit metadata for simulated edits.

**Verification:** circular cut-site fixtures, fragment lengths summing to molecule length, GenBank round-trip comparison, reverse-strand coordinates, primer junction and cloning-orientation tests.

### 3. Analysis Studio — reproducible local bioinformatics

**Goal:** accessible analysis without turning HelixEdit into a heavyweight bundled bioinformatics distribution.

**Core workflow:** select project sequences → choose analysis/tool → review parameters and executable version → run locally with progress/cancel → inspect tables/plots → export results and provenance.

**Capabilities and acceptance criteria:**
- **MAFFT / MUSCLE:** shared alignment job manager, executable discovery, validated inputs/outputs, timeouts/cancellation and clear dependency setup instructions for Ubuntu x86_64/ARM64 and Apple Silicon.
- **BLAST+:** local blastn/blastp (and later blastx/tblastn) with database manager, parameter presets, hit table, alignment detail, e-value/identity/coverage filters and tabular/JSON export. Never download databases without consent.
- **Sequence statistics:** length, gap fraction, canonical/ambiguous GC, base/amino-acid composition, motif counts, consensus and conservation; document denominator and ambiguity policies.
- **Distance matrices:** p-distance and configurable gap handling first, then scientifically justified models; export labeled CSV/TSV and heatmaps with explicit method settings.
- **Phylogenetics (later phase):** optional external IQ-TREE integration for aligned datasets, model selection, bootstrap/support values, Newick import/export and tree viewing; clearly separate inference from visualization.
- **Reproducibility:** per-run log with input hashes, program version, command arguments, timestamp, parameters, outputs, errors and provenance; avoid shell interpolation by using QProcess argument arrays.
- **Safety and performance:** background jobs, cancellation, bounded memory, temporary-file cleanup, input-size warnings and meaningful error messages when optional programs are missing.

**Verification:** independent reference datasets, known distance matrices, parameter/identity preservation, malformed-output rejection and reproducible results across supported architectures.

### Shared navigation and release discipline

- **Project Home:** New Project, Open Project, Recent Projects, autosave/recovery status and a clear Empty Workspace action. Starting a new project saves or prompts for current unsaved changes and never deletes existing files.
- **Workspace tabs:** Sequence | Plasmid | Analysis. All views reflect one active project, and results can be saved as linked artifacts without overwriting source records.
- **Suggested implementation order:** 0.1.x stabilize project management/themes → 0.2 Sequence Studio editing, ORFs, translation and primer mapping → 0.3 Plasmid Studio GenBank/restriction/maps → 0.4 Analysis Studio BLAST/statistics/distances → 0.5 cloning and performance → later phylogenetics and stable 1.0.
- **Definition of done:** working implementation, automated scientific tests, GUI accessibility checks, documentation, sample project, native CI packaging and an explicit limitations note. Planned functionality is not labeled shipped.

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
