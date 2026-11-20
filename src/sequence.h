#pragma once
#include <QtCore>
namespace helix {
struct Feature { QString name, type; int start=0, end=0; bool reverse=false; }; // zero-based, half-open, ungapped
struct Record { QString name, sequence; QVector<Feature> features; bool circular=false; };
struct Document { QVector<Record> records; QString alphabet="DNA"; QStringList warnings; };
Document parseFasta(const QString &text);
Document parseClustal(const QString &text);
Document parseGenBank(const QString &text);
Document readDocument(const QString &path);
void writeProject(const QString &path, const Document &doc);
QString fasta(const Document &doc);
QString clustal(const Document &doc);
QString genbank(const Document &doc); // exports simple intervals; rejects unsupported names
QString ungapped(QString seq);
QString reverseComplement(const QString &seq, bool rna=false);
QString translate(const QString &seq, int frame=0); // standard genetic code; gaps rejected
QString consensus(const Document &doc); // majority; ties N (DNA/RNA) or X (protein)
double gcPercent(const QString &seq); // A,C,G,T,U denominator only; -1 if undefined
double identity(const QString &a, const QString &b); // exact, non-gap pairs; -1 if undefined
QString validateSequence(QString seq);
void validateDocument(const Document &doc);
int width(const Document &doc);
Document demo();
Document acceptAlignment(const Document &original, const Document &aligned); // hx0... identifiers
}
