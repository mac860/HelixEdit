#include <QtTest>
#include "academic.h"
using namespace helix;
class AcademicTests : public QObject {
 Q_OBJECT
private slots:
 void overlapAssembly() {
  auto a=assembleOverlap("AAAACCCCGGGG","GGGGTTTT",4);
  QCOMPARE(a.sequence,QString("AAAACCCCGGGGTTTT"));
  QCOMPARE(a.overlap,4);
  QVERIFY(!a.circular);
 }
 void overlapFailure() {
  QVERIFY_EXCEPTION_THROWN(assembleOverlap("AAAA","TTTT",2),std::invalid_argument);
  QVERIFY_EXCEPTION_THROWN(assembleOverlap("AAAA","AAAN",2),std::invalid_argument);
 }
 void matrix() {
  Document d=parseFasta(">a\\nACGT\\n>b\\nACGA\\n>c\\nA-GA\\n");
  const auto m=pairwiseIdentityMatrix(d);
  QCOMPARE(m.size(),3);
  QCOMPARE(m[0][1],75.0);
  QCOMPARE(m[1][0],75.0);
  QCOMPARE(m[1][2],100.0);
 }
 void annotatedGenBankRoundTrip() {
  Document d;Record r;
  r.name="pAcademic";r.sequence="ATGGAATTCTAA";r.circular=true;
  r.features.append({"reporter","CDS",0,12,false});
  r.features.append({"reverse","misc_feature",3,9,true});
  d.records.append(r);
  const auto gb=genbank(d);
  const auto parsed=parseGenBank(gb);
  QCOMPARE(parsed.records.size(),1);
  QCOMPARE(parsed.records[0].sequence,r.sequence);
  QVERIFY(parsed.records[0].circular);
  QCOMPARE(parsed.records[0].features.size(),2);
  QCOMPARE(parsed.records[0].features[1].start,3);
  QVERIFY(parsed.records[0].features[1].reverse);
 }
};
QTEST_GUILESS_MAIN(AcademicTests)
#include "academic_tests.moc"
