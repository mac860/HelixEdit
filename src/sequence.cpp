#include "sequence.h"
#include <stdexcept>
namespace helix {
static void fail(const QString &s) { throw std::runtime_error(s.toStdString()); }
QString validateSequence(QString s) {
    s.remove(QRegularExpression("\\s")); s=s.toUpper(); s.replace('.', '-');
    if(s.isEmpty() || s.contains(QRegularExpression("[^A-Z*?-]"))) fail("Invalid or empty sequence. Use letters, -, ?, or *.");
    return s;
}
QString ungapped(QString s) { s.remove('-'); s.remove('.'); return s; }
int width(const Document &d) { int n=0; for(const auto &r:d.records) n=qMax(n,int(r.sequence.size())); return n; }
void validateDocument(const Document &d) {
    if(d.records.isEmpty()) fail("No sequences found.");
    if(!QStringList{"DNA","RNA","Protein"}.contains(d.alphabet)) fail("Unknown molecule type.");
    for(const auto &r:d.records) {
        if(r.name.trimmed().isEmpty() || r.name.contains('\n') || r.name.contains('\r')) fail("Invalid sequence name.");
        if(validateSequence(r.sequence)!=r.sequence) fail("Sequences must be normalized uppercase.");
        if(d.alphabet!="Protein" && r.sequence.contains(QRegularExpression("[^ACGTURYSWKMBDHVN?-]"))) fail("Non-nucleotide character: select Protein molecule type for amino acids.");
        const int len=ungapped(r.sequence).size();
        for(const auto &f:r.features) if(f.start<0 || f.end<=f.start || f.end>len) fail("Feature coordinates outside sequence.");
    }
}
static void infer(Document &d) {
    bool protein=false,rna=false;
    for(const auto &r:d.records) {
        protein |= r.sequence.contains(QRegularExpression("[^ACGTURYSWKMBDHVN?-]"));
        rna |= r.sequence.contains('U');
    }
    d.alphabet=protein?"Protein":rna?"RNA":"DNA";
    if(!protein) d.warnings << "Molecule type inferred from letters; verify DNA/RNA/Protein before analysis (short proteins can look like DNA).";
    validateDocument(d);
}
Document parseFasta(const QString &text) {
    Document d;
    for(QString line:text.split('\n')) {
        line=line.trimmed(); if(line.isEmpty() || line.startsWith(';')) continue;
        if(line.startsWith('>')) { d.records.append(Record{line.mid(1).trimmed(),{},{},false}); }
        else { if(d.records.isEmpty()) fail("FASTA data must begin with >name."); d.records.last().sequence+=line; }
    }
    for(auto &r:d.records) r.sequence=validateSequence(r.sequence);
    infer(d); return d;
}
QString fasta(const Document &d) {
    QString out;
    for(const auto &r:d.records) { out+='>'+r.name+'\n'; for(int i=0;i<r.sequence.size();i+=80) out+=r.sequence.mid(i,80)+'\n'; }
    return out;
}
Document parseClustal(const QString &text) {
    Document d; QHash<QString,int> ids; bool header=false;
    for(QString line:text.split('\n')) {
        if(!header && !line.trimmed().isEmpty()) { if(!line.startsWith("CLUSTAL") && !line.startsWith("MUSCLE")) fail("Missing CLUSTAL header."); header=true; continue; }
        if(line.trimmed().isEmpty() || line.at(0).isSpace()) continue;
        auto p=line.simplified().split(' '); if(p.size()<2) fail("Malformed CLUSTAL line.");
        if(!ids.contains(p[0])) { ids.insert(p[0],d.records.size()); d.records.append(Record{p[0],{},{},false}); }
        d.records[ids[p[0]]].sequence+=validateSequence(p[1]);
    }
    infer(d);
    for(const auto &r:d.records) if(r.sequence.size()!=width(d)) fail("CLUSTAL rows have different lengths.");
    return d;
}
QString clustal(const Document &d) {
    QSet<QString> names;
    for(const auto &r:d.records) {
        if(r.sequence.size()!=width(d)) fail("CLUSTAL export requires equal row lengths; align or pad sequences first.");
        if(r.name.contains(QRegularExpression("\\s")) || names.contains(r.name)) fail("CLUSTAL requires unique names without spaces. Export FASTA instead.");
        names.insert(r.name);
    }
    QString out="CLUSTAL W multiple sequence alignment\n\n";
    int longest=0; for(const auto &r:d.records) longest=qMax(longest,int(r.name.size()));
    for(int i=0;i<width(d);i+=60) { for(const auto &r:d.records) out+=r.name.leftJustified(longest+2)+r.sequence.mid(i,60)+'\n'; out+='\n'; }
    return out;
}
Document parseGenBank(const QString &text) {
    Document d;
    for(const auto &block:text.split("//")) {
        if(block.trimmed().isEmpty()) continue;
        Record r; bool origin=false,features=false; int active=-1, declared=-1;
        for(QString line:block.split('\n')) {
            if(line.startsWith("LOCUS")) {
                auto p=line.simplified().split(' '); if(p.size()<3) fail("Invalid GenBank LOCUS.");
                r.name=p[1]; declared=p[2].toInt(); r.circular=line.contains("circular",Qt::CaseInsensitive);
            } else if(line.startsWith("FEATURES")) features=true;
            else if(line.startsWith("ORIGIN")) { origin=true; features=false; }
            else if(origin) { line.remove(QRegularExpression("[0-9\\s]")); r.sequence+=line; }
            else if(features) {
                QString key=line.mid(5,15).trimmed(), loc=line.mid(21).trimmed();
                if(!key.isEmpty()) {
                    active=-1;
                    auto m=QRegularExpression("^(complement\\()?([0-9]+)(?:\\.\\.([0-9]+))?\\)?$").match(loc);
                    if(!m.hasMatch() || (loc.startsWith("complement(")!=loc.endsWith(')'))) { d.warnings << "Unsupported GenBank feature location skipped: "+key+" "+loc; continue; }
                    int start=m.captured(2).toInt()-1, end=m.captured(3).isEmpty()?start+1:m.captured(3).toInt();
                    r.features.append(Feature{key,key,start,end,!m.captured(1).isEmpty()}); active=r.features.size()-1;
                } else if(active>=0) {
                    auto m=QRegularExpression("^/(label|gene|product)=\"([^\"]+)\"").match(loc);
                    if(m.hasMatch()) r.features[active].name=m.captured(2);
                }
            }
        }
        r.sequence=validateSequence(r.sequence);
        if(r.name.isEmpty() || declared!=r.sequence.size()) fail("GenBank sequence length does not match LOCUS.");
        d.records.append(r);
    }
    d.warnings << "GenBank import keeps sequence, topology and simple feature intervals only. Complex locations and other metadata are not preserved. Keep the original file.";
    infer(d); return d;
}
void writeProject(const QString &path,const Document &d) {
    validateDocument(d); QJsonArray a;
    for(const auto &r:d.records) {
        QJsonArray fs; for(const auto &f:r.features) fs.append(QJsonObject{{"name",f.name},{"type",f.type},{"start",f.start},{"end",f.end},{"reverse",f.reverse}});
        a.append(QJsonObject{{"name",r.name},{"sequence",r.sequence},{"circular",r.circular},{"features",fs}});
    }
    QSaveFile file(path); if(!file.open(QIODevice::WriteOnly)) fail(file.errorString());
    auto bytes=QJsonDocument(QJsonObject{{"format","HelixEdit"},{"version",1},{"alphabet",d.alphabet},{"warnings",QJsonArray::fromStringList(d.warnings)},{"records",a}}).toJson();
    if(file.write(bytes)!=bytes.size() || !file.commit()) fail(file.errorString());
}
Document readDocument(const QString &path) {
    QFile file(path); if(!file.open(QIODevice::ReadOnly)) fail(file.errorString());
    if(file.size()>128*1024*1024) fail("This preview limits imported files to 128 MiB.");
    QString text=QString::fromUtf8(file.readAll());
    if(text.trimmed().startsWith('{')) {
        QJsonParseError e; auto j=QJsonDocument::fromJson(text.toUtf8(),&e).object();
        if(e.error!=QJsonParseError::NoError || j["format"]!="HelixEdit" || j["version"].toInt()!=1) fail("Unsupported HelixEdit project.");
        Document d; d.alphabet=j["alphabet"].toString();
        for(auto warning:j["warnings"].toArray()) d.warnings.append(warning.toString());
        for(auto v:j["records"].toArray()) {
            auto o=v.toObject(); Record r{o["name"].toString(),o["sequence"].toString(),{},o["circular"].toBool()};
            for(auto v2:o["features"].toArray()) { auto f=v2.toObject(); r.features.append(Feature{f["name"].toString(),f["type"].toString(),f["start"].toInt(-1),f["end"].toInt(-1),f["reverse"].toBool()}); }
            d.records.append(r);
        }
        validateDocument(d); return d;
    }
    if(text.trimmed().startsWith('>')) return parseFasta(text);
    if(text.trimmed().startsWith("LOCUS")) return parseGenBank(text);
    if(text.startsWith("CLUSTAL") || text.startsWith("MUSCLE")) return parseClustal(text);
    fail("Supported input: FASTA, CLUSTAL, GenBank or .helix project."); return {};
}
QString reverseComplement(const QString &seq,bool rna) {
    const QString from="ACGTURYSWKMBDHVN?-",to="TGCAAYRSWMKVHDBN?-";
    QString out; out.reserve(seq.size());
    for(auto it=seq.crbegin();it!=seq.crend();++it) { int n=from.indexOf(it->toUpper()); if(n<0) fail("Reverse complement requires nucleotide symbols."); out+=to[n]; }
    if(rna) out.replace('T','U'); return out;
}
QString translate(const QString &seq,int frame) {
    if(frame<0 || frame>2) fail("Frame must be 0, 1 or 2.");
    if(seq.contains('-') || seq.contains('.')) fail("Translation requires an ungapped sequence. Export/remove gaps explicitly first.");
    QString s=seq.toUpper(); s.replace('U','T');
    if(s.contains(QRegularExpression("[^ACGTRYSWKMBDHVN?]"))) fail("Translation requires nucleotides.");
    const QString bases="TCAG", aa="FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG";
    QString out;
    for(int i=frame;i+2<s.size();i+=3) { int a=bases.indexOf(s[i]),b=bases.indexOf(s[i+1]),c=bases.indexOf(s[i+2]); out+=(a<0||b<0||c<0)?'X':aa[a*16+b*4+c]; }
    return out;
}
double gcPercent(const QString &seq) {
    int gc=0,n=0; for(QChar c:seq.toUpper()) { if(QString("ACGTU").contains(c)) { ++n; if(c=='G'||c=='C') ++gc; } } return n?100.*gc/n:-1.;
}
double identity(const QString &a,const QString &b) {
    if(a.size()!=b.size()) fail("Identity requires equal-length aligned sequences.");
    int same=0,n=0;
    for(int i=0;i<a.size();++i) if(a[i]!='-'&&b[i]!='-') { ++n; same+=(a[i]==b[i]); }
    return n?100.*same/n:-1.;
}
QString consensus(const Document &d) {
    QString out;
    for(int i=0;i<width(d);++i) {
        QMap<QChar,int> counts; for(const auto &r:d.records) if(i<r.sequence.size()&&r.sequence[i]!='-') ++counts[r.sequence[i]];
        QChar winner='-'; int max=0; bool tie=false;
        for(auto it=counts.cbegin();it!=counts.cend();++it) { if(it.value()>max) { max=it.value(); winner=it.key(); tie=false; } else if(it.value()==max) tie=true; }
        out+=tie?(d.alphabet=="Protein"?'X':'N'):winner;
    } return out;
}
Document acceptAlignment(const Document &original,const Document &aligned) {
    if(original.records.size()!=aligned.records.size()) fail("Aligner changed record count.");
    Document result=original; QSet<int> seen;
    for(const auto &r:aligned.records) {
        auto m=QRegularExpression("^hx([0-9]+)$").match(r.name); bool ok=false; int idx=m.captured(1).toInt(&ok);
        if(!m.hasMatch()||!ok||idx<0||idx>=original.records.size()||seen.contains(idx)) fail("Aligner returned unexpected/duplicate identifiers.");
        if(r.sequence.size()!=width(aligned)||ungapped(r.sequence)!=ungapped(original.records[idx].sequence)) fail("Aligner modified residues or returned unequal lengths.");
        seen.insert(idx); result.records[idx].sequence=r.sequence;
    }
    validateDocument(result); return result;
}
Document demo() {
    Document d;
    QString motif="ATGACCGTACGTTCCGATGCTAGCTACGATCGGATCCGCTAACGTTAGCGAATTCGGCATCGTACGATGACCTGACTGACG";
    QString s=motif.repeated(6);
    Record a{"pHelix_demo • synthetic teaching construct",s,{{"promoter","regulatory",15,85,false},{"reporter fragment","CDS",100,260,false},{"reverse feature","misc_feature",300,390,true}},true};
    Record b{"variant_01",s,{},false}; b.sequence[120]='A'; b.sequence[200]='G';
    Record c{"variant_02",s,{},false}; c.sequence[160]='T'; c.sequence.replace(240,3,"---");
    d.records={a,b,c}; d.warnings << "Synthetic teaching sequences and illustrative annotations; not a validated plasmid."; return d;
}
}
