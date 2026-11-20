#pragma once
#include "sequence.h"
#include <cmath>
#include <stdexcept>
namespace helix {
struct SequenceStatistics {
 int alignmentLength=0, ungappedLength=0, gaps=0, ambiguous=0;
 double gc=-1;
};
inline SequenceStatistics sequenceStatistics(const Record &r,const QString &alphabet) {
 SequenceStatistics s;
 s.alignmentLength=r.sequence.size();
 s.ungappedLength=ungapped(r.sequence).size();
 s.gaps=s.alignmentLength-s.ungappedLength;
 s.gc=alphabet=="Protein"?-1:gcPercent(r.sequence);
 const QString canonical=alphabet=="Protein"?
   "ACDEFGHIKLMNPQRSTVWY":"ACGTU";
 for(QChar c:r.sequence.toUpper())
  if(c!='-'&&c!='.'&&!canonical.contains(c))++s.ambiguous;
 return s;
}
inline double pDistance(const QString &a,const QString &b) {
 if(a.size()!=b.size())throw std::invalid_argument("Distance requires equal-length aligned sequences.");
 int compared=0,mismatches=0;
 for(int i=0;i<a.size();++i) {
  if(a[i]=='-'||b[i]=='-'||a[i]=='?'||b[i]=='?'||
     a[i]=='.'||b[i]=='.')continue;
  ++compared;
  if(a[i]!=b[i])++mismatches;
 }
 return compared?double(mismatches)/compared:-1.;
}
inline QVector<QVector<double>> pDistanceMatrix(const Document &d) {
 validateDocument(d);
 const int n=d.records.size();
 QVector<QVector<double>> out(n,QVector<double>(n,0.));
 for(int i=0;i<n;++i)for(int j=i+1;j<n;++j)
  out[i][j]=out[j][i]=pDistance(d.records[i].sequence,d.records[j].sequence);
 return out;
}
// PHYLIP strict sequential export: normalized unique 10-character IDs.
inline QString phylipSequential(const Document &d) {
 validateDocument(d);
 if(d.records.size()<2)throw std::invalid_argument("PHYLIP requires at least two sequences.");
 const int columns=width(d);
 QSet<QString> ids;
 QString output=QString("%1 %2\n").arg(d.records.size()).arg(columns);
 for(int i=0;i<d.records.size();++i) {
  const auto &r=d.records[i];
  if(r.sequence.size()!=columns)
   throw std::invalid_argument("PHYLIP requires equal aligned sequence lengths.");
  const QString id=QString("HX%1").arg(i+1,8,10,QChar('0'));
  if(ids.contains(id))throw std::invalid_argument("Duplicate PHYLIP ID.");
  ids.insert(id);
  output+=id+" "+r.sequence+"\n";
 }
 return output;
}
}
