#include <QtTest>
#include "restriction.h"
using namespace helix;
class RestrictionTests : public QObject {
 Q_OBJECT
private slots:
 void linear() {
  Record r;r.name="p";r.sequence="AAAGAATTCTTT";
  auto sites=findRestrictionSites(r);
  QCOMPARE(sites.size(),1);
  QCOMPARE(sites[0].enzyme,QString("EcoRI"));
  QCOMPARE(sites[0].start,3);
  QCOMPARE(sites[0].end,9);
 }
 void circularWrap() {
  Record r;r.name="p";r.sequence="AATTCGGGG";r.circular=true;
  auto sites=findRestrictionSites(r);
  QCOMPARE(sites.size(),1);
  QCOMPARE(sites[0].enzyme,QString("EcoRI"));
  QCOMPARE(sites[0].start,8);
  QCOMPARE(sites[0].end,14);
 }
 void noWrapLinear() {
  Record r;r.name="p";r.sequence="AATTCGGGG";r.circular=false;
  QVERIFY(findRestrictionSites(r).isEmpty());
 }
};
QTEST_GUILESS_MAIN(RestrictionTests)
#include "restriction_tests.moc"
