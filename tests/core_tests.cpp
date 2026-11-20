#include "sequence.h"
#include <QtTest>
using namespace helix;
class CoreTests: public QObject {
    Q_OBJECT
private slots:
    void fastaInput() {
        auto d=parseFasta(">one comment\nacgt. n\n>two\nA--TCG\n");
        QCOMPARE(d.records.size(),2);QCOMPARE(d.records[0].sequence,QString("ACGT-N"));
        QCOMPARE(parseFasta(fasta(d)).records[0].name,QString("one comment"));
        QVERIFY_EXCEPTION_THROWN(parseFasta("ACGT"),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(parseFasta(">empty\n"),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(parseFasta(">a\nAC123"),std::runtime_error);
    }
    void complements() {
        QString s="ACGTURYSWKMBDHVN?-";
        QCOMPARE(reverseComplement(s),QString("-?NBDHVKMWSRYAACGT"));
        QCOMPARE(reverseComplement("AUGC",true),QString("GCAU"));
        QCOMPARE(reverseComplement(reverseComplement("ACGTRYSWKMBDHVN?-")),QString("ACGTRYSWKMBDHVN?-"));
        QVERIFY_EXCEPTION_THROWN(reverseComplement("PEPTIDE"),std::runtime_error);
    }
    void geneticCode() {
        QCOMPARE(translate("ATGGCTTAA"),QString("MA*"));
        QCOMPARE(translate("AUGGCNUAA"),QString("MX*"));
        QCOMPARE(translate("AATGGCT",1),QString("MA"));
        QCOMPARE(translate("ATGA"),QString("M"));
        QVERIFY_EXCEPTION_THROWN(translate("ATG---"),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(translate("ATG",3),std::runtime_error);
        // All 64 codons independently checked against standard-code amino-acid groups.
        const QMap<QChar,QStringList> groups={
          {'F',{"TTT","TTC"}},{'L',{"TTA","TTG","CTT","CTC","CTA","CTG"}},
          {'I',{"ATT","ATC","ATA"}},{'M',{"ATG"}},{'V',{"GTT","GTC","GTA","GTG"}},
          {'S',{"TCT","TCC","TCA","TCG","AGT","AGC"}},{'P',{"CCT","CCC","CCA","CCG"}},
          {'T',{"ACT","ACC","ACA","ACG"}},{'A',{"GCT","GCC","GCA","GCG"}},
          {'Y',{"TAT","TAC"}},{'*',{"TAA","TAG","TGA"}},{'H',{"CAT","CAC"}},
          {'Q',{"CAA","CAG"}},{'N',{"AAT","AAC"}},{'K',{"AAA","AAG"}},
          {'D',{"GAT","GAC"}},{'E',{"GAA","GAG"}},{'C',{"TGT","TGC"}},
          {'W',{"TGG"}},{'R',{"CGT","CGC","CGA","CGG","AGA","AGG"}},
          {'G',{"GGT","GGC","GGA","GGG"}}};
        int count=0;for(auto it=groups.cbegin();it!=groups.cend();++it)for(const auto &codon:it.value()){QCOMPARE(translate(codon),QString(it.key()));++count;}QCOMPARE(count,64);
    }
    void statistics() {
        QCOMPARE(gcPercent("GGCCATNN--"),100.*4/6);QCOMPARE(gcPercent("NN--"),-1.);
        QCOMPARE(identity("AC--GN","ATC-GN"),75.);QCOMPARE(identity("--","--"),-1.);
        QVERIFY_EXCEPTION_THROWN(identity("AC","A"),std::runtime_error);
        auto d=parseFasta(">a\nAC-G\n>b\nAT-G\n");QCOMPARE(consensus(d),QString("AN-G"));
    }
    void clustalRoundTrip() {
        auto d=parseClustal("CLUSTAL W\n\na  AC-G 3\nb  ACTG 4\n   ** *\n\na  TT\nb  TT\n");
        QCOMPARE(d.records[0].sequence,QString("AC-GTT"));QCOMPARE(parseClustal(clustal(d)).records[1].sequence,QString("ACTGTT"));
        d.records[0].name="space name";QVERIFY_EXCEPTION_THROWN(clustal(d),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(parseClustal("CLUSTAL W\n\na AC\nb A\n"),std::runtime_error);
    }
    void genbankCoordinates() {
        QString input="LOCUS       demo 12 bp DNA circular\nFEATURES             Location/Qualifiers\n     CDS             complement(2..9)\n                     /gene=\"rev\"\n     misc_feature    join(1..2,8..9)\nORIGIN\n        1 acgtacgtacgt\n//\n";
        auto d=parseGenBank(input);QCOMPARE(d.records.size(),1);QVERIFY(d.records[0].circular);QCOMPARE(d.records[0].features.size(),1);
        auto f=d.records[0].features[0];QCOMPARE(f.start,1);QCOMPARE(f.end,9);QVERIFY(f.reverse);QCOMPARE(f.name,QString("rev"));QVERIFY(d.warnings.join(" ").contains("Unsupported"));
        input.replace("12 bp","13 bp");QVERIFY_EXCEPTION_THROWN(parseGenBank(input),std::runtime_error);
    }
    void projectRoundTrip() {
        QTemporaryDir tmp;auto d=demo();d.records[0].name="Δ construct";auto path=tmp.filePath("round.helix");writeProject(path,d);auto out=readDocument(path);
        QCOMPARE(out.records[0].name,d.records[0].name);QVERIFY(out.records[0].circular);QCOMPARE(out.records[0].features[1].end,260);
        QCOMPARE(out.records[2].sequence,d.records[2].sequence);
        d.records[0].features[0].end=99999;QVERIFY_EXCEPTION_THROWN(writeProject(path,d),std::runtime_error);
        QCOMPARE(readDocument(path).records[0].features[0].end,85); // failed save leaves last valid file
    }
    void alignmentValidation() {
        auto d=parseFasta(">original A\nACG\n>original B\nAG\n");
        auto out=acceptAlignment(d,parseFasta(">hx1\nA-G\n>hx0\nACG\n"));
        QCOMPARE(out.records[0].name,QString("original A"));QCOMPARE(out.records[1].sequence,QString("A-G"));
        QVERIFY_EXCEPTION_THROWN(acceptAlignment(d,parseFasta(">hx0\nACG\n>hx1\nATG\n")),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(acceptAlignment(d,parseFasta(">hx0\nACG\n>hx0\nACG\n")),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(acceptAlignment(d,parseFasta(">oops\nACG\n>hx1\nA-G\n")),std::runtime_error);
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
