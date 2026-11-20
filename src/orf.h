#pragma once
#include "sequence.h"
#include <algorithm>
namespace helix {
struct Orf {
 int start=0; // zero-based inclusive on the forward reference
 int end=0;   // zero-based exclusive
 int frame=0; // +1..+3 or -1..-3
 bool hasStop=false;
 QString protein;
};
inline QVector<Orf> findOrfs(const QString &sequence, int minimumAminoAcids=30, bool includePartial=false) {
 QVector<Orf> out;
 if(minimumAminoAcids<0) return out;
 QString forward=ungapped(sequence).toUpper();
 forward.replace('U','T');
 const int n=forward.size();
 if(n<3) return out;
 const QString reverse=reverseComplement(forward);
 for(int strand=0;strand<2;++strand) {
  const QString &s=strand?reverse:forward;
  for(int frame=0;frame<3;++frame) {
   int start=-1;
   for(int i=frame;i+3<=n;i+=3) {
    const QString codon=s.mid(i,3);
    if(start<0 && codon=="ATG") start=i;
    if(codon=="TAA"||codon=="TAG"||codon=="TGA") {
     if(start>=0) {
      const int aa=(i-start)/3;
      if(aa>=minimumAminoAcids) {
       Orf o;
       o.start=strand?n-(i+3):start;
       o.end=strand?n-start:i+3;
       o.frame=strand?-(frame+1):frame+1;
       o.hasStop=true;
       o.protein=translate(s.mid(start,i-start));
       out.append(o);
      }
     }
     start=-1;
    }
   }
   if(includePartial && start>=0) {
    const int codingEnd=start+((n-start)/3)*3;
    if((codingEnd-start)/3>=minimumAminoAcids) {
     Orf o;
     o.start=strand?n-codingEnd:start;
     o.end=strand?n-start:codingEnd;
     o.frame=strand?-(frame+1):frame+1;
     o.protein=translate(s.mid(start,codingEnd-start));
     out.append(o);
    }
   }
  }
 }
 std::sort(out.begin(),out.end(),[](const Orf &a,const Orf &b){
  if(a.start!=b.start)return a.start<b.start;
  return a.frame<b.frame;
 });
 return out;
}
}
