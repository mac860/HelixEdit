#pragma once
#include "sequence.h"
#include <algorithm>
namespace helix {
struct RestrictionSite {
 QString enzyme;
 int start=0; // zero-based recognition start
 int end=0;   // zero-based recognition end, may exceed molecule length for circular wrap
 bool reverse=false;
};
inline QVector<RestrictionSite> findRestrictionSites(const Record &record) {
 QVector<RestrictionSite> result;
 QString seq=ungapped(record.sequence).toUpper();
 seq.replace('U','T');
 const int n=seq.size();
 if(n==0)return result;
 // A focused set of exact, nondegenerate PALINDROMIC recognition motifs.
 // Recognition starts are reported, not cleavage coordinates. No methylation
 // or buffer compatibility is inferred. See NEB specificity catalog.
 const QVector<QPair<QString,QString>> motifs={
  {"AatII","GACGTC"},{"ApaI","GGGCCC"},{"BamHI","GGATCC"},
  {"BglII","AGATCT"},{"ClaI","ATCGAT"},{"EcoRI","GAATTC"},
  {"HindIII","AAGCTT"},{"HpaI","GTTAAC"},{"KpnI","GGTACC"},
  {"MluI","ACGCGT"},{"NcoI","CCATGG"},{"NdeI","CATATG"},
  {"NheI","GCTAGC"},{"NotI","GCGGCCGC"},{"NruI","TCGCGA"},
  {"NsiI","ATGCAT"},{"PstI","CTGCAG"},{"PvuII","CAGCTG"},
  {"SacI","GAGCTC"},{"SalI","GTCGAC"},{"SmaI","CCCGGG"},
  {"SpeI","ACTAGT"},{"SphI","GCATGC"},{"SwaI","ATTTAAAT"},
  {"XbaI","TCTAGA"},{"XhoI","CTCGAG"}
 };
 for(const auto &entry:motifs) {
  const QString motif=entry.second;
  if(motif.size()>n)continue;
  const QString search=record.circular?seq+seq.left(motif.size()-1):seq;
  for(int pos=0;pos<n;++pos) {
   if(!record.circular && pos+int(motif.size())>n)break;
   if(search.mid(pos,motif.size())==motif)
    result.append({entry.first,pos,pos+int(motif.size()),false});
  }
 }
 std::sort(result.begin(),result.end(),[](const auto &a,const auto &b){
  return a.start==b.start?a.enzyme<b.enzyme:a.start<b.start;
 });
 return result;
}
}
