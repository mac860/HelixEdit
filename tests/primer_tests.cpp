#include <QtTest>
#include "primer.h"
using namespace helix;
class PrimerTests : public QObject {
 Q_OBJECT
private slots:
 void forward() {
  Record r;r.sequence="GGGACGTACGTTTT";
  auto hits=primerHits(r,"ACGTACGT");
  QCOMPARE(hits.size(),2); // palindrome matches both orientations
  QCOMPARE(hits[0].start,3);
 }
 void reverse() {
  Record r;r.sequence="TTTGCTAGCTAACCC";
  auto hits=primerHits(r,"TTAGCTAGC");
  QCOMPARE(hits.size(),1);
  QVERIFY(hits[0].reverse);
  QCOMPARE(hits[0].start,3);
 }
 void circular() {
  Record r;r.sequence="CGTAAAAAACG";r.circular=true;
  auto hits=primerHits(r,"CGCGTAAA");
  QCOMPARE(hits.size(),1);
  QCOMPARE(hits[0].start,9);
  QCOMPARE(hits[0].end,17);
 }
 void csv() {
  auto primers=readPrimerCsv("name,sequence,notes\\np1,ACGTACGT,control\\nbad,ZZZZZZZZ,skip\\n");
  QCOMPARE(primers.size(),1);
  QCOMPARE(primers[0].name,QString("p1"));
 }
 void estimates() {
  QCOMPARE(primerWallaceTm("AACCGGTT"),24);
  QCOMPARE(primerGc("AACCGGTT"),50.0);
 }
};
QTEST_GUILESS_MAIN(PrimerTests)
#include "primer_tests.moc"
