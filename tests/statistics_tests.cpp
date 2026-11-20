#include <QtTest>
#include "statistics.h"
using namespace helix;
class StatisticsTests: public QObject {
 Q_OBJECT
private slots:
 void stats() {
  Record r;r.name="demo";r.sequence="ACGT-N";
  auto s=sequenceStatistics(r,"DNA");
  QCOMPARE(s.alignmentLength,6);
  QCOMPARE(s.ungappedLength,5);
  QCOMPARE(s.gaps,1);
  QCOMPARE(s.ambiguous,1);
  QCOMPARE(s.gc,50.0);
 }
 void distances() {
  QCOMPARE(pDistance("ACGT","ACGA"),0.25);
  QCOMPARE(pDistance("A-GT","ATGA"),1.0/3.0);
  QCOMPARE(pDistance("----","----"),-1.0);
  QVERIFY_EXCEPTION_THROWN(pDistance("AC","A"),std::invalid_argument);
 }
 void matrix() {
  auto d=parseFasta(">one\nACGT\n>two\nACGA\n");
  const auto m=pDistanceMatrix(d);
  QCOMPARE(m.size(),2);
  QCOMPARE(m[0][1],0.25);
  QCOMPARE(m[1][0],0.25);
  QCOMPARE(m[0][0],0.0);
 }
 void phylip() {
  auto d=parseFasta(">long descriptive label\nACGT\n>second label\nACGA\n");
  auto output=phylipSequential(d);
  QVERIFY(output.startsWith("2 4\n"));
  QVERIFY(output.contains("HX00000001 ACGT\n"));
  QVERIFY(output.contains("HX00000002 ACGA\n"));
  d.records[1].sequence="ACG";
  QVERIFY_EXCEPTION_THROWN(phylipSequential(d),std::invalid_argument);
 }
};
QTEST_GUILESS_MAIN(StatisticsTests)
#include "statistics_tests.moc"
