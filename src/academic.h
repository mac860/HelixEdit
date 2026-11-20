#pragma once
#include "sequence.h"
#include <stdexcept>
namespace helix {
struct OverlapAssembly {
 QString sequence;
 int overlap=0;
 bool circular=false;
};
// Exact suffix/prefix overlap assembly for teaching and construct previews.
// This is not a thermodynamic or experimental assembly success predictor.
inline OverlapAssembly assembleOverlap(QString left,QString right,int minimumOverlap=15,
                                       bool circularize=false) {
 left=ungapped(left).toUpper();right=ungapped(right).toUpper();
 if(minimumOverlap<1||left.isEmpty()||right.isEmpty())
  throw std::invalid_argument("Assembly requires nonempty fragments and a positive minimum overlap.");
 if(left.contains(QRegularExpression("[^ACGT]"))||right.contains(QRegularExpression("[^ACGT]")))
  throw std::invalid_argument("Assembly requires unambiguous DNA fragments.");
 const int maximum=qMin(left.size(),right.size());
 int overlap=0;
 for(int n=maximum;n>=minimumOverlap;--n)
  if(left.right(n)==right.left(n)){overlap=n;break;}
 if(!overlap)throw std::invalid_argument("No exact suffix-prefix overlap meets the threshold.");
 QString product=left+right.mid(overlap);
 if(circularize){
  // Circularization requires a second, independent terminal overlap.
  int closure=0;
  for(int n=product.size()/2;n>=minimumOverlap;--n)
   if(product.left(n)==product.right(n)){closure=n;break;}
  if(!closure)throw std::invalid_argument("Product termini do not support exact circularization.");
  product.chop(closure);
 }
 return {product,overlap,circularize};
}
inline QVector<QVector<double>> pairwiseIdentityMatrix(const Document &d) {
 validateDocument(d);
 const int n=d.records.size();
 QVector<QVector<double>> matrix(n,QVector<double>(n,100.0));
 for(int i=0;i<n;++i)for(int j=i+1;j<n;++j) {
  const double value=identity(d.records[i].sequence,d.records[j].sequence);
  matrix[i][j]=matrix[j][i]=value;
 }
 return matrix;
}
}
