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
 void catalogIsUniqueAndPalindromic() {
  const auto &motifs=restrictionMotifs();
  QCOMPARE(motifs.size(),26);
  QSet<QString> names;
  for(const auto &entry:motifs) {
   QVERIFY2(!names.contains(entry.first),qPrintable(entry.first));
   names.insert(entry.first);
   QCOMPARE(reverseComplement(entry.second),entry.second);
  }
 }
 void verifiedCommonPalindromicSites() {
  Record r;r.name="source";r.sequence="GGGCTAGCTTTGGTACCACGCGT";
  auto sites=findRestrictionSites(r);
  auto has=[&](const QString &enzyme,int position) {
   return std::any_of(sites.begin(),sites.end(),[&](const RestrictionSite &site){
    return site.enzyme==enzyme && site.start==position;
   });
  };
  QVERIFY(has("NheI",2));
  QVERIFY(has("KpnI",11));
  QVERIFY(has("MluI",17));
  r.sequence.replace(2,6,"CTAGCA");
  const auto mutated=findRestrictionSites(r);
  QVERIFY(std::none_of(mutated.begin(),mutated.end(),[](const RestrictionSite &site){
   return site.enzyme=="NheI";
  }));
 }
 void noWrapLinear() {
  Record r;r.name="p";r.sequence="AATTCGGGG";r.circular=false;
  QVERIFY(findRestrictionSites(r).isEmpty());
 }
};
QTEST_GUILESS_MAIN(RestrictionTests)
#include "restriction_tests.moc"
