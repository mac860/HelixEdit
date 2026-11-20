#!/usr/bin/env bash
set -euo pipefail
for tool in mafft muscle blastn blastp makeblastdb; do
  command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 1; }
done
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/seq.fa" <<'EOF'
>one
ATGACCGTACGTTCCGATGCTAGCTACGATCGGATCCGCTAACGTTAGCGAATTCGGCATCGTACGATGACCTGACTGACG
>two
ATGACCGTACGTTCCGATGCTAGCTACGATCGGATCCGCTAACGTTAGCGAATTCGGCATCGTACGATGACCTGACTGACA
EOF
mafft --auto --nuc "$tmp/seq.fa" > "$tmp/mafft.fa" 2> "$tmp/mafft.log"
grep -q '^>one' "$tmp/mafft.fa"
grep -q '^>two' "$tmp/mafft.fa"
muscle -align "$tmp/seq.fa" -output "$tmp/muscle.fa" -nt > "$tmp/muscle.log" 2>&1
grep -q '^>one' "$tmp/muscle.fa"
grep -q '^>two' "$tmp/muscle.fa"
makeblastdb -in "$tmp/seq.fa" -dbtype nucl -out "$tmp/nucldb" > "$tmp/makeblastdb.log" 2>&1
blastn -query "$tmp/seq.fa" -db "$tmp/nucldb" -outfmt 6 -max_target_seqs 10 > "$tmp/blast.tsv"
test -s "$tmp/blast.tsv"
echo "MAFFT, MUSCLE 5, makeblastdb and BLAST+ nucleotide smoke tests passed"
