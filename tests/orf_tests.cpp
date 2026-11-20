#include <QtTest>
#include "orf.h"
using namespace helix;
class OrfTests : public QObject {
 Q_OBJECT
private slots:
 void forwardStop() {
  auto v=findOrfs("CCCATGAAATAAGGG",2);
  QCOMPARE(v.size(),1);
  QCOMPARE(v[0].start,3);
  QCOMPARE(v[0].end,12);
  QCOMPARE(v[0].frame,1);
  QVERIFY(v[0].hasStop);
  QCOMPARE(v[0].protein,QString("MK"));
 }
 void reverseStop() {
  auto v=findOrfs("TTATTTCAT",2);
  QCOMPARE(v.size(),1);
  QCOMPARE(v[0].start,0);
  QCOMPARE(v[0].end,9);
  QCOMPARE(v[0].frame,-1);
  QCOMPARE(v[0].protein,QString("MK"));
 }
 void incomplete() {
  QCOMPARE(findOrfs("ATGAAA",2).size(),0);
  auto v=findOrfs("ATGAAA",2,true);
  QCOMPARE(v.size(),1);
  QVERIFY(!v[0].hasStop);
  QCOMPARE(v[0].protein,QString("MK"));
 }
 void threshold() {
  QCOMPARE(findOrfs("ATGAAATAA",3).size(),0);
 }
 void rna() {
  QCOMPARE(findOrfs("AUGAAAUAA",2).size(),1);
 }
};
QTEST_GUILESS_MAIN(OrfTests)
#include "orf_tests.moc"
