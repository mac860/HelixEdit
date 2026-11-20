# HelixEdit 0.1.7 — GenBank and FASTA import repair

## Reproduced user failure

The uploaded Addgene record `addgene-plasmid-10878-sequence-438456.gbk` is **20,747 bytes**, with:

- `LOCUS sequence-438456- 8901 bp DNA circular` and `ORIGIN` containing **8,901 A/C/G/T characters**.
- `COMMENT` metadata containing `https://www.snapgene.com` (the first `//` in the file).
- A **single standalone GenBank `//` record terminator** at the end.
- **42 feature entries** including **40 simple locations** and two complex `complement(join(...))` locations.
- **41 unquoted feature labels** such as `/label=ori`, `/label=AmpR promoter`, and `/label=T7 promoter`.

The old parser used `text.split("//")` which incorrectly split at `https://`, producing a fragment without any ORIGIN. `validateSequence()` then raised `Invalid or empty sequence`. The original uploaded biological sequence is **not empty or malformed**.

## 0.1.7 fixes

1. Parse GenBank record terminators **only when `//` appears alone on its own line**, not as a substring in URLs or feature notes.
2. Preserve both unquoted `/label=AmpR promoter` and quoted `/label="AmpR promoter"` feature labels. Prefer an explicit label over generic gene/product descriptions.
3. Distinguish a missing GenBank `ORIGIN` from malformed sequence characters and report a LOCUS/ORIGIN size mismatch with both lengths.
4. Handle UTF-8 BOMs for FASTA and GenBank. Accept FASTA files beginning with blank, `;` or `#` comment lines.
5. Give record-aware FASTA errors that identify the offending character/position instead of an uninformative generic error.
6. Keep existing scientific restrictions: no inference of topology from FASTA, no silent remapping of complex joined feature locations, and annotation transfer only after exact donor/recipient nucleotide equality.

## Test coverage

Core regression tests now simulate a realistic Addgene/SnapGene-style GenBank file containing a URL, 24 bp circular DNA, three annotation features, and quoted/unquoted labels. The test also validates **file-open detection via `readDocument()`**, not just direct calls to `parseGenBank()`.

Additional tests exercise FASTA with a BOM, comments, multiple records, and the same file-based import route. The user's full uploaded file was inspected separately, but is **not committed to this public repository as a test fixture**.

## Build on Ubuntu 26.04 x86_64

```bash
cd ~/Downloads/v3/HelixEdit
git pull --ff-only origin main
bash scripts/build_ubuntu_deb.sh
sudo apt install --reinstall ./build-ubuntu-amd64/HelixEdit-0.1.7-x86_64.deb
```

After updating, use **File → Open…** and select your `.gbk` file. The expected imported project has one circular 8,901 bp DNA record with source annotations. Two joined-location feature entries are intentionally skipped and reported; if exact GenBank feature parity is required, retain the original file.

**Verification status:** The source patch and regression tests are committed, and the uploaded record's structure/length were independently inspected. A completed Qt6 compile and GUI import run of 0.1.7 has not yet been verified on Ubuntu 26.04.
