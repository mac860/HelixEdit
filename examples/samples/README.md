# Bundled HelixEdit teaching sequence examples (0.1.3)

Each category opens as a **multi-record FASTA project** from File → Open examples. All work **offline**. These example records are not clinical or reference-grade interpretation tools.

## DNA (6 records, 300 nt each)

- **File:** `dna_orchid_6.fasta`.
- The initial 300 nucleotide characters of six orchid nuclear ITS/5.8S-containing DNA records, extracted from Biopython's public **ls_orchid.fasta** tutorial dataset.
- INSDC/EMBL accessions: Z78533.1, Z78532.1, Z78531.1, Z78530.1, Z78529.1, Z78527.1.
- Source: https://github.com/biopython/biopython/blob/master/Doc/examples/ls_orchid.fasta
- Original sequence records: https://www.ebi.ac.uk/ena/browser/view/Z78533.1
- These are **fragments** starting at record nucleotide 1, not complete loci. Source descriptions are retained in FASTA headers.

## RNA (3 records, 300 nt each; explicitly simulated)

- **File:** `rna_transcripts_3.fasta`.
- In-silico T→U conversions of the first 300 bases from the first three orchid DNA records above. These are educational *synthetic RNA equivalents* only. They are **not validated mature RNA transcripts**, and genomic ITS/5.8S segments are not complete processed RNAs.
- Provenance: same Biopython/INSDC accessions above. Marked `simulated_RNA` in each FASTA header.

## Protein (5 records; 110–396 aa)

- **File:** `proteins_5.fasta`.
- UniProtKB/Swiss-Prot examples (full sequences at time of retrieval):
  - GFP `P42212`: https://rest.uniprot.org/uniprotkb/P42212.fasta
  - Human insulin precursor `P01308`: https://rest.uniprot.org/uniprotkb/P01308.fasta
  - Human hemoglobin beta `P68871`: https://rest.uniprot.org/uniprotkb/P68871.fasta
  - Human hemoglobin alpha `P69905`: https://rest.uniprot.org/uniprotkb/P69905.fasta
  - E. coli aspartate aminotransferase `P00509`: https://rest.uniprot.org/uniprotkb/P00509.fasta

Proteins have different lengths and functions; their collection is **not a biologically meaningful homologous alignment**. Use it to exercise editing, basic statistics, display and import, not to infer phylogeny.

**Licensing/attribution:** Biopython's tutorial file is distributed in its BSD-licensed repository (see https://github.com/biopython/biopython/blob/master/LICENSE.rst). Sequences originate from openly accessible INSDC/UniProt records; this project attributes the accessions and is independent of the providers. Cite primary sequence records when using these data in research.
