# Sources and package references

Checked 20 November 2026. External dependencies retain their own licenses.

| Purpose | Official source | Current use |
|---|---|---|
| Historical workflow | https://thalljiscience.github.io/ | BioEdit inspiration only |
| Original distribution | https://github.com/thalljiscience/thalljiscience.github.io | Examined; no application source visible |
| BioEdit license | https://thalljiscience.github.io/LICENSE.TXT | Does not explicitly authorize modifications; original assets not reused |
| BioEdit manual | https://thalljiscience.github.io/BioDoc.pdf | Documents Borland C++ Builder and Windows-specific architecture |
| Qt | https://doc.qt.io/qt-6/ | Desktop runtime |
| Table model | https://doc.qt.io/qt-6/qabstracttablemodel.html | Virtual alignment grid |
| Process execution | https://doc.qt.io/qt-6/qprocess.html | Local asynchronous package adapters |
| Qt deployment | https://doc.qt.io/qt-6/macos-deployment.html | macOS bundle packaging |
| Qt licenses | https://www.qt.io/licensing/ | Qt is separately licensed; preserve dependency notices in distributions |
| MAFFT | https://mafft.cbrc.jp/alignment/software/ | Optional `--auto input.fasta` adapter |
| MUSCLE 5 | https://github.com/rcedgar/muscle | Optional alignment package |
| MUSCLE command | https://drive5.com/muscle5/manual/cmd_align.html | `-align input -output output` |
| Biopython | https://biopython.org/wiki/SeqIO | Optional format conversion, including ABI and SnapGene sequence extraction |
| BLAST+ | https://www.ncbi.nlm.nih.gov/books/NBK279690/ | Planned search integration; no search adapter yet |
| GitHub runners | https://docs.github.com/en/actions/reference/runners/github-hosted-runners | ARM64 and macOS build matrix |

ARM64 MUSCLE platform issue and upstream fix: https://github.com/rcedgar/muscle/issues/103, https://github.com/rcedgar/muscle/pull/104 and https://github.com/rcedgar/muscle/pull/73. The optional Linux ARM64 helper pins v5.3 source commit `2cf9d33078c9a85697e38a6f3ad827e6862420df` and checks the official build generator checksum. macOS MUSCLE uses the maintained https://github.com/brewsci/homebrew-bio formula.

HelixEdit is independent of BioEdit and SnapGene. Neither project endorses or maintains this application.
