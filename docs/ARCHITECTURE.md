# Architecture

HelixEdit is a new C++17/Qt 6 application. No BioEdit binaries, source code, tables, icons or SnapGene assets are incorporated.

- `src/sequence.*`: normalized document data, parsers, atomic project persistence, standard-code translation, complements, statistics and aligner validation.
- `src/editor.*`: Qt Widgets UI; `QAbstractTableModel` supplies cells on demand instead of allocating a widget per residue. Qt handles visible-cell painting and scrolling. `MapView` paints vector geometry and exports SVG.
- `QUndoStack`: at most 30 document changes. Qt strings use implicit sharing, but whole edited rows and history can still consume significant RAM. This is a desktop sequence editor, not a whole-genome browser.
- `QProcess`: asynchronous MAFFT 7 / MUSCLE 5 adapters, argument lists (no shell command interpolation), temporary input/output files, stable internal IDs, strict output checks and original-name restoration.
- Qt JSON `.helix`: versioned document format. Stores molecule type, names, residues, circular topology and supported feature intervals. Writes via `QSaveFile`.
- Optional Python converter uses Biopython; Python is not part of the desktop runtime.

## Coordinates and scientific rules

UI coordinates are 1-based and inclusive. Stored feature coordinates are zero-based, half-open, relative to the ungapped sequence. Gaps change alignment positions, not feature coordinates. Removing residues or changing a residue into/out of a gap clears affected annotations rather than retaining invalid locations. Reverse complement transforms feature coordinates and direction. Base substitutions preserve feature locations but may invalidate their biological meaning; revalidate edited annotations yourself.

Consensus excludes gaps and absent trailing cells; the most frequent symbol wins, with ties represented by N or X. Symbols compare literally, not as IUPAC sets. Identity requires equal-length rows, excludes any column containing a gap, and compares remaining symbols literally. GC uses canonical A/C/G/T/U only. Translation uses the standard genetic code, maps any ambiguous codon to X, rejects gaps and ignores incomplete terminal codons. Alternative genetic codes and CDS qualifiers are not interpreted.

## Import boundaries

FASTA and CLUSTAL import normalize lowercase to uppercase and dots to gaps. Molecule type is inferred from characters; a short protein containing only nucleotide-like letters is ambiguous. The molecule selector must be checked before analysis. Different row lengths are accepted in FASTA and displayed as blank trailing cells; CLUSTAL requires equal row lengths.

GenBank support is deliberately limited: sequence, topology, simple intervals/single-base features and single-line label/gene/product qualifiers. `join`, `order`, fuzzy positions, remote accessions and compound features are skipped with a warning. Other qualifiers, references and metadata are not retained. The original file must be kept. GenBank export is not implemented.

The `.helix` format does not support traces or BioEdit's proprietary binary projects. A BioEdit workflow should export FASTA/CLUSTAL or GenBank first. SnapGene `.dna` is not directly supported; the optional Biopython converter extracts sequences only.

## Persistence

A single instance per user profile is protected by a lock file. Unsaved edits are autosaved every 30 seconds to Qt's application-data directory. Recovery is offered after an interrupted session. Save and an intentional discard/close remove recovery data. Autosave is not a backup system. Input files over 128 MiB are rejected in this development preview. UI imports and analyses run on the main thread; very large datasets can stall it. External alignment runs in a separate process.

## Extension priorities

1. ABI chromatogram reader/viewer with basecall quality and original-trace preservation.
2. Richer GenBank feature-location model and annotation editing.
3. Alignment drag/slide, keyboard residue entry and multi-document tabs.
4. Configurable, benchmarked large-file limits and delta-based undo.
5. BLAST+ search UI, reproducible parameter manifests, and additional aligners.
6. Signed/notarized macOS distributions and verified ARM64 release artifacts.
