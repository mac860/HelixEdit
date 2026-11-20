#pragma once
#include "sequence.h"
#include <algorithm>
namespace helix {
struct Primer {
 QString name, sequence, notes;
};
struct PrimerHit {
 int start=0, end=0; // 0-based half-open, end may exceed circular length
 bool reverse=false;
};
inline bool validPrimer(QString s) {
 s=s.trimmed().toUpper();s.replace('U','T');
 return s.size()>=8 && s.size()<=200 && !s.contains(QRegularExpression("[^ACGTRYSWKMBDHVN]"));
}
inline QVector<PrimerHit> primerHits(const Record &record,QString primer) {
 QVector<PrimerHit> hits;
 primer=primer.trimmed().toUpper();primer.replace('U','T');
 if(!validPrimer(primer))return hits;
 QString dna=ungapped(record.sequence).toUpper();dna.replace('U','T');
 const int n=dna.size(), m=primer.size();
 if(m>n)return hits;
 const QString reverse=reverseComplement(primer);
 const QString scan=record.circular?dna+dna.left(m-1):dna;
 auto mask=[](QChar c)->int{
  switch(c.toLatin1()){
   case 'A':return 1;case 'C':return 2;case 'G':return 4;case 'T':return 8;
   case 'R':return 5;case 'Y':return 10;case 'S':return 6;case 'W':return 9;
   case 'K':return 12;case 'M':return 3;case 'B':return 14;case 'D':return 13;
   case 'H':return 11;case 'V':return 7;case 'N':return 15;
   default:return 0;
  }
 };
 for(int pos=0;pos<n;++pos){
  if(!record.circular&&pos+m>n)break;
  for(int strand=0;strand<2;++strand){
   const QString &query=strand?reverse:primer;
   bool match=true;
   for(int j=0;j<m;++j)if(!(mask(scan[pos+j])&mask(query[j]))){match=false;break;}
   if(match)hits.append({pos,pos+m,strand!=0});
  }
 }
 return hits;
}
inline double primerGc(QString s) {
 s=s.toUpper();s.replace('U','T');
 int gc=0,at=0;
 for(QChar c:s){if(c=='G'||c=='C')++gc;else if(c=='A'||c=='T')++at;}
 return gc+at?100.0*gc/(gc+at):-1;
}
inline int primerWallaceTm(QString s) {
 s=s.toUpper();s.replace('U','T');
 int gc=0,at=0;
 for(QChar c:s){if(c=='G'||c=='C')++gc;else if(c=='A'||c=='T')++at;}
 return 4*gc+2*at; // rough estimate only; not Primer3 thermodynamics
}
inline QVector<Primer> readPrimerCsv(const QString &csv) {
 QVector<Primer> result;
 QStringList lines=csv.split(QRegularExpression("\r?\n"),Qt::SkipEmptyParts);
 for(QString line:lines){
  auto fields=line.split(',');
  if(fields.size()<2)continue;
  QString name=fields[0].trimmed(),seq=fields[1].trimmed().toUpper();
  if(name.compare("name",Qt::CaseInsensitive)==0&&seq.compare("SEQUENCE",Qt::CaseInsensitive)==0)continue;
  if(name.isEmpty()||!validPrimer(seq))continue;
  result.append({name,seq,fields.mid(2).join(',').trimmed()});
 }
 return result;
}
}
