#include "editor.h"
#include "themes.h"
#include "updater.h"
#include "orf.h"
#include "restriction.h"
#include "primer.h"
#include "academic.h"
#include "statistics.h"
#include <QtSvg/QSvgGenerator>
#include <QDesktopServices>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#ifdef Q_OS_UNIX
#include <signal.h>
#include <unistd.h>
#endif
namespace helix {
static QColor residueColor(QChar c,bool protein) {
    if(protein) {
        if(QString("AVLIMFWY").contains(c)) return QColor("#dcebe3");
        if(QString("KRH").contains(c)) return QColor("#e5dafa");
        if(QString("DE").contains(c)) return QColor("#ffd9d4");
        if(QString("STNQ").contains(c)) return QColor("#d8eef7");
        return QColor("#f4eee0");
    }
    if(c=='A') return QColor("#cbf1df"); if(c=='C') return QColor("#d2e9ff");
    if(c=='G') return QColor("#ffe8b5"); if(c=='T'||c=='U') return QColor("#ffd6df");
    return QColor("#f0edf7");
}
static QString number(double value) {return value<0?"undefined":QString::number(value,'f',1)+"%";}
QVariant AlignmentModel::data(const QModelIndex &i,int role) const {
    if(!i.isValid()) return {};
    const auto &r=document.records[i.row()]; bool present=i.column()<r.sequence.size();
    QChar c=present?r.sequence[i.column()]:QChar(' ');
    if(role==Qt::DisplayRole || role==Qt::EditRole) return QString(c);
    if(role==Qt::TextAlignmentRole) return int(Qt::AlignCenter);
    if(role==Qt::BackgroundRole) return present?residueColor(c,document.alphabet=="Protein"):QColor("#faf9fc");
    if(role==Qt::ForegroundRole) return QColor(c=='-'?"#a09aaf":"#29283c");
    if(role==Qt::ToolTipRole) return r.name+"\nAlignment column "+QString::number(i.column()+1)+(present?"":" (outside sequence)");
    return {};
}
QVariant AlignmentModel::headerData(int s,Qt::Orientation o,int role) const {
    if(role==Qt::DisplayRole) return o==Qt::Vertical?QVariant(document.records[s].name):QVariant(s+1);
    return {};
}
Qt::ItemFlags AlignmentModel::flags(const QModelIndex &i) const {
    auto f=QAbstractTableModel::flags(i);
    if(i.isValid()&&i.column()<document.records[i.row()].sequence.size()) f|=Qt::ItemIsEditable;
    return f;
}
bool AlignmentModel::setData(const QModelIndex &i,const QVariant &v,int role) {
    QString s=v.toString().trimmed().toUpper();
    if(role!=Qt::EditRole||!i.isValid()||s.size()!=1||!QString("ABCDEFGHIJKLMNOPQRSTUVWXYZ*?-").contains(s[0])) return false;
    if(document.alphabet!="Protein"&&!QString("ACGTURYSWKMBDHVN?-").contains(s[0])) return false;
    emit residueEdit(i.row(),i.column(),s[0]); return true;
}
void AlignmentModel::replace(const Document &d) {beginResetModel();document=d;columns_=helix::width(d);endResetModel();}
static QColor featureColor(int i) { static const QStringList c={"#7c5ce7","#16a590","#f39c52","#e46d97","#408bd0"};return QColor(c[i%c.size()]); }
void MapView::paintEvent(QPaintEvent *) {QPainter p(this);draw(p,rect());}
void MapView::draw(QPainter &p,const QRectF &bounds) const {
    p.save();
    p.setRenderHint(QPainter::Antialiasing,true);
    const QString theme=savedTheme();
    const bool dark=isDarkTheme(theme);
    const QColor bg(theme=="Dracula"?"#21222c":(dark?"#131820":"#ffffff"));
    const QColor foreground(dark?"#eef3fa":"#243247");
    const QColor muted(dark?"#97a7bb":"#72839a");
    const QColor line(dark?"#607088":"#a0b1c7");
    p.fillRect(bounds,bg);
    const QString seq=ungapped(record.sequence);
    const int n=int(seq.size());
    if(!n) {
        p.setPen(muted);
        p.drawText(bounds,Qt::AlignCenter,"Open a DNA or protein sequence to see its map");
        p.restore();return;
    }
    if(!record.circular || alphabet=="Protein") {
        const qreal x=bounds.left()+55,y=bounds.center().y()-20,w=qMax<qreal>(50,bounds.width()-110);
        p.setPen(QPen(line,7,Qt::SolidLine,Qt::RoundCap));
        p.drawLine(QPointF(x,y),QPointF(x+w,y));
        p.setPen(foreground);
        p.drawText(QRectF(x,y-62,w,30),Qt::AlignCenter,
            record.name+"  ·  "+QLocale().toString(n)+(alphabet=="Protein"?" aa":" bp")+"  ·  LINEAR");
        for(int j=0;j<=5;++j) {
            const qreal xx=x+w*j/5.;
            p.setPen(QPen(line,1));p.drawLine(QPointF(xx,y-9),QPointF(xx,y+9));
            p.setPen(muted);p.drawText(QRectF(xx-38,y+15,76,20),Qt::AlignCenter,
                QString::number(j?int(n*j/5.):1));
        }
        if(showFeatures)for(int i=0;i<record.features.size();++i) {
            const auto &f=record.features[i];
            const qreal left=x+w*f.start/n,right=x+w*f.end/n, yy=y+56+(i%4)*29;
            if(right<=left)continue;
            const QColor color=featureColor(i);
            const qreal tip=qMin<qreal>(9,(right-left)*.4);
            QPolygonF polygon;
            if(f.reverse)polygon<<QPointF(right,yy-7)<<QPointF(left+tip,yy-7)
                <<QPointF(left,yy)<<QPointF(left+tip,yy+7)<<QPointF(right,yy+7);
            else polygon<<QPointF(left,yy-7)<<QPointF(right-tip,yy-7)
                <<QPointF(right,yy)<<QPointF(right-tip,yy+7)<<QPointF(left,yy+7);
            p.setPen(Qt::NoPen);p.setBrush(color);p.drawPolygon(polygon);
            p.setPen(foreground);
            p.drawText(QRectF(left,yy+8,qMax<qreal>(70,right-left),17),Qt::AlignLeft,
                f.name.left(22));
        }
        p.setBrush(Qt::NoBrush);p.restore();return;
    }
    const QPointF center=bounds.center();
    const qreal radius=qMax<qreal>(64,qMin(bounds.width()*.245,bounds.height()*.29)*zoom);
    const QRectF ring(center.x()-radius,center.y()-radius,radius*2,radius*2);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(dark?QColor("#56667b"):QColor("#b4c2d5"),13));
    p.drawEllipse(ring);
    p.setPen(QPen(bg,5));p.drawEllipse(ring);
    p.setPen(QPen(line,1));p.drawEllipse(ring.adjusted(-9,-9,9,9));
    p.drawEllipse(ring.adjusted(9,9,-9,-9));
    for(int j=0;j<8;++j) {
        const qreal angle=j*2*M_PI/8-M_PI/2;
        const QPointF unit(std::cos(angle),std::sin(angle));
        p.setPen(QPen(line,1));
        p.drawLine(center+unit*(radius-8),center+unit*(radius+8));
        p.setPen(muted);
        // Keep coordinate ticks inside the ring when enzyme callouts occupy the exterior.
        if(!showEnzymes || j%2==0) {
            const QPointF label=center+unit*(showEnzymes?radius-21:radius+24);
            p.drawText(QRectF(label.x()-23,label.y()-9,46,18),Qt::AlignCenter,
                QString::number(j?int(n*j/8.):1));
        }
    }
    if(showFeatures) {
        for(int i=0;i<record.features.size();++i) {
            const auto &f=record.features[i];
            if(f.end<=f.start)continue;
            const QColor color=featureColor(i);
            const double a=90.-360.*f.start/n;
            const double span=-360.*(f.end-f.start)/n;
            p.setPen(QPen(color,15,Qt::SolidLine,Qt::FlatCap));
            p.drawArc(ring,int(a*16),int(span*16));
            const double theta=(f.reverse?f.start:f.end)*2*M_PI/n-M_PI/2;
            const QPointF radial(std::cos(theta),std::sin(theta));
            QPointF tangent(-std::sin(theta),std::cos(theta));
            if(f.reverse)tangent=-tangent;
            const QPointF end=center+radial*radius;
            p.setPen(Qt::NoPen);p.setBrush(color);
            p.drawPolygon(QPolygonF{end+tangent*9,
                end-tangent*6+radial*9,end-tangent*6-radial*9});
            // On compact maps, prioritize legible sequence name over overlapping feature labels.
            // All features are listed with complete names and coordinates in the Features tab.
            if(radius>=205 && f.end-f.start>=n/8) {
                const qreal mid=(f.start+f.end)*M_PI/n-M_PI/2;
                const QPointF midpoint=center+QPointF(std::cos(mid),std::sin(mid))*(radius-40);
                QFont labelFont=p.font();labelFont.setPointSize(8);labelFont.setBold(true);
                p.setFont(labelFont);p.setPen(dark?QColor("#f2f2fb"):QColor("#2b3545"));
                const qreal w=qMin<qreal>(95,f.name.size()*6+12);
                p.drawText(QRectF(midpoint.x()-w/2,midpoint.y()-9,w,18),
                    Qt::AlignCenter,QFontMetrics(labelFont).elidedText(f.name,Qt::ElideRight,int(w)));
            }
        }
    }
    QFont centerFont=p.font();centerFont.setPointSize(11);centerFont.setBold(true);
    p.setFont(centerFont);p.setPen(foreground);
    const qreal centerWidth=qMin<qreal>(radius*1.05,160);
    const QString centerTitle=QFontMetrics(centerFont).elidedText(record.name,
        Qt::ElideMiddle,int(centerWidth));
    p.drawText(QRectF(center.x()-centerWidth/2,center.y()-27,centerWidth,24),
        Qt::AlignCenter,centerTitle);
    centerFont.setPointSize(9);centerFont.setBold(false);
    p.setFont(centerFont);p.setPen(muted);
    p.drawText(QRectF(center.x()-centerWidth/2,center.y()+2,centerWidth,20),
        Qt::AlignCenter,QLocale().toString(n)+" bp  ·  circular");
    if(showEnzymes && alphabet!="Protein") {
        struct Callout { QString title; QPointF anchor; bool right; };
        QVector<Callout> left,right;
        const auto sites=findRestrictionSites(record);
        constexpr int maxLabels=24;
        for(int i=0;i<sites.size() && i<maxLabels;++i) {
            const auto &site=sites[i];
            const qreal theta=site.start*2*M_PI/n-M_PI/2;
            const QPointF radial(std::cos(theta),std::sin(theta));
            const QPointF anchor=center+radial*(radius+10);
            const QString title=site.enzyme+"  ("+QString::number(site.start+1)+")";
            (radial.x()>=0?right:left).append({title,anchor,radial.x()>=0});
        }
        QFont labelFont=p.font();labelFont.setPointSize(9);p.setFont(labelFont);
        const QFontMetrics metrics(labelFont);
        auto paintLabels=[&](QVector<Callout> &labels,bool rightSide) {
            std::sort(labels.begin(),labels.end(),[](const Callout &a,const Callout &b){
                return a.anchor.y()<b.anchor.y();
            });
            const qreal x=center.x()+(rightSide?radius+41:-radius-41);
            qreal previous=bounds.top()+14;
            for(const auto &label:labels) {
                const qreal y=qMax(previous,label.anchor.y());
                if(y>bounds.bottom()-14)break;
                const int tw=metrics.horizontalAdvance(label.title)+8;
                const qreal textX=rightSide?x+5:x-tw-5;
                p.setPen(QPen(dark?QColor("#718095"):QColor("#8796ac"),1));
                p.drawLine(label.anchor,QPointF(x,y));
                p.setPen(QPen(dark?QColor("#62d5c1"):QColor("#356fe4"),2));
                p.drawPoint(label.anchor);
                p.setPen(foreground);
                p.drawText(QRectF(textX,y-10,tw,20),
                    rightSide?Qt::AlignLeft:Qt::AlignRight,label.title);
                previous=y+17;
            }
        };
        paintLabels(left,false);paintLabels(right,true);
        if(sites.size()>maxLabels) {
            p.setPen(muted);
            p.drawText(QRectF(bounds.left()+12,bounds.bottom()-31,bounds.width()-24,20),
                Qt::AlignLeft,"Showing first 24 recognition sites of "+QString::number(sites.size())+
                "; open Enzymes for the complete list");
        }
    }
    p.restore();
}
class Change : public QUndoCommand {
    Editor *editor_; Document before_,after_;
public:
    Change(Editor *e,Document before,Document after,const QString &label):editor_(e),before_(std::move(before)),after_(std::move(after)){setText(label);}
    void undo() override {editor_->applyDocument(before_);}
    void redo() override {editor_->applyDocument(after_);}
};
Editor::Editor(bool preview):preview_(preview) {
    setWindowTitle("HelixEdit");resize(1260,780);setMinimumSize(900,600);
    model=new AlignmentModel(this);history=new QUndoStack(this);history->setUndoLimit(30);
    applyTheme(savedTheme());
    auto fileMenu=menuBar()->addMenu("&File");
    fileMenu->addAction("New project / workspace…",QKeySequence::New,this,[this]{
        if(busy())return;
        const QString dir=QFileDialog::getExistingDirectory(this,"Choose project workspace directory");
        if(dir.isEmpty())return;
        bool ok=false;
        QString name=QInputDialog::getText(this,"New HelixEdit project","Project name",QLineEdit::Normal,"Untitled project",&ok).trimmed();
        if(!ok||name.isEmpty())return;
        name.replace(QRegularExpression("[^A-Za-z0-9 _.-]"),"_");
        if(name=="."||name=="..")return;
        const QString projectDir=QDir(dir).filePath(name);
        const QString projectPath=QDir(projectDir).filePath(name+".helix");
        if(QFileInfo::exists(projectPath)||QFileInfo::exists(projectDir)){
            QMessageBox::warning(this,"Project exists","Choose a new project name to avoid overwriting existing files.");
            return;
        }
        // Save the current document before replacing it; cancellation preserves it.
        if(!history->isClean() && !save())return;
        if(!QDir().mkpath(projectDir)){
            QMessageBox::warning(this,"Workspace error","Could not create the project directory.");
            return;
        }
        Document blank;blank.alphabet="DNA";
        Record initial;initial.name="Sequence 1";initial.sequence="";
        blank.records.append(initial);
        try {
            writeProject(projectPath,blank);
            setDocument(blank);
            path_=projectPath;
            setWindowTitle("HelixEdit — "+name+"[*]");
            statusBar()->showMessage("New project saved automatically: "+projectPath);
        } catch(const std::exception &e) {
            QMessageBox::warning(this,"Project creation failed",QString::fromUtf8(e.what()));
        }
    });
    auto actOpen=fileMenu->addAction("Open…",QKeySequence::Open,this,&Editor::open);
    auto actSave=fileMenu->addAction("Save project",QKeySequence::Save,this,[this]{save();});
    fileMenu->addAction("Save project as…",QKeySequence::SaveAs,this,[this]{save(true);});
    auto examplesMenu=fileMenu->addMenu("Open teaching examples");
    examplesMenu->setObjectName("exampleMenu");
    auto addSample=[this,examplesMenu](const QString &label,
                                      const QString &resource,
                                      const QString &alphabet,
                                      const QString &note){
        auto action=examplesMenu->addAction(label);
        connect(action,&QAction::triggered,this,[this,resource,alphabet,note]{
            if(!canReplace())return;
            runSafely([&]{
                QFile input(resource);
                if(!input.open(QIODevice::ReadOnly))
                    throw std::runtime_error("The selected example is missing from the application resources.");
                Document d=parseFasta(QString::fromUtf8(input.readAll()));
                d.alphabet=alphabet;
                validateDocument(d);
                d.warnings.clear();
                d.warnings << note;
                setDocument(d);
                statusBar()->showMessage("Loaded bundled "+alphabet+" example · Save as a .helix project to keep your edits.");
            });
        });
    };
    addSample("DNA · six orchid fragments (300 nt)",
        ":/examples/samples/dna_orchid_6.fasta","DNA",
        "DNA demonstration: six 300-nt fragments from public orchid ITS/5.8S-containing records; see examples/samples/README.md.");
    addSample("RNA · three simulated transcripts (300 nt)",
        ":/examples/samples/rna_transcripts_3.fasta","RNA",
        "RNA demonstration: in-silico T-to-U conversions of DNA fragments. Not validated mature RNA.");
    addSample("Protein · five UniProt entries (110–396 aa)",
        ":/examples/samples/proteins_5.fasta","Protein",
        "Protein demonstration: five UniProt sequences are not necessarily homologous; no phylogenetic interpretation.");
    fileMenu->addSeparator();
    auto addgeneMenu=fileMenu->addMenu("Addgene plasmids");
    addgeneMenu->addAction("Find plasmid by Addgene ID…",this,[this]{
        bool ok=false;
        const int id=QInputDialog::getInt(this,"Addgene plasmid","Addgene plasmid ID",10878,1,999999999,1,&ok);
        if(!ok)return;
        const QUrl url(QString("https://www.addgene.org/%1/").arg(id));
        QDesktopServices::openUrl(url);
        QMessageBox::information(this,"Import from Addgene",
            "Sign in to your Addgene account in the browser, then download the GenBank (.gb/.gbk) sequence from the plasmid page.\n\n"
            "Use 'Import Addgene GenBank into project…' to add it without replacing your current alignment. "
            "Some plasmids have only partial sequences; check the source and annotations.");
    });
    addgeneMenu->addAction("Search Addgene catalog…",this,[]{
        QDesktopServices::openUrl(QUrl("https://www.addgene.org/search/catalog/plasmids/"));
    });
    addgeneMenu->addAction("Import Addgene GenBank into project…",this,[this]{
        if(busy())return;
        const QString p=QFileDialog::getOpenFileName(this,"Import downloaded Addgene GenBank",{},
            "GenBank (*.gb *.gbk *.genbank);;All files (*)");
        if(p.isEmpty())return;
        runSafely([&]{
            const Document imported=readDocument(p);
            if(imported.alphabet=="Protein")
                throw std::runtime_error("Addgene plasmid import expects nucleotide sequence.");
            if(imported.records.isEmpty())
                throw std::runtime_error("No plasmid records were found.");
            if(model->document.alphabet=="Protein")
                throw std::runtime_error("Open a DNA/RNA project before importing plasmids.");
            auto next=model->document;
            // Do not mix RNA and DNA molecule types silently.
            if(next.alphabet!=imported.alphabet && !next.records.isEmpty()) {
                if(QMessageBox::question(this,"Molecule type",
                    "Imported sequence type differs from this project. Continue only if the sequences are compatible?",
                    QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return;
            }
            for(auto record:imported.records) {
                const QString originalName=record.name;
                int suffix=2;
                auto taken=[&](const QString &name){
                    for(const auto &existing:next.records)if(existing.name==name)return true;
                    return false;
                };
                while(taken(record.name))record.name=originalName+QString(" (%1)").arg(suffix++);
                next.records.append(record);
            }
            next.warnings+=imported.warnings;
            next.warnings.append("Imported from a user-selected GenBank file. Verify full-length sequence and annotations against the Addgene source.");
            commit(next,"Import Addgene GenBank");
            statusBar()->showMessage(QString("Imported %1 plasmid record(s); save your .helix project.").arg(imported.records.size()));
        });
    });
    fileMenu->addAction("Export FASTA / CLUSTAL…",this,&Editor::exportFile);
    fileMenu->addAction("Export annotated GenBank…",this,[this]{
        const QString path=QFileDialog::getSaveFileName(this,"Export GenBank",{},"GenBank (*.gb)");
        if(path.isEmpty())return;
        runSafely([&]{
            const QString output=genbank(model->document);
            const QString destination=path.endsWith(".gb",Qt::CaseInsensitive)?path:path+".gb";
            QSaveFile file(destination);
            const QByteArray bytes=output.toUtf8();
            if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())
                throw std::runtime_error("Failed to write GenBank file.");
            statusBar()->showMessage("Exported annotated GenBank: "+destination);
        });
    });

    fileMenu->addAction("Export map as SVG…",this,[this]{auto p=QFileDialog::getSaveFileName(this,"Export vector map",{},"SVG (*.svg)");if(!p.isEmpty())runSafely([&]{exportMap(p);});});
    fileMenu->addAction("Load teaching demo",this,[this]{if(canReplace())setDocument(demo());});
    fileMenu->addSeparator();fileMenu->addAction("Quit",QKeySequence::Quit,this,&QWidget::close);
    auto edit=menuBar()->addMenu("&Edit");
    auto undo=history->createUndoAction(this,"Undo");undo->setShortcut(QKeySequence::Undo);edit->addAction(undo);
    auto redo=history->createRedoAction(this,"Redo");redo->setShortcut(QKeySequence::Redo);edit->addAction(redo);
    edit->addAction("Add sequence…",this,&Editor::addSequence);
    auto copyAction=edit->addAction("Copy selection as FASTA",QKeySequence::Copy,this,[this]{
        auto ranges=table->selectionModel()->selection();if(ranges.size()!=1){statusBar()->showMessage("Select one rectangular block to copy as FASTA.");return;}
        const auto range=ranges.first();Document block;block.alphabet=model->document.alphabet;
        for(int row=range.top();row<=range.bottom();++row){auto r=model->document.records[row];r.sequence=r.sequence.mid(range.left(),range.width());r.features.clear();r.circular=false;if(!r.sequence.isEmpty())block.records.append(r);}
        QGuiApplication::clipboard()->setText(fasta(block));statusBar()->showMessage("Selected block copied as sequence-only FASTA.");
    });
    auto pasteAction=edit->addAction("Paste new FASTA records",QKeySequence::Paste,this,[this]{if(busy())return;runSafely([&]{auto incoming=parseFasta(QGuiApplication::clipboard()->text());auto d=model->document;d.records+=incoming.records;commit(d,"Paste sequences");});});
    edit->addAction("Rename active sequence…",this,[this]{if(busy())return;bool ok=false;int row=activeRow();auto name=QInputDialog::getText(this,"Rename sequence","Name",QLineEdit::Normal,model->document.records[row].name,&ok);if(ok)runSafely([&]{auto d=model->document;d.records[row].name=name.trimmed();commit(d,"Rename sequence");});});
    edit->addAction("Remove selected sequences…",this,[this]{
        if(busy())return;
        QSet<int> rows;
        for(const auto &range:table->selectionModel()->selection())
            for(int r=range.top();r<=range.bottom();++r)rows.insert(r);
        // A clicked residue may be the active sequence even without a selected row.
        if(rows.isEmpty() && table->currentIndex().isValid())
            rows.insert(table->currentIndex().row());
        if(rows.isEmpty()){
            QMessageBox::information(this,"Remove sequences","Select a sequence first.");return;
        }
        if(rows.size()>=model->document.records.size()){
            QMessageBox::information(this,"Remove sequences",
                "At least one sequence must remain in the current document.");return;
        }
        const QString message=QString("Remove %1 selected sequence(s)?\n\nThis can be undone.")
                                  .arg(rows.size());
        if(QMessageBox::question(this,"Remove sequences",message)!=QMessageBox::Yes)return;
        auto d=model->document;
        auto sorted=rows.values();
        std::sort(sorted.begin(),sorted.end(),std::greater<int>());
        for(int row:sorted)d.records.removeAt(row);
        runSafely([&]{commit(d,"Remove sequences");});
    });

    edit->addAction("Insert gap at selected column",QKeySequence("Ctrl+I"),this,[this]{gapColumns(true);});
    edit->addAction("Delete selected columns…",this,[this]{gapColumns(false);});
    edit->addAction("Reverse complement selected rows",this,&Editor::reverseSelected);
    auto analysis=menuBar()->addMenu("&Analysis");
    analysis->addAction("Translate active sequence…",this,&Editor::translateSelected);
    analysis->addAction("Find open reading frames…",this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"ORF discovery","Select a DNA or RNA project first.");
            return;
        }
        bool ok=false;
        const int minimum=QInputDialog::getInt(this,"Find ORFs","Minimum amino acids",30,0,100000,1,&ok);
        if(!ok)return;
        runSafely([&]{
            const auto &record=model->document.records[activeRow()];
            const auto orfs=findOrfs(record.sequence,minimum);
            QString report="Sequence: "+record.name+"\nCoordinates: 1-based inclusive, forward reference\nComplete ATG-start ORFs only\n\n";
            if(orfs.isEmpty())report+="No matching ORFs found.";
            for(const auto &orf:orfs) {
                report+=QString("Frame %1  |  %2–%3  |  %4 aa\n%5\n\n")
                    .arg(orf.frame).arg(orf.start+1).arg(orf.end)
                    .arg(orf.protein.size()).arg(orf.protein);
            }
            showAnalysis("Open reading frames",report);
        });
    });

    analysis->addAction("Consensus",this,[this]{showAnalysis("Consensus",">majority_consensus\n"+consensus(model->document)+"\n\nGaps excluded. Ties → N (DNA/RNA), X (protein). No frequency threshold.");});
    analysis->addAction("Pairwise identity (first two selected rows)",this,[this]{
        runSafely([&]{QSet<int> rows;for(const auto &range:table->selectionModel()->selection())for(int r=range.top();r<=range.bottom();++r)rows.insert(r);auto sorted=rows.values();std::sort(sorted.begin(),sorted.end());
        if(sorted.size()!=2)throw std::runtime_error("Select exactly two rows (click row headers with Ctrl/Cmd).");
        auto &a=model->document.records[sorted[0]],&b=model->document.records[sorted[1]];
        showAnalysis("Aligned identity",a.name+"\n"+b.name+"\n\n"+number(identity(a.sequence,b.sequence))+"\nExact matching symbols / columns where neither sequence is a gap. Ambiguity symbols compare literally.");});
    });
    analysis->addAction("Common restriction sites",this,[this]{
        if(model->document.alphabet=="Protein"){QMessageBox::information(this,"Restriction sites","Select a nucleotide document.");return;}
        auto r=model->document.records[activeRow()];auto s=ungapped(r.sequence);s.replace('U','T');QString out=r.name+"\nRecognition-site starts (1-based); exact matches only.\n";
        QMap<QString,QString> motifs={{"EcoRI","GAATTC"},{"BamHI","GGATCC"},{"HindIII","AAGCTT"},{"NotI","GCGGCCGC"}};
        for(auto it=motifs.cbegin();it!=motifs.cend();++it){QStringList hits;QString scan=s;if(r.circular)scan+=s.left(it.value().size()-1);for(int i=0;i<s.size();++i)if(scan.mid(i,it.value().size())==it.value())hits<<QString::number(i+1);out+="\n"+it.key()+" ("+it.value()+"): "+(hits.isEmpty()?"none":hits.join(", "));}showAnalysis("Restriction sites",out);
    });
    analysis->addAction("Pairwise identity matrix…",this,[this]{
        runSafely([&]{
            const auto matrix=pairwiseIdentityMatrix(model->document);
            QString result="Aligned identity (%), excluding columns with gaps in either sequence.\n"
                           "Undefined comparisons are marked NA.\n\n";
            for(int i=0;i<matrix.size();++i){
                result+=model->document.records[i].name+": ";
                for(int j=0;j<matrix[i].size();++j)
                    result+=(matrix[i][j]<0?"NA":QString::number(matrix[i][j],'f',2))+
                        (j+1==matrix[i].size()?"":"  |  ");
                result+="\n";
            }
            showAnalysis("Pairwise identity matrix",result);
        });
    });
    analysis->addAction("Sequence statistics…",this,[this]{
        const auto &d=model->document;
        QString report="SEQUENCE STATISTICS\n";
        report+="Molecule type: "+d.alphabet+"\nRecord count: "+QString::number(d.records.size())+
                "\nAlignment width: "+QString::number(helix::width(d))+"\n\n";
        for(const auto &r:d.records){
            const auto x=sequenceStatistics(r,d.alphabet);
            report+=QString("%1\n  ungapped=%2  columns=%3  gaps=%4  ambiguous=%5  GC=%6\n")
                .arg(r.name).arg(x.ungappedLength).arg(x.alignmentLength)
                .arg(x.gaps).arg(x.ambiguous)
                .arg(x.gc<0?"NA":QString::number(x.gc,'f',2)+"%");
        }
        showAnalysis("Sequence statistics",report);
    });
    analysis->addAction("p-distance matrix…",this,[this]{
        runSafely([&]{
            const auto matrix=pDistanceMatrix(model->document);
            QString report="Uncorrected p-distance: mismatches / compared non-gap, non-? columns.\n"
                           "Ambiguous symbols are compared literally. NA = no comparable columns.\n\n";
            for(int i=0;i<matrix.size();++i){
                report+=model->document.records[i].name+": ";
                for(int j=0;j<matrix[i].size();++j)
                    report+=(matrix[i][j]<0?"NA":QString::number(matrix[i][j],'f',5))+
                            (j+1==matrix[i].size()?"":"  |  ");
                report+="\n";
            }
            showAnalysis("p-distance matrix",report);
        });
    });
    analysis->addAction("Export PHYLIP for phylogenetics…",this,[this]{
        const QString path=QFileDialog::getSaveFileName(this,"Export aligned PHYLIP",{},"PHYLIP (*.phy)");
        if(path.isEmpty())return;
        runSafely([&]{
            const QByteArray bytes=phylipSequential(model->document).toUtf8();
            const QString destination=path.endsWith(".phy",Qt::CaseInsensitive)?path:path+".phy";
            QSaveFile file(destination);
            if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())
                throw std::runtime_error("Could not export PHYLIP.");
            QMessageBox::information(this,"PHYLIP export",
                "Exported strict sequential PHYLIP with stable HX identifiers. "
                "Sequence names and annotations are not included; retain your .helix project.");
        });
    });
    auto cloningMenu=menuBar()->addMenu("&Cloning");
    cloningMenu->addAction("Assemble selected fragments by exact overlap…",this,[this]{
        if(busy()||model->document.alphabet=="Protein")return;
        QSet<int> selected;
        for(const auto &range:table->selectionModel()->selection())
            for(int i=range.top();i<=range.bottom();++i)selected.insert(i);
        auto rows=selected.values();std::sort(rows.begin(),rows.end());
        if(rows.size()!=2){
            QMessageBox::information(this,"Overlap assembly",
                "Select exactly two sequence rows in the desired 5′ to 3′ order.");return;
        }
        bool ok=false;
        int minimum=QInputDialog::getInt(this,"Overlap assembly",
            "Minimum exact overlap (bp)",15,1,1000,1,&ok);
        if(!ok)return;
        const bool circular=QMessageBox::question(this,"Topology",
            "Attempt to circularize the assembled product using an additional terminal overlap?",
            QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes;
        runSafely([&]{
            const auto &left=model->document.records[rows[0]];
            const auto &right=model->document.records[rows[1]];
            auto assembled=assembleOverlap(left.sequence,right.sequence,minimum,circular);
            auto next=model->document;
            const QString name=QString("assembly_%1_%2").arg(rows[0]+1).arg(rows[1]+1);
            next.records.append(Record{name,assembled.sequence,{},assembled.circular});
            next.warnings.append("Assembly is an exact sequence overlap preview; no biochemical feasibility or junction fidelity is guaranteed.");
            commit(next,"Assemble overlapping fragments");
            showAnalysis("Assembly preview",QString("Product: %1\nLength: %2 bp\n"
                "Junction overlap: %3 bp\nCircular: %4\n\n"
                "No feature annotations were transferred; verify the product before laboratory use.")
                .arg(name).arg(assembled.sequence.size()).arg(assembled.overlap)
                .arg(assembled.circular?"yes":"no"));
        });
    });
    auto tools=menuBar()->addMenu("&Tools");
    tools->addAction("Align with MAFFT",this,[this]{align("mafft");});
    tools->addAction("Align with MUSCLE 5",this,[this]{align("muscle");});
    auto blastAction=tools->addAction("BLAST+ against local database…",this,[this]{
        if(busy())return;
        const QString kind=model->document.alphabet=="Protein"?"blastp":"blastn";
        QSettings settings;
        QString exe=settings.value("tools/"+kind).toString();
        if(exe.isEmpty())exe=QStandardPaths::findExecutable(kind);
#ifdef Q_OS_MACOS
        if(exe.isEmpty()&&QFileInfo::exists("/opt/homebrew/bin/"+kind))
            exe="/opt/homebrew/bin/"+kind;
#endif
        if(exe.isEmpty()||!QFileInfo(exe).isExecutable()){
            QMessageBox::information(this,"BLAST+ not found",
                "Install NCBI BLAST+ and configure the "+kind+" executable in Tools → Configure executables.");
            return;
        }
        bool ok=false;
        const QString db=QInputDialog::getText(this,"Local BLAST database",
            "Database prefix (created with makeblastdb; no shell commands):",
            QLineEdit::Normal,settings.value("blast/database").toString(),&ok).trimmed();
        if(!ok||db.isEmpty())return;
        if(!QFileInfo::exists(db+".nin")&&!QFileInfo::exists(db+".pin")&&
           !QFileInfo::exists(db+".ndb")&&!QFileInfo::exists(db+".pdb")){
            QMessageBox::warning(this,"BLAST database",
                "No recognizable BLAST index found at this prefix. "
                "Create a nucleotide/protein database with makeblastdb first.");
            return;
        }
        settings.setValue("blast/database",db);
        const auto &record=model->document.records[activeRow()];
        const QString query=ungapped(record.sequence).toUpper();
        if(query.isEmpty()||query.contains(QRegularExpression(
            model->document.alphabet=="Protein"?"[^A-Z*]":"[^ACGTURYSWKMBDHVN]"))){
            QMessageBox::warning(this,"BLAST query","Active sequence contains unsupported symbols.");return;
        }
        QTemporaryDir dir;
        if(!dir.isValid()){QMessageBox::warning(this,"BLAST","Cannot create temporary workspace.");return;}
        const QString queryPath=dir.filePath("query.fa");
        QFile file(queryPath);
        if(!file.open(QIODevice::WriteOnly)){QMessageBox::warning(this,"BLAST","Cannot write query.");return;}
        const QByteArray input=(">helix_query\n"+query+"\n").toUtf8();
        file.write(input);file.close();
        const QStringList args={"-query",queryPath,"-db",db,"-outfmt",
            "6 qseqid sseqid pident length mismatch gapopen qstart qend sstart send evalue bitscore",
            "-max_target_seqs","50"};
        QProcess job;
        job.setProgram(exe);
        job.setArguments(args);
        job.setWorkingDirectory(dir.path());
        job.start();
        if(!job.waitForStarted(5000)){QMessageBox::warning(this,"BLAST","Unable to start BLAST+.");return;}
        if(!job.waitForFinished(60000)){
            job.kill();job.waitForFinished();
            QMessageBox::warning(this,"BLAST","Search exceeded the 60-second preview limit.");return;
        }
        const QString stdoutText=QString::fromUtf8(job.readAllStandardOutput());
        const QString stderrText=QString::fromUtf8(job.readAllStandardError());
        const QString hash=QString::fromLatin1(QCryptographicHash::hash(input,QCryptographicHash::Sha256).toHex());
        const QString log=QString("UTC: %1\nTool: %2\nExecutable: %3\n"
            "Database: %4\nQuery SHA256: %5\nArguments: %6\nExit code: %7\n"
            "STDERR:\n%8\nRESULTS (BLAST tabular outfmt 6):\n%9\n")
            .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),kind,exe,db,hash,
                 args.join(" "),QString::number(job.exitCode()),stderrText,stdoutText);
        const QString logFolder=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(logFolder);
        QFile audit(QDir(logFolder).filePath("analysis-history.log"));
        if(audit.open(QIODevice::WriteOnly|QIODevice::Append)){
            audit.write(("\n===== BLAST+ RUN =====\n"+log).toUtf8());
            audit.close();
        }
        if(log_)log_->appendPlainText(log);
        if(job.exitStatus()!=QProcess::NormalExit||job.exitCode()!=0)
            QMessageBox::warning(this,"BLAST+ failed",stderrText.left(2000));
        else showAnalysis("BLAST+ local results",log);
    });
    tools->addAction("View reproducible analysis history…",this,[this]{
        const QString path=QDir(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation)).filePath("analysis-history.log");
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)){
            QMessageBox::information(this,"Analysis history","No completed alignment or BLAST+ runs have been recorded yet.");
            return;
        }
        const QByteArray bytes=file.readAll();
        showAnalysis("Local analysis history",QString::fromUtf8(bytes.right(1024*1024))+
            "\n\nHistory path: "+path+
            "\nOnly the last 1 MiB is displayed; the complete file remains on disk.");
    });
    auto settingsMenu=menuBar()->addMenu("&Settings");
    auto settingsAction=settingsMenu->addAction("Preferences…",this,&Editor::showSettings);
    settingsAction->setObjectName("openSettings");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsMenu->addAction("Update from GitHub source…",this,[this]{openSourceUpdater(this);});
    auto help=menuBar()->addMenu("&Help");
    help->addAction("About HelixEdit",this,[this]{
        QDialog dialog(this);
        dialog.setObjectName("helixAboutDialog");
        dialog.setWindowTitle("About HelixEdit");
        dialog.setMinimumWidth(475);
        auto layout=new QVBoxLayout(&dialog);
        layout->setSpacing(12);
        auto heading=new QLabel("<span style='font-size:23px;font-weight:700'>⬡ HelixEdit</span>   "
            "<span style='font-size:12px'>"+QCoreApplication::applicationVersion()+" beta</span>");
        layout->addWidget(heading);
        auto description=new QLabel("Compact, offline-first DNA, RNA, protein and plasmid sequence studio.");
        description->setWordWrap(true);layout->addWidget(description);
        auto credit=new QLabel("<b>Developed and edited by Wael H</b><br>"
            "Contact: <a href='mailto:waelye@gmail.com'>waelye@gmail.com</a><br>"
            "GitHub repository: <a href='https://github.com/mac860/HelixEdit'>"
            "https://github.com/mac860/HelixEdit</a>");
        credit->setTextFormat(Qt::RichText);
        credit->setTextInteractionFlags(Qt::TextBrowserInteraction);
        credit->setOpenExternalLinks(true);
        credit->setWordWrap(true);layout->addWidget(credit);
        auto disclaimer=new QLabel("Independent open-source beta, not affiliated with BioEdit or "
            "SnapGene. Validate scientific analyses before research use. "
            "No proprietary third-party application assets are included.");
        disclaimer->setWordWrap(true);layout->addWidget(disclaimer);
        auto buttons=new QDialogButtonBox(QDialogButtonBox::Close);
        connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        layout->addWidget(buttons);
        dialog.exec();
    });
    auto bar=addToolBar("Quick actions");
    bar->setObjectName("quickActions");
    bar->setMovable(false);
    bar->setIconSize(QSize(17,17));
    bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto brand=new QLabel("<b style='font-size:15px'>⬡ HelixEdit</b>  ");
    bar->addWidget(brand);
    actOpen->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    actSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    undo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    redo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    copyAction->setText("Copy FASTA");
    copyAction->setToolTip("Copy the selected alignment rectangle as sequence-only FASTA");
    pasteAction->setText("Paste FASTA");
    pasteAction->setToolTip("Append FASTA records from the clipboard");
    copyAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    pasteAction->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    bar->addAction(actOpen);bar->addAction(actSave);bar->addSeparator();
    bar->addAction(undo);bar->addAction(redo);bar->addSeparator();
    bar->addAction(copyAction);bar->addAction(pasteAction);
    auto exampleButton=new QToolButton(bar);exampleButton->setText("Examples");
    exampleButton->setToolTip("Load attributed offline teaching sequences");
    exampleButton->setMenu(examplesMenu);exampleButton->setPopupMode(QToolButton::InstantPopup);
    bar->addWidget(exampleButton);
    auto addQuick=bar->addAction("+ Sequence",this,&Editor::addSequence);
    addQuick->setToolTip("Add a DNA, RNA or protein record");
    auto spacer=new QWidget;spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    bar->addWidget(spacer);
    auto offline=new QLabel("<b style='color:#35ae98'>● LOCAL</b>   <span style='color:#8d9bac'>BETA</span>  ");
    bar->addWidget(offline);
    auto quickUpdater=bar->addAction("Update",this,[this]{openSourceUpdater(this);});
    quickUpdater->setToolTip("Check GitHub main, then build, test and install from your local checkout");
    quickUpdater->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    auto root=new QWidget;setCentralWidget(root);
    auto layout=new QVBoxLayout(root);
    layout->setContentsMargins(6,6,6,4);layout->setSpacing(5);
    // Compact, category-based ribbon: each button triggers the existing menu action,
    // keeping keyboard shortcuts, enablement and behavior in one place.
    auto ribbon=new QTabWidget(root);
    ribbon->setObjectName("commandRibbon");
    ribbon->setDocumentMode(true);
    ribbon->setMaximumHeight(79);
    ribbon->setMinimumHeight(67);
    auto addRibbonCategory=[&](const QString &name,
                               const QVector<QPair<QString,QAction*>> &commands){
        auto page=new QWidget(ribbon);
        page->setObjectName("ribbonPage");
        auto row=new QHBoxLayout(page);
        row->setContentsMargins(4,2,4,2);
        row->setSpacing(2);
        for(const auto &command:commands){
            if(!command.second)continue;
            auto button=new QToolButton(page);
            button->setText(command.first);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setIconSize(QSize(15,15));
            button->setMinimumWidth(56);
            button->setMaximumWidth(126);
            button->setAutoRaise(true);
            button->setDefaultAction(command.second);
            button->setText(command.first);
            button->setToolTip(command.second->text()+"\n"+command.second->toolTip());
            row->addWidget(button);
        }
        row->addStretch(1);
        ribbon->addTab(page,name);
    };
    auto findMenuAction=[](QMenu *menu,const QString &label)->QAction*{
        for(auto *action:menu->actions())if(action->text()==label)return action;
        return nullptr;
    };
    auto setRibbonIcon=[this](QAction *action,QStyle::StandardPixmap icon){
        if(action && action->icon().isNull())action->setIcon(style()->standardIcon(icon));
    };
    // Keep the existing menu actions as the single command source.
    auto actionFor=[&](QMenu *menu,const QString &name,QStyle::StandardPixmap icon){
        QAction *a=findMenuAction(menu,name);
        setRibbonIcon(a,icon);
        if(a)a->setToolTip(name);
        return a;
    };
    auto openA=actionFor(fileMenu,"Open…",QStyle::SP_DialogOpenButton);
    auto saveA=actionFor(fileMenu,"Save project",QStyle::SP_DialogSaveButton);
    auto exportA=actionFor(fileMenu,"Export FASTA / CLUSTAL…",QStyle::SP_DialogSaveButton);
    auto gbExportA=actionFor(fileMenu,"Export annotated GenBank…",QStyle::SP_DialogSaveButton);
    auto addA=actionFor(edit,"Add sequence…",QStyle::SP_FileIcon);
    auto renameA=actionFor(edit,"Rename active sequence…",QStyle::SP_FileDialogDetailedView);
    auto removeA=actionFor(edit,"Remove selected sequences…",QStyle::SP_TrashIcon);
    auto reverseA=actionFor(edit,"Reverse complement selected rows",QStyle::SP_BrowserReload);
    auto gapA=actionFor(edit,"Insert gap at selected column",QStyle::SP_ArrowRight);
    auto translateA=actionFor(analysis,"Translate active sequence…",QStyle::SP_MediaPlay);
    auto orfA=actionFor(analysis,"Find open reading frames…",QStyle::SP_FileDialogDetailedView);
    auto consensusA=actionFor(analysis,"Consensus",QStyle::SP_DialogApplyButton);
    auto restrictA=actionFor(analysis,"Common restriction sites",QStyle::SP_FileDialogContentsView);
    auto assemblyA=actionFor(cloningMenu,"Assemble selected fragments by exact overlap…",QStyle::SP_FileDialogDetailedView);
    auto matrixA=actionFor(analysis,"Pairwise identity matrix…",QStyle::SP_FileDialogContentsView);
    auto statsA=actionFor(analysis,"Sequence statistics…",QStyle::SP_FileDialogDetailedView);
    auto distanceA=actionFor(analysis,"p-distance matrix…",QStyle::SP_FileDialogContentsView);
    auto phyloA=actionFor(analysis,"Export PHYLIP for phylogenetics…",QStyle::SP_DialogSaveButton);
    auto blastA=actionFor(tools,"BLAST+ against local database…",QStyle::SP_FileDialogContentsView);
    auto mafftA=actionFor(tools,"Align with MAFFT",QStyle::SP_BrowserReload);
    auto muscleA=actionFor(tools,"Align with MUSCLE 5",QStyle::SP_BrowserReload);
    auto addgeneA=actionFor(addgeneMenu,"Find plasmid by Addgene ID…",QStyle::SP_DirLinkIcon);
    auto importA=actionFor(addgeneMenu,"Import Addgene GenBank into project…",QStyle::SP_DialogOpenButton);
    setRibbonIcon(undo,QStyle::SP_ArrowBack);
    setRibbonIcon(redo,QStyle::SP_ArrowForward);
    addRibbonCategory("Sequence",{{"Add",addA},{"Rename",renameA},{"Remove",removeA},{"Insert gap",gapA},
                                  {"Reverse",reverseA},{"Translate",translateA},{"Find ORFs",orfA}});
    addRibbonCategory("Plasmid",{{"Addgene ID",addgeneA},{"Import GenBank",importA},
                                 {"Restriction",restrictA},{"Export GB",gbExportA},{"Assemble",assemblyA}});
    addRibbonCategory("Analysis",{{"MAFFT",mafftA},{"MUSCLE",muscleA},{"BLAST+",blastA},
                                  {"Consensus",consensusA},{"Matrix",matrixA},{"p-distance",distanceA},{"Statistics",statsA},{"PHYLIP",phyloA}});
    layout->addWidget(ribbon);
    auto strip=new QHBoxLayout;auto title=new QLabel("<b style='font-size:14px'>Workspace</b>");strip->addWidget(title);strip->addStretch();
    molecule_=new QComboBox;molecule_->addItems({"DNA","RNA","Protein"});molecule_->setToolTip("Verify molecule type; short proteins cannot always be inferred from letters.");strip->addWidget(molecule_);
    search_=new QLineEdit;search_->setPlaceholderText("Find nucleotide or peptide motif…");search_->setMaximumWidth(260);strip->addWidget(search_);
    auto find=new QPushButton("Find next");strip->addWidget(find);layout->addLayout(strip);
    notice_=new QLabel;notice_->setObjectName("infoNotice");notice_->setWordWrap(false);
    notice_->setMaximumHeight(25);notice_->setStyleSheet("background:#fff0d8;color:#86602b;border-radius:5px;padding:3px 8px;");layout->addWidget(notice_);
    auto split=new QSplitter(Qt::Horizontal,root);layout->addWidget(split,1);
    tabs_=new QTabWidget;tabs_->setObjectName("studioTabs");tabs_->setDocumentMode(true);
    tabs_->setMovable(false);split->addWidget(tabs_);
    auto alignment=new QWidget;auto al=new QVBoxLayout(alignment);al->setContentsMargins(0,8,0,0);
    auto legend=new QLabel("   <b style='color:#199c73'>A</b> adenine   <b style='color:#408bd0'>C</b> cytosine   <b style='color:#b18527'>G</b> guanine   <b style='color:#cf6281'>T/U</b> thymine / uracil   ·   Double-click a residue to edit");legend->setObjectName("residueLegend");al->addWidget(legend);
    table=new QTableView;table->setObjectName("alignment");table->setModel(model);table->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);table->horizontalHeader()->setMinimumSectionSize(24);table->horizontalHeader()->setDefaultSectionSize(28);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);table->verticalHeader()->setDefaultSectionSize(35);table->verticalHeader()->setFixedWidth(205);table->verticalHeader()->setTextElideMode(Qt::ElideRight);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);table->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);al->addWidget(table,1);
    auto actions=new QHBoxLayout;actions->setContentsMargins(10,0,10,10);
    for(auto item:QStringList{"+ Gap","Delete columns","Reverse complement","Translate"}) {
        auto b=new QPushButton(item);actions->addWidget(b);
        if(item=="+ Gap")connect(b,&QPushButton::clicked,this,[this]{gapColumns(true);});
        else if(item=="Delete columns")connect(b,&QPushButton::clicked,this,[this]{gapColumns(false);});
        else if(item=="Translate")connect(b,&QPushButton::clicked,this,&Editor::translateSelected);
        else connect(b,&QPushButton::clicked,this,&Editor::reverseSelected);
    }actions->addStretch();al->addLayout(actions);tabs_->addTab(alignment,"01  Alignment");
    auto mapPage=new QWidget;auto ml=new QVBoxLayout(mapPage);mapTitle_=new QLabel;mapTitle_->setStyleSheet("font-size:13px;font-weight:600;padding:4px;");ml->addWidget(mapTitle_);
    auto mapStrip=new QWidget(mapPage);mapStrip->setObjectName("mapToolStrip");
    auto mapStripLayout=new QHBoxLayout(mapStrip);
    mapStripLayout->setContentsMargins(5,1,5,1);mapStripLayout->setSpacing(8);
    auto enzymeLabels=new QCheckBox("Enzyme labels");
    enzymeLabels->setObjectName("mapEnzymeToggle");
    enzymeLabels->setChecked(QSettings("HelixEdit","HelixEdit").value("map/showEnzymes",true).toBool());
    mapEnzymeToggle_=enzymeLabels;
    enzymeLabels->setToolTip("Show exact recognition sites from the built-in eight-enzyme set");
    auto featureLabels=new QCheckBox("Features");
    featureLabels->setObjectName("mapFeatureToggle");
    featureLabels->setChecked(QSettings("HelixEdit","HelixEdit").value("map/showFeatures",true).toBool());
    mapFeatureToggle_=featureLabels;
    mapStripLayout->addWidget(enzymeLabels);mapStripLayout->addWidget(featureLabels);
    mapStripLayout->addStretch();
    auto zoomOut=new QToolButton(mapStrip);zoomOut->setObjectName("mapZoomOut");zoomOut->setText("−");
    zoomOut->setToolTip("Zoom out plasmid map");
    auto zoomReset=new QToolButton(mapStrip);zoomReset->setObjectName("mapZoomReset");zoomReset->setText("100%");
    zoomReset->setToolTip("Reset map zoom");
    auto zoomIn=new QToolButton(mapStrip);zoomIn->setObjectName("mapZoomIn");zoomIn->setText("+");
    zoomIn->setToolTip("Zoom in plasmid map");
    mapStripLayout->addWidget(new QLabel("Map zoom"));mapStripLayout->addWidget(zoomOut);
    mapStripLayout->addWidget(zoomReset);mapStripLayout->addWidget(zoomIn);
    ml->addWidget(mapStrip);
    map_=new MapView;map_->setObjectName("plasmidMap");
    map_->showEnzymes=enzymeLabels->isChecked();
    map_->showFeatures=featureLabels->isChecked();
    map_->setZoom(QSettings("HelixEdit","HelixEdit").value("map/defaultZoom",100).toInt()/100.0);
    ml->addWidget(map_,1);
    connect(enzymeLabels,&QCheckBox::toggled,map_,[this](bool on){
        QSettings("HelixEdit","HelixEdit").setValue("map/showEnzymes",on);
        map_->showEnzymes=on;map_->update();
    });
    connect(featureLabels,&QCheckBox::toggled,map_,[this](bool on){
        QSettings("HelixEdit","HelixEdit").setValue("map/showFeatures",on);
        map_->showFeatures=on;map_->update();
    });
    connect(zoomIn,&QToolButton::clicked,map_,[this]{map_->setZoom(map_->zoom+.1);});
    connect(zoomOut,&QToolButton::clicked,map_,[this]{map_->setZoom(map_->zoom-.1);});
    connect(zoomReset,&QToolButton::clicked,map_,[this]{map_->setZoom(1.0);});

    auto primer3Button=new QPushButton("Design with Primer3");
    connect(primer3Button,&QPushButton::clicked,this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"Primer3","Primer3 requires a nucleotide template.");return;
        }
        QString executable=QSettings("HelixEdit","HelixEdit").value("tools/primer3_core").toString();
        if(executable.isEmpty())executable=QStandardPaths::findExecutable("primer3_core");
        if(executable.isEmpty()){
            QMessageBox::information(this,"Primer3 not installed",
                "Install the current Primer3 command-line package (primer3_core) and add it to PATH. "
                "HelixEdit does not bundle Primer3 or substitute its thermodynamic algorithms.");
            return;
        }
        const auto &record=model->document.records[activeRow()];
        QString sequence=ungapped(record.sequence).toUpper();sequence.replace('U','T');
        if(sequence.size()<60||sequence.contains(QRegularExpression("[^ACGT]"))){
            QMessageBox::information(this,"Primer3 input","Select a DNA template of at least 60 unambiguous bases.");return;
        }
        QTemporaryDir dir;
        if(!dir.isValid()){QMessageBox::warning(this,"Primer3","Unable to create temporary directory.");return;}
        const QString inputPath=dir.filePath("primer3_input.txt");
        QFile input(inputPath);
        if(!input.open(QIODevice::WriteOnly)){QMessageBox::warning(this,"Primer3","Cannot create input file.");return;}
        const QByteArray payload=("SEQUENCE_ID=HelixEdit_template\nSEQUENCE_TEMPLATE="+sequence+
            "\nPRIMER_TASK=generic\nPRIMER_PICK_LEFT_PRIMER=1\nPRIMER_PICK_RIGHT_PRIMER=1\n"
            "PRIMER_NUM_RETURN=5\nPRIMER_PRODUCT_SIZE_RANGE=100-1000\n=\n").toUtf8();
        input.write(payload);input.close();
        QProcess process;
        process.setProgram(executable);
        process.setArguments({inputPath});
        process.setWorkingDirectory(dir.path());
        process.start();
        if(!process.waitForStarted(3000)){
            QMessageBox::warning(this,"Primer3","Could not start primer3_core.");return;
        }
        if(!process.waitForFinished(30000)){
            process.kill();process.waitForFinished();
            QMessageBox::warning(this,"Primer3","Primer3 timed out after 30 seconds.");return;
        }
        const QString result=QString::fromUtf8(process.readAllStandardOutput());
        const QString errors=QString::fromUtf8(process.readAllStandardError());
        if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0){
            QMessageBox::warning(this,"Primer3 failed",errors.left(2000));return;
        }
        showAnalysis("Primer3 design output",result+"\n"+errors+
            "\nReview primer specificity and thermodynamics independently before experimental use.");
    });
    auto primerButton=new QPushButton("Primer library");
    connect(primerButton,&QPushButton::clicked,this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"Primer library","Choose a nucleotide project first.");return;
        }
        QDialog dlg(this);dlg.setWindowTitle("Primer library · local CSV");
        dlg.resize(700,500);
        auto layout=new QVBoxLayout(&dlg);
        auto help=new QLabel("One primer per line: name,sequence,notes. Local-only library. Exact IUPAC-aware binding; no mismatch scoring.");
        help->setWordWrap(true);layout->addWidget(help);
        auto editor=new QPlainTextEdit;layout->addWidget(editor);
        const QString base=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString libraryPath=QDir(base).filePath("primers.csv");
        QFile input(libraryPath);
        if(input.open(QIODevice::ReadOnly))editor->setPlainText(QString::fromUtf8(input.readAll()));
        else editor->setPlainText("name,sequence,notes\n");
        auto actions=new QHBoxLayout;
        auto mapButton=new QPushButton("Map primers to active plasmid");
        auto saveButton=new QPushButton("Save library");
        actions->addWidget(mapButton);actions->addWidget(saveButton);layout->addLayout(actions);
        auto output=new QPlainTextEdit;output->setReadOnly(true);layout->addWidget(output);
        connect(saveButton,&QPushButton::clicked,&dlg,[=,&dlg]{
            if(!QDir().mkpath(base)){QMessageBox::warning(&dlg,"Primer library","Cannot create data folder.");return;}
            QSaveFile file(libraryPath);
            QByteArray bytes=editor->toPlainText().toUtf8();
            if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
                QMessageBox::warning(&dlg,"Primer library","Unable to save primer library.");return;
            }
            output->setPlainText("Library saved locally: "+libraryPath);
        });
        connect(mapButton,&QPushButton::clicked,&dlg,[=,this]{
            const auto primers=readPrimerCsv(editor->toPlainText());
            const auto &record=model->document.records[activeRow()];
            QString result=QString("%1 · %2 primers\n\n").arg(record.name).arg(primers.size());
            for(const auto &primer:primers){
                auto hits=primerHits(record,primer.sequence);
                result+=QString("%1  [%2 nt; GC %3%; Wallace ~%4 C]  %5 hits\n")
                    .arg(primer.name).arg(primer.sequence.size())
                    .arg(primerGc(primer.sequence),0,'f',1).arg(primerWallaceTm(primer.sequence))
                    .arg(hits.size());
                for(const auto &hit:hits)
                    result+=QString("  %1–%2  %3%4\n").arg(hit.start+1).arg(hit.end)
                        .arg(hit.reverse?"reverse":"forward")
                        .arg(hit.end>ungapped(record.sequence).size()?" (wraps origin)":"");
            }
            output->setPlainText(result);
        });
        dlg.exec();
    });
    auto mapButtons=new QHBoxLayout;
    auto restrictionButton=new QPushButton("Restriction sites");
    connect(restrictionButton,&QPushButton::clicked,this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"Restriction sites","Select a nucleotide project first.");
            return;
        }
        const auto &record=model->document.records[activeRow()];
        const auto sites=findRestrictionSites(record);
        QString report=QString("Sequence: %1\nTopology: %2\n"
            "Recognition coordinates: 1-based inclusive\n"
            "Prototype: eight exact recognition motifs; no cut offsets or digest simulation.\n\n")
            .arg(record.name,record.circular?"Circular":"Linear");
        if(sites.isEmpty())report+="No sites found for the current motif set.";
        for(const auto &site:sites)
            report+=QString("%1: %2–%3%4\n").arg(site.enzyme)
                .arg(site.start+1).arg(site.end)
                .arg(site.end>ungapped(record.sequence).size()?" (wraps origin)":"");
        showAnalysis("Restriction recognition sites",report);
    });
auto topology=new QPushButton("Toggle linear / circular");mapButtons->addWidget(topology);
    auto feat=new QPushButton("+ Feature");mapButtons->addWidget(feat);auto svg=new QPushButton("Export SVG");mapButtons->addWidget(svg);mapButtons->addStretch();ml->addLayout(mapButtons);
    tabs_->addTab(mapPage,"02  Sequence map");
    auto toolPage=new QWidget;auto tl=new QVBoxLayout(toolPage);auto th=new QLabel("<b style='font-size:19px'>Bring your own tools.</b><br><br>Native editor. Optional local packages. No sequence uploads.<br>MAFFT and MUSCLE 5 run as background jobs; output is checked before applying.");th->setWordWrap(true);tl->addWidget(th);
    for(auto name:QStringList{"mafft","muscle"}){auto b=new QPushButton("Align document with "+name.toUpper());tl->addWidget(b);connect(b,&QPushButton::clicked,this,[this,name]{align(name);});}
    auto cancel=new QPushButton("Cancel running alignment");tl->addWidget(cancel);connect(cancel,&QPushButton::clicked,this,&Editor::cancelAlignment);
    auto cfg=new QPushButton("Configure scientific tools in Settings…");tl->addWidget(cfg);connect(cfg,&QPushButton::clicked,this,&Editor::showSettings);
    log_=new QPlainTextEdit;log_->setObjectName("alignerLog");log_->setReadOnly(true);log_->setPlaceholderText("Job logs appear here. No alignment tools are bundled.");tl->addWidget(log_,1);
    tabs_->addTab(toolPage,"03  Local tools");
    // Map-first scientific navigation. Each page invokes existing, real algorithms.
    tabs_->removeTab(tabs_->indexOf(mapPage));
    tabs_->insertTab(0,mapPage,"Map");
    tabs_->setTabText(tabs_->indexOf(alignment),"Sequence");
    tabs_->setTabText(tabs_->indexOf(toolPage),"Tools");

    auto enzymePage=new QWidget;auto enzymeLayout=new QVBoxLayout(enzymePage);
    enzymeLayout->setContentsMargins(9,8,9,8);enzymeLayout->setSpacing(6);
    auto enzymeTitle=new QLabel("<b>Restriction-site explorer</b>  ·  exact recognition motifs; not a digest simulation");
    enzymeTitle->setWordWrap(true);enzymeLayout->addWidget(enzymeTitle);
    auto enzymeSearch=new QLineEdit;enzymeSearch->setObjectName("enzymeFilter");
    enzymeSearch->setPlaceholderText("Filter enzyme or coordinate…");
    enzymeLayout->addWidget(enzymeSearch);
    enzymeList_=new QListWidget;enzymeList_->setObjectName("enzymeExplorer");
    enzymeLayout->addWidget(enzymeList_,1);
    restrictionButton->setText("Full recognition-site report");
    enzymeLayout->addWidget(restrictionButton);
    connect(enzymeSearch,&QLineEdit::textChanged,this,[this](const QString &filter){
        for(int i=0;i<enzymeList_->count();++i)
            enzymeList_->item(i)->setHidden(!enzymeList_->item(i)->text().contains(filter,Qt::CaseInsensitive));
    });
    connect(enzymeList_,&QListWidget::itemActivated,this,[this,alignment](QListWidgetItem *item){
        if(!item||!item->data(Qt::UserRole).isValid())return;
        const int pos=item->data(Qt::UserRole).toInt();
        const auto &record=model->document.records[activeRow()];
        int alignedPos=-1,base=0;
        for(int col=0;col<record.sequence.size();++col) {
            if(record.sequence[col]=='-')continue;
            if(base==pos) {alignedPos=col;break;}
            ++base;
        }
        if(alignedPos<0)return;
        tabs_->setCurrentWidget(alignment);
        table->setCurrentIndex(model->index(activeRow(),alignedPos));
        table->scrollTo(table->currentIndex(),QAbstractItemView::PositionAtCenter);
    });
    tabs_->insertTab(2,enzymePage,"Enzymes");

    auto featurePage=new QWidget;auto featureLayout=new QVBoxLayout(featurePage);
    featureLayout->setContentsMargins(9,8,9,8);featureLayout->setSpacing(6);
    featureLayout->addWidget(new QLabel("<b>Annotated regions</b>  ·  1-based inclusive coordinates"));
    annotationTable_=new QTreeWidget;annotationTable_->setObjectName("annotationExplorer");
    annotationTable_->setColumnCount(5);
    annotationTable_->setHeaderLabels({"Feature","Type","Start","End","Strand"});
    annotationTable_->setRootIsDecorated(false);
    annotationTable_->setAlternatingRowColors(true);
    featureLayout->addWidget(annotationTable_,1);
    auto featureActions=new QHBoxLayout;
    auto createFeature=new QPushButton("+ Add feature");
    auto exportFeatures=new QPushButton("Export GenBank…");
    featureActions->addWidget(createFeature);featureActions->addWidget(exportFeatures);
    featureActions->addStretch();featureLayout->addLayout(featureActions);
    connect(createFeature,&QPushButton::clicked,feat,&QPushButton::click);
    connect(exportFeatures,&QPushButton::clicked,gbExportA,&QAction::trigger);
    connect(annotationTable_,&QTreeWidget::itemActivated,this,[this,alignment](QTreeWidgetItem *item,int){
        if(!item)return;
        selectFeature(item->data(0,Qt::UserRole).toInt());
        tabs_->setCurrentWidget(alignment);
    });
    tabs_->insertTab(3,featurePage,"Features");

    auto primerPage=new QWidget;auto primerLayout=new QVBoxLayout(primerPage);
    primerLayout->setContentsMargins(9,8,9,8);primerLayout->setSpacing(6);
    auto primerTitle=new QLabel("<b>Primer workspace</b>  ·  local CSV library, IUPAC-aware exact binding and Primer3");
    primerTitle->setWordWrap(true);primerLayout->addWidget(primerTitle);
    auto primerActions=new QHBoxLayout;
    primerButton->setText("Open primer library");
    auto primerRefresh=new QPushButton("Map saved primers");
    primerActions->addWidget(primerButton);
    primerActions->addWidget(primer3Button);
    primerActions->addWidget(primerRefresh);
    primerActions->addStretch();primerLayout->addLayout(primerActions);
    auto primerSummary=new QPlainTextEdit;
    primerSummary->setReadOnly(true);primerSummary->setObjectName("primerExplorer");
    primerSummary->setPlaceholderText("Choose 'Map saved primers' to report exact binding matches for the active sequence.");
    primerLayout->addWidget(primerSummary,1);
    connect(primerRefresh,&QPushButton::clicked,this,[this,primerSummary]{
        if(model->document.alphabet=="Protein"){
            primerSummary->setPlainText("Primer binding requires a nucleotide project.");return;
        }
        const QString path=QDir(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation)).filePath("primers.csv");
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)){
            primerSummary->setPlainText("No local primer library yet. Use 'Open primer library' to create one.");
            return;
        }
        runSafely([this,primerSummary,&file]{
            const auto primers=readPrimerCsv(QString::fromUtf8(file.readAll()));
            const auto &record=model->document.records[activeRow()];
            QString report=record.name+"  ·  "+QString::number(primers.size())+" primers\n\n";
            for(const auto &primer:primers) {
                const auto matches=primerHits(record,primer.sequence);
                report+=QString("%1  (%2 nt; GC %3%)  —  %4 hits\n")
                    .arg(primer.name).arg(primer.sequence.size())
                    .arg(primerGc(primer.sequence),0,'f',1).arg(matches.size());
                for(const auto &hit:matches)
                    report+=QString("   %1–%2  %3\n").arg(hit.start+1).arg(hit.end)
                        .arg(hit.reverse?"reverse":"forward");
            }
            primerSummary->setPlainText(report);
        });
    });
    tabs_->insertTab(4,primerPage,"Primers");

    auto historyPage=new QWidget;auto historyLayout=new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(9,8,9,8);historyLayout->setSpacing(6);
    historyLayout->addWidget(new QLabel("<b>Edit undo history</b>"));
    auto undoView=new QUndoView(history,historyPage);
    undoView->setObjectName("undoHistory");
    historyLayout->addWidget(undoView,1);
    historyLayout->addWidget(new QLabel("<b>Reproducible external-tool log</b>  ·  stored locally"));
    auto historyLog=new QPlainTextEdit;historyLog->setObjectName("reproducibilityLog");
    historyLog->setReadOnly(true);historyLayout->addWidget(historyLog,1);
    auto reloadHistory=new QPushButton("Refresh analysis log");
    historyLayout->addWidget(reloadHistory);
    connect(reloadHistory,&QPushButton::clicked,this,[historyLog]{
        const QString path=QDir(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation)).filePath("analysis-history.log");
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)){
            historyLog->setPlainText("No external-tool executions recorded yet.");return;
        }
        historyLog->setPlainText(QString::fromUtf8(file.readAll().right(1024*1024)));
    });
    tabs_->insertTab(5,historyPage,"History");
    connect(tabs_,&QTabWidget::currentChanged,this,[this,historyPage,reloadHistory](int){
        if(tabs_->currentWidget()==historyPage)reloadHistory->click();
    });
    if(QSettings("HelixEdit","HelixEdit").value("workspace/startTab","Map").toString()=="Sequence")
        tabs_->setCurrentWidget(alignment);
    else tabs_->setCurrentWidget(mapPage);

    auto side=new QWidget;side->setObjectName("projectSidebar");
    side->setMinimumWidth(205);side->setMaximumWidth(295);
    auto sl=new QVBoxLayout(side);sl->setContentsMargins(9,9,9,7);sl->setSpacing(7);
    auto sideTitle=new QLabel("HELIXEDIT  /  PROJECT");
    sideTitle->setObjectName("sidebarHeading");sl->addWidget(sideTitle);
    auto navFilter=new QLineEdit;navFilter->setPlaceholderText("Filter sequences…");
    navFilter->setObjectName("navigatorFilter");sl->addWidget(navFilter);
    navigator_=new QListWidget; navigator_->setObjectName("sequenceNavigator");
    navigator_->setSelectionMode(QAbstractItemView::SingleSelection);
    navigator_->setMinimumHeight(120);
    navigator_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigator_->setTextElideMode(Qt::ElideRight);
    sl->addWidget(navigator_,2);
    auto overview=new QGroupBox("DOCUMENT");
    auto ov=new QVBoxLayout(overview);stats_=new QLabel;
    stats_->setWordWrap(true);ov->addWidget(stats_);sl->addWidget(overview);
    auto sel=new QGroupBox("ACTIVE SEQUENCE");
    auto se=new QVBoxLayout(sel);selection_=new QLabel;
    selection_->setWordWrap(true);se->addWidget(selection_);sl->addWidget(sel);
    auto annotations=new QGroupBox("ANNOTATIONS");
    auto an=new QVBoxLayout(annotations);features_=new QListWidget;
    features_->setObjectName("featureNavigator");
    an->addWidget(features_);sl->addWidget(annotations,2);
    split->insertWidget(0,side);split->setStretchFactor(0,0);
    split->setStretchFactor(1,1);split->setSizes({245,960});
    connect(navFilter,&QLineEdit::textChanged,this,[this](const QString &filter){
        for(int i=0;i<navigator_->count();++i)
            navigator_->item(i)->setHidden(!navigator_->item(i)->text().contains(filter,Qt::CaseInsensitive));
    });
    connect(navigator_,&QListWidget::currentItemChanged,this,
            [this](QListWidgetItem *item,QListWidgetItem *){
        if(!item)return;
        const int row=item->data(Qt::UserRole).toInt();
        if(row<0||row>=model->rowCount())return;
        if(model->columnCount()>0) {
            const int col=qMax(0,table->currentIndex().column());
            table->setCurrentIndex(model->index(row,qMin(col,model->columnCount()-1)));
        }
        updateSelection();
    });
    connect(model,&AlignmentModel::residueEdit,this,&Editor::editResidue);
    connect(table->selectionModel(),&QItemSelectionModel::selectionChanged,this,[this]{updateSelection();});
    connect(table->selectionModel(),&QItemSelectionModel::currentChanged,this,[this]{updateSelection();});
    connect(history,&QUndoStack::cleanChanged,this,[this](bool clean){setWindowModified(!clean);});
    connect(molecule_,&QComboBox::currentTextChanged,this,[this](const QString &s){if(busy())return;runSafely([&]{auto d=model->document;d.alphabet=s;validateDocument(d);commit(d,"Change molecule type");});QSignalBlocker blocker(molecule_);molecule_->setCurrentText(model->document.alphabet);});
    connect(find,&QPushButton::clicked,this,&Editor::findNext);connect(search_,&QLineEdit::returnPressed,this,&Editor::findNext);
    connect(features_,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){selectFeature(features_->row(item));});
    connect(topology,&QPushButton::clicked,this,[this]{if(busy())return;auto d=model->document;if(d.alphabet=="Protein"){QMessageBox::information(this,"Topology","Circular maps are available for nucleotide sequences.");return;}auto &r=d.records[activeRow()];r.circular=!r.circular;commit(d,"Change topology");});
    connect(feat,&QPushButton::clicked,this,&Editor::addFeature);
    connect(svg,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getSaveFileName(this,"Export map",{},"SVG (*.svg)");if(!p.isEmpty())runSafely([&]{exportMap(p);});});
    recoveryPath_=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/recovery.helix";
    QDir().mkpath(QFileInfo(recoveryPath_).absolutePath());
    setDocument(demo());
    autosave_=new QTimer(this);
    autosave_->setInterval(qBound(30,QSettings("HelixEdit","HelixEdit")
        .value("editing/autosaveSeconds",30).toInt(),300)*1000);connect(autosave_,&QTimer::timeout,this,[this]{if(!preview_&&!history->isClean())try{
        // Save named projects directly; keep an independent recovery copy.
        // Imported FASTA/GenBank files have no project path and are never overwritten.
        if(!path_.isEmpty())writeProject(path_,model->document);
        writeProject(recoveryPath_,model->document);
        statusBar()->showMessage("Project autosaved.");
    }catch(const std::exception &e){statusBar()->showMessage("Autosave failed: "+QString::fromUtf8(e.what()));}});autosave_->start();
    if(!preview_ && QFile::exists(recoveryPath_)) {
        if(QMessageBox::question(this,"Recover previous session?","A recovery project from an interrupted session is available. Open it?")==QMessageBox::Yes)runSafely([this]{setDocument(readDocument(recoveryPath_));history->resetClean();notice_->setText("Recovered session. Save a new project to keep it.");});
        else QFile::remove(recoveryPath_);
    }
}
void Editor::runSafely(const std::function<void()> &fn){try{fn();}catch(const std::exception &e){QMessageBox::warning(this,"HelixEdit",QString::fromUtf8(e.what()));}}
bool Editor::busy() const{return process_!=nullptr;}
int Editor::activeRow() const {return table->currentIndex().isValid()?table->currentIndex().row():0;}
void Editor::applyDocument(const Document &d) {int row=activeRow(),col=qMax(0,table->currentIndex().column());model->replace(d);if(!d.records.isEmpty())table->setCurrentIndex(model->index(qMin(row,int(d.records.size()-1)),qMin(col,helix::width(d)-1)));refresh();}
void Editor::setDocument(const Document &d){validateDocument(d);path_.clear();history->clear();applyDocument(d);history->setClean();setWindowTitle("HelixEdit — Untitled[*]");}
void Editor::commit(const Document &d,const QString &label){validateDocument(d);history->push(new Change(this,model->document,d,label));}
void Editor::refresh() {
    const auto &d=model->document;
    if(auto legend=findChild<QLabel*>("residueLegend"))
        legend->setText(d.alphabet=="Protein"
            ? "  PROTEIN   ·   hydrophobic / basic / acidic / polar   ·   Double-click to edit"
            : "  DNA / RNA   ·   A adenine   ·   C cytosine   ·   G guanine   ·   T/U thymine / uracil");
    {
        QSignalBlocker block(molecule_);
        molecule_->setCurrentText(d.alphabet);
    }
    stats_->setText(QString("<b>%1</b> sequences  ·  <b>%2</b> columns<br>%3 project  ·  local")
        .arg(d.records.size()).arg(QLocale().toString(helix::width(d))).arg(d.alphabet));
    if(navigator_) {
        QSignalBlocker block(navigator_);
        const int chosen=qBound(0,activeRow(),int(d.records.size())-1);
        const auto *filter=findChild<QLineEdit*>("navigatorFilter");
        const QString filterText=filter?filter->text():QString{};
        navigator_->clear();
        for(int i=0;i<d.records.size();++i) {
            const auto &r=d.records[i];
            const int n=ungapped(r.sequence).size();
            const QString shortName=r.name.size()>30?r.name.left(29)+"…":r.name;
            auto item=new QListWidgetItem(shortName+"\n"+
                QString("%1  ·  %2 %3").arg(r.circular?"◉ circular":"— linear")
                    .arg(QLocale().toString(n)).arg(d.alphabet=="Protein"?"aa":"bp"));
            item->setSizeHint(QSize(210,42));
            item->setData(Qt::UserRole,i);
            item->setToolTip(r.name+"  ·  "+QString::number(n)+" residues");
            item->setHidden(!item->text().contains(filterText,Qt::CaseInsensitive));
            navigator_->addItem(item);
        }
        if(!d.records.isEmpty())navigator_->setCurrentRow(chosen);
    }
    notice_->setVisible(!d.warnings.isEmpty());
    const QString warningText=d.warnings.join(" ");
    notice_->setText(QFontMetrics(notice_->font()).elidedText(warningText,Qt::ElideRight,qMax(240,notice_->width()-25)));
    notice_->setToolTip(warningText);
    updateSelection();
    statusBar()->showMessage(QString("%1 sequences  ·  %2 columns  ·  DNA, protein and annotation workspaces")
        .arg(d.records.size()).arg(helix::width(d)));
}
void Editor::updateSelection() {
    const auto &doc=model->document;
    if(doc.records.isEmpty())return;
    const int row=qBound(0,activeRow(),int(doc.records.size())-1);
    const auto &r=doc.records[row];
    const auto ranges=table->selectionModel()->selection();
    const QString pos=ranges.isEmpty()?"No selected cells"
        :QString("Columns %1–%2").arg(ranges.first().left()+1).arg(ranges.first().right()+1);
    selection_->setText(QString("<b>%1</b><br>%2 residues  ·  %3<br>GC: %4")
        .arg((r.name.size()>42?r.name.left(41)+"…":r.name).toHtmlEscaped()).arg(ungapped(r.sequence).size()).arg(pos)
        .arg(doc.alphabet=="Protein"?"n/a":number(gcPercent(r.sequence))));
    if(navigator_&&navigator_->currentRow()!=row) {
        QSignalBlocker block(navigator_);
        navigator_->setCurrentRow(row);
    }
    map_->record=r;
    map_->alphabet=doc.alphabet;
    map_->update();
    mapTitle_->setTextFormat(Qt::PlainText);
    mapTitle_->setText(QString("%1   ·   %2 %3   ·   %4")
        .arg(r.name).arg(QLocale().toString(ungapped(r.sequence).size()))
        .arg(doc.alphabet=="Protein"?"aa":"bp")
        .arg(r.circular?"Circular":"Linear"));
    features_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    features_->clear();
    if(annotationTable_)annotationTable_->clear();
    for(int i=0;i<r.features.size();++i) {
        const auto &f=r.features[i];
        auto item=new QListWidgetItem(QString("%1\n%2–%3  %4")
            .arg(f.name).arg(f.start+1).arg(f.end).arg(f.reverse?"←":"→"));
        item->setForeground(featureColor(i));item->setSizeHint(QSize(190,40));features_->addItem(item);
        if(annotationTable_) {
            auto tree=new QTreeWidgetItem(annotationTable_,
                QStringList{f.name,f.type,QString::number(f.start+1),
                    QString::number(f.end),f.reverse?"Reverse":"Forward"});
            tree->setData(0,Qt::UserRole,i);
            tree->setForeground(0,featureColor(i));
        }
    }
    if(r.features.isEmpty()) {
        auto item=new QListWidgetItem("No annotations\nUse + Add feature to annotate.");
        item->setFlags(Qt::NoItemFlags);features_->addItem(item);
    }
    if(enzymeList_) {
        enzymeList_->clear();
        if(doc.alphabet=="Protein") {
            auto item=new QListWidgetItem("Restriction enzymes require nucleotide sequences");
            item->setFlags(Qt::NoItemFlags);enzymeList_->addItem(item);
        } else {
            const auto sites=findRestrictionSites(r);
            if(sites.isEmpty()) {
                auto item=new QListWidgetItem("No exact recognition sites in the built-in eight-enzyme set");
                item->setFlags(Qt::NoItemFlags);enzymeList_->addItem(item);
            }
            for(const auto &hit:sites) {
                auto item=new QListWidgetItem(QString("%1   ·   position %2")
                    .arg(hit.enzyme).arg(hit.start+1));
                item->setData(Qt::UserRole,hit.start);
                item->setToolTip("Double-click to select the recognition-site position in Sequence");
                enzymeList_->addItem(item);
            }
        }
        if(const auto *filter=findChild<QLineEdit*>("enzymeFilter")) {
            for(int i=0;i<enzymeList_->count();++i)
                enzymeList_->item(i)->setHidden(!enzymeList_->item(i)->text()
                    .contains(filter->text(),Qt::CaseInsensitive));
        }
    }
}
void Editor::editResidue(int row,int col,QChar residue){if(busy())return;auto d=model->document;if(row<0||row>=d.records.size()||col<0||col>=d.records[row].sequence.size())return;if(d.records[row].sequence[col]==residue)return;
    if(d.alphabet!="Protein"&&!QString("ACGTURYSWKMBDHVN?-").contains(residue))return;
    if((residue=='-')!=(d.records[row].sequence[col]=='-'))d.records[row].features.clear();
    d.records[row].sequence[col]=residue;commit(d,"Replace residue");table->setCurrentIndex(model->index(row,col));}
void Editor::gapColumns(bool insert){if(busy())return;auto ranges=table->selectionModel()->selection();if(ranges.size()!=1){QMessageBox::information(this,"Selection","Select one contiguous rectangle first.");return;}auto range=ranges.first();
    if(!insert && !preview_ && QMessageBox::question(this,"Delete columns","Delete the selected alignment columns from the selected rows? Features on changed rows will be cleared. Undo is available.")!=QMessageBox::Yes)return;
    runSafely([&]{auto d=model->document;for(int row=range.top();row<=range.bottom();++row){auto &r=d.records[row];if(insert){if(range.left()>r.sequence.size())throw std::runtime_error("Cannot insert beyond a sequence's end.");r.sequence.insert(range.left(),'-');}else{auto old=ungapped(r.sequence);r.sequence.remove(range.left(),range.width());if(ungapped(r.sequence)!=old)r.features.clear();if(r.sequence.isEmpty())throw std::runtime_error("Cannot delete an entire sequence.");}}commit(d,insert?"Insert gap":"Delete columns");});}
void Editor::reverseSelected(){if(busy())return;if(model->document.alphabet=="Protein"){QMessageBox::information(this,"Reverse complement","Select a DNA or RNA document.");return;}QSet<int> rows;for(const auto &range:table->selectionModel()->selection())for(int r=range.top();r<=range.bottom();++r)rows.insert(r);if(rows.isEmpty())rows.insert(activeRow());
    runSafely([&]{auto d=model->document;for(int row:rows){auto &r=d.records[row];r.sequence=reverseComplement(r.sequence,d.alphabet=="RNA");int len=ungapped(r.sequence).size();for(auto &f:r.features){int start=f.start;f.start=len-f.end;f.end=len-start;f.reverse=!f.reverse;}}commit(d,"Reverse complement");});}
bool Editor::canReplace(){if(busy()){QMessageBox::information(this,"Alignment running","Wait for the local alignment job to finish before opening another document.");return false;}if(history->isClean())return true;auto answer=QMessageBox::warning(this,"Unsaved edits","Save this project before continuing?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(answer==QMessageBox::Cancel)return false;if(answer==QMessageBox::Save)return save();return true;}
void Editor::loadPath(const QString &p){auto d=readDocument(p);setDocument(d);if(p.endsWith(".helix",Qt::CaseInsensitive))path_=p;setWindowTitle("HelixEdit — "+QFileInfo(p).fileName()+"[*]");}
void Editor::savePath(const QString &p){writeProject(p,model->document);path_=p;history->setClean();QFile::remove(recoveryPath_);setWindowTitle("HelixEdit — "+QFileInfo(p).fileName()+"[*]");statusBar()->showMessage("Project saved: "+p);}
void Editor::open(){if(!canReplace())return;auto p=QFileDialog::getOpenFileName(this,"Open sequences",{},"Sequences (*.helix *.fa *.fasta *.fna *.faa *.aln *.gb *.gbk *.genbank);;All files (*)");if(!p.isEmpty())runSafely([&]{loadPath(p);});}
bool Editor::save(bool as){QString p=path_;if(as||p.isEmpty())p=QFileDialog::getSaveFileName(this,"Save HelixEdit project",p,"HelixEdit project (*.helix)");if(p.isEmpty())return false;if(!p.endsWith(".helix",Qt::CaseInsensitive))p+=".helix";try{savePath(p);return true;}catch(const std::exception &e){QMessageBox::warning(this,"Save failed",e.what());return false;}}
void Editor::exportFile(){auto p=QFileDialog::getSaveFileName(this,"Export sequences (features and topology omitted)",{},"FASTA (*.fasta);;CLUSTAL (*.aln)");if(p.isEmpty())return;runSafely([&]{QString data=p.endsWith(".aln",Qt::CaseInsensitive)?clustal(model->document):fasta(model->document);QSaveFile f(p);if(!f.open(QIODevice::WriteOnly)||f.write(data.toUtf8())!=data.toUtf8().size()||!f.commit())throw std::runtime_error(f.errorString().toStdString());statusBar()->showMessage("Sequence-only export saved; project metadata is retained in .helix only.");});}
void Editor::addSequence(){if(busy())return;QDialog dlg(this);dlg.setWindowTitle("Add a sequence");auto l=new QVBoxLayout(&dlg);auto name=new QLineEdit;name->setPlaceholderText("Sequence name");l->addWidget(name);auto seq=new QPlainTextEdit;seq->setPlaceholderText("Paste residues or a FASTA record");l->addWidget(seq);auto buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);dlg.resize(480,310);if(dlg.exec()!=QDialog::Accepted)return;runSafely([&]{auto d=model->document;auto text=seq->toPlainText().trimmed();if(text.startsWith('>')){auto incoming=parseFasta(text);for(const auto &r:incoming.records)d.records.append(r);}else d.records.append(Record{name->text().trimmed(),validateSequence(text),{},false});commit(d,"Add sequence");});}
void Editor::addFeature(){if(busy())return;int row=activeRow(),len=ungapped(model->document.records[row].sequence).size();QDialog dlg(this);dlg.setWindowTitle("Add feature · ungapped coordinates");auto l=new QFormLayout(&dlg);auto name=new QLineEdit;auto start=new QSpinBox;start->setRange(1,len);auto end=new QSpinBox;end->setRange(1,len);end->setValue(qMin(30,len));auto rev=new QCheckBox("Reverse strand");l->addRow("Label",name);l->addRow("Start (1-based, inclusive)",start);l->addRow("End (inclusive)",end);l->addRow(rev);auto b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addRow(b);connect(b,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);if(dlg.exec()!=QDialog::Accepted)return;runSafely([&]{if(name->text().trimmed().isEmpty()||end->value()<start->value())throw std::runtime_error("Enter a label and an end at or after the start.");auto d=model->document;d.records[row].features.append(Feature{name->text().trimmed(),"misc_feature",start->value()-1,end->value(),rev->isChecked()});commit(d,"Add feature");});}
void Editor::selectFeature(int idx){const auto &r=model->document.records[activeRow()];if(idx<0||idx>=r.features.size())return;auto f=r.features[idx];int ungap=0,left=-1,right=-1;for(int i=0;i<r.sequence.size();++i)if(r.sequence[i]!='-'){if(ungap==f.start)left=i;if(ungap==f.end-1)right=i;++ungap;}if(left<0||right<0)return;int row=activeRow();QItemSelection sel(model->index(row,left),model->index(row,right));table->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);table->scrollTo(model->index(row,left));tabs_->setCurrentIndex(1);}
void Editor::showAnalysis(const QString &title,const QString &text){QDialog d(this);d.setWindowTitle(title);auto l=new QVBoxLayout(&d);auto view=new QPlainTextEdit;view->setReadOnly(true);view->setPlainText(text);l->addWidget(view);auto b=new QDialogButtonBox(QDialogButtonBox::Close);l->addWidget(b);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);d.resize(650,400);d.exec();}
void Editor::translateSelected(){if(model->document.alphabet=="Protein"){QMessageBox::information(this,"Translate","Select DNA or RNA first.");return;}bool ok=false;int frame=QInputDialog::getInt(this,"Translation","Frame (standard genetic code, complete codons only)",1,1,3,1,&ok);if(!ok)return;runSafely([&]{const auto &r=model->document.records[activeRow()];showAnalysis("Translation",">"+r.name+" frame "+QString::number(frame)+"\n"+translate(r.sequence,frame-1)+"\n\nStandard code. Ambiguous codons → X. Trailing partial codons omitted. Stops → *. Full active row, not selected columns.");});}
void Editor::findNext(){QString motif=search_->text().trimmed().toUpper();if(motif.isEmpty())return;if(motif.contains(QRegularExpression("[^A-Z*?-]"))){statusBar()->showMessage("Search uses exact residue symbols; no regular expressions.");return;}int row=activeRow();const auto &s=model->document.records[row].sequence;int from=table->currentIndex().isValid()?table->currentIndex().column()+1:0;int at=s.indexOf(motif,from);if(at<0)at=s.indexOf(motif);if(at<0){statusBar()->showMessage("No exact match in active row. Search does not cross a circular origin.");return;}table->setCurrentIndex(model->index(row,at));QItemSelection sel(model->index(row,at),model->index(row,at+motif.size()-1));table->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);table->scrollTo(model->index(row,at));tabs_->setCurrentIndex(0);statusBar()->showMessage("Exact match at alignment column "+QString::number(at+1));}
void Editor::showSettings() {
    QSettings stored("HelixEdit","HelixEdit");
    QDialog dialog(this);
    dialog.setObjectName("helixSettingsDialog");
    dialog.setWindowTitle("HelixEdit · Settings");
    dialog.resize(690,500);
    auto main=new QVBoxLayout(&dialog);
    main->setContentsMargins(14,14,14,12);
    main->setSpacing(9);
    auto header=new QLabel("<b style='font-size:17px'>Settings & preferences</b>"
        "<br><span style='color:#8291a5'>Personalize the workspace, local tools and update source.</span>");
    header->setWordWrap(true);
    main->addWidget(header);
    auto pages=new QTabWidget(&dialog);
    pages->setObjectName("preferencesTabs");
    main->addWidget(pages,1);

    // Appearance: theme and initial working view.
    auto appearancePage=new QWidget(pages);
    auto appearanceLayout=new QVBoxLayout(appearancePage);
    appearanceLayout->setContentsMargins(14,15,14,12);
    auto appearanceForm=new QFormLayout;
    appearanceForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    auto theme=new QComboBox;
    theme->setObjectName("themePreference");
    theme->addItems({"Light","Dark","Dracula"});
    theme->setCurrentText(savedTheme());
    appearanceForm->addRow("Color theme",theme);
    auto startPage=new QComboBox;
    startPage->setObjectName("startupTabPreference");
    startPage->addItems({"Map","Sequence"});
    startPage->setCurrentText(stored.value("workspace/startTab","Map").toString());
    appearanceForm->addRow("Default document view",startPage);
    appearanceLayout->addLayout(appearanceForm);
    auto themeNote=new QLabel("Themes apply to menus, editor panels, dialogs and the plasmid map. "
        "Choose <b>Save settings</b> to apply your changes. "
        "The startup view is used for newly opened sessions.");
    themeNote->setWordWrap(true);
    appearanceLayout->addWidget(themeNote);
    appearanceLayout->addStretch();
    pages->addTab(appearancePage,"Appearance");

    // Map display and local autosave interval.
    auto mapPage=new QWidget(pages);
    auto mapLayout=new QVBoxLayout(mapPage);
    mapLayout->setContentsMargins(14,15,14,12);
    auto mapEnzymes=new QCheckBox("Show recognition-site labels on circular maps");
    mapEnzymes->setObjectName("enzymeLabelsPreference");
    mapEnzymes->setChecked(stored.value("map/showEnzymes",true).toBool());
    auto mapFeatures=new QCheckBox("Display colored annotation arrows and labels");
    mapFeatures->setObjectName("featureLabelsPreference");
    mapFeatures->setChecked(stored.value("map/showFeatures",true).toBool());
    mapLayout->addWidget(mapEnzymes);
    mapLayout->addWidget(mapFeatures);
    auto zoomForm=new QFormLayout;
    auto zoomRow=new QWidget;
    auto zoomLayout=new QHBoxLayout(zoomRow);
    zoomLayout->setContentsMargins(0,0,0,0);
    auto zoom=new QSlider(Qt::Horizontal);
    zoom->setObjectName("defaultMapZoom");
    zoom->setRange(65,135);
    zoom->setSingleStep(5);
    zoom->setPageStep(10);
    zoom->setValue(stored.value("map/defaultZoom",100).toInt());
    auto zoomValue=new QLabel(QString::number(zoom->value())+"%");
    zoomValue->setMinimumWidth(44);
    zoomLayout->addWidget(zoom,1);
    zoomLayout->addWidget(zoomValue);
    connect(zoom,&QSlider::valueChanged,&dialog,[zoomValue](int value){
        zoomValue->setText(QString::number(value)+"%");
    });
    zoomForm->addRow("Map zoom",zoomRow);
    auto autosave=new QComboBox;
    autosave->setObjectName("autosavePreference");
    for(const int seconds:{30,60,120,300})
        autosave->addItem(QString::number(seconds)+" seconds",seconds);
    const int chosenInterval=stored.value("editing/autosaveSeconds",30).toInt();
    const int initialInterval=autosave->findData(chosenInterval);
    autosave->setCurrentIndex(initialInterval>=0?initialInterval:0);
    zoomForm->addRow("Automatic local save",autosave);
    mapLayout->addLayout(zoomForm);
    auto mapNote=new QLabel("Changes to a named .helix project are periodically saved. "
        "An independent local recovery copy is retained. FASTA/GenBank originals are not overwritten. "
        "Map visibility controls remain available for quick adjustments in the Map tab.");
    mapNote->setWordWrap(true);
    mapLayout->addWidget(mapNote);
    mapLayout->addStretch();
    pages->addTab(mapPage,"Map & editing");

    // Executable paths are optional: empty means find each tool on PATH.
    auto toolsPage=new QWidget(pages);
    auto toolsLayout=new QVBoxLayout(toolsPage);
    toolsLayout->setContentsMargins(14,15,14,12);
    auto executables=new QFormLayout;
    executables->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QMap<QString,QLineEdit*> toolFields;
    for(const auto &key:QStringList{"mafft","muscle","blastn","blastp","primer3_core"}) {
        auto row=new QWidget(toolsPage);
        auto horizontal=new QHBoxLayout(row);
        horizontal->setContentsMargins(0,0,0,0);
        horizontal->setSpacing(5);
        auto input=new QLineEdit(row);
        input->setObjectName("executable_"+key);
        input->setText(stored.value("tools/"+key).toString());
        const QString detected=QStandardPaths::findExecutable(key);
        input->setPlaceholderText(detected.isEmpty()?"Search system PATH":detected);
        auto browse=new QPushButton("Browse…",row);
        horizontal->addWidget(input,1);
        horizontal->addWidget(browse);
        executables->addRow(key,row);
        toolFields.insert(key,input);
        connect(browse,&QPushButton::clicked,&dialog,[&dialog,input,key]{
            const QString chosen=QFileDialog::getOpenFileName(&dialog,
                "Choose "+key+" executable",input->text());
            if(!chosen.isEmpty())input->setText(chosen);
        });
    }
    toolsLayout->addLayout(executables);
    auto toolsNote=new QLabel("Leave a field blank to use an installed executable on PATH. "
        "Specify an absolute executable path without command-line arguments. "
        "MAFFT, MUSCLE, BLAST+ and Primer3 are optional scientific packages; "
        "HelixEdit does not install or bundle them.");
    toolsNote->setWordWrap(true);
    toolsLayout->addWidget(toolsNote);
    toolsLayout->addStretch();
    pages->addTab(toolsPage,"Scientific tools");

    // Source-based upgrades are initiated by the user, never silently scheduled.
    auto updatesPage=new QWidget(pages);
    auto updatesLayout=new QVBoxLayout(updatesPage);
    updatesLayout->setContentsMargins(14,15,14,12);
    updatesLayout->addWidget(new QLabel("<b>GitHub source update</b>"));
    auto sourceRow=new QHBoxLayout;
    auto source=new QLineEdit;
    source->setObjectName("updateSourcePreference");
    source->setPlaceholderText("Existing cloned HelixEdit source folder");
    source->setText(stored.value("updater/source").toString());
    auto browseSource=new QPushButton("Browse…");
    sourceRow->addWidget(source,1);
    sourceRow->addWidget(browseSource);
    updatesLayout->addLayout(sourceRow);
    connect(browseSource,&QPushButton::clicked,&dialog,[&dialog,source]{
        const QString folder=QFileDialog::getExistingDirectory(&dialog,
            "Select existing HelixEdit Git repository",source->text());
        if(!folder.isEmpty())source->setText(folder);
    });
    auto updateNote=new QLabel("Updates use the official "
        "<a href='https://github.com/mac860/HelixEdit'>mac860/HelixEdit</a> "
        "<b>main</b> branch. A clean existing Git checkout is required. "
        "Source updates, builds, tests and package installation run only when requested. "
        "No automatic background update checks or scheduled downloads are enabled.");
    updateNote->setOpenExternalLinks(true);
    updateNote->setWordWrap(true);
    updatesLayout->addWidget(updateNote);
    auto launchUpdater=new QPushButton("Open update assistant…");
    updatesLayout->addWidget(launchUpdater,0,Qt::AlignLeft);
    connect(launchUpdater,&QPushButton::clicked,&dialog,[this,&dialog]{
        dialog.accept();
        openSourceUpdater(this);
    });
    updatesLayout->addStretch();
    pages->addTab(updatesPage,"Updates");

    auto bottom=new QHBoxLayout;
    auto reset=new QPushButton("Restore defaults");
    reset->setObjectName("restoreSettingsDefaults");
    bottom->addWidget(reset);
    bottom->addStretch();
    auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText("Save settings");
    bottom->addWidget(buttons);
    main->addLayout(bottom);
    connect(reset,&QPushButton::clicked,&dialog,[=]{
        theme->setCurrentText("Dark");
        startPage->setCurrentText("Map");
        mapEnzymes->setChecked(true);
        mapFeatures->setChecked(true);
        zoom->setValue(100);
        autosave->setCurrentIndex(0);
        for(auto *field:toolFields)field->clear();
        source->clear();
    });
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        for(auto it=toolFields.cbegin();it!=toolFields.cend();++it) {
            const QString value=it.value()->text().trimmed();
            if(!value.isEmpty() && (!QFileInfo(value).isAbsolute() ||
                                     !QFileInfo(value).isExecutable())) {
                QMessageBox::warning(&dialog,"Invalid executable path",
                    "The path for "+it.key()+" must be a valid executable file. "
                    "Clear the field to search your system PATH automatically.");
                pages->setCurrentWidget(toolsPage);
                it.value()->setFocus();
                return;
            }
        }
        const QString sourceFolder=source->text().trimmed();
        if(!sourceFolder.isEmpty() &&
           (!QFileInfo::exists(QDir(sourceFolder).filePath(".git")) ||
            !QFileInfo::exists(QDir(sourceFolder).filePath("CMakeLists.txt")))) {
            QMessageBox::warning(&dialog,"Invalid source folder",
                "Select a HelixEdit Git checkout containing .git and CMakeLists.txt, "
                "or clear the path to configure it later.");
            pages->setCurrentWidget(updatesPage);
            source->setFocus();
            return;
        }
        QSettings settings("HelixEdit","HelixEdit");
        settings.setValue("workspace/startTab",startPage->currentText());
        settings.setValue("map/showEnzymes",mapEnzymes->isChecked());
        settings.setValue("map/showFeatures",mapFeatures->isChecked());
        settings.setValue("map/defaultZoom",zoom->value());
        settings.setValue("editing/autosaveSeconds",autosave->currentData().toInt());
        settings.setValue("updater/source",sourceFolder);
        for(auto it=toolFields.cbegin();it!=toolFields.cend();++it)
            settings.setValue("tools/"+it.key(),it.value()->text().trimmed());
        applyTheme(theme->currentText());
        if(mapEnzymeToggle_)mapEnzymeToggle_->setChecked(mapEnzymes->isChecked());
        if(mapFeatureToggle_)mapFeatureToggle_->setChecked(mapFeatures->isChecked());
        if(map_) {map_->setZoom(zoom->value()/100.0);map_->update();}
        if(autosave_)autosave_->setInterval(autosave->currentData().toInt()*1000);
        dialog.accept();
        statusBar()->showMessage("Settings saved locally.");
    });
    dialog.exec();
}
void Editor::align(const QString &tool){
    if(busy()){QMessageBox::information(this,"Alignment running","One job at a time. The current document remains locked until it finishes.");return;}
    if(model->document.records.size()<2){QMessageBox::information(this,"Alignment","Add at least two sequences.");return;}
    QSettings settings;QString exe=settings.value("tools/"+tool).toString();if(exe.isEmpty())exe=QStandardPaths::findExecutable(tool);
#ifdef Q_OS_MACOS
    if(exe.isEmpty()&&QFileInfo::exists("/opt/homebrew/bin/"+tool))exe="/opt/homebrew/bin/"+tool;
#endif
    if(exe.isEmpty()||!QFileInfo(exe).isExecutable()){QMessageBox::information(this,"Package not found","Install "+tool+" for your architecture, then configure its executable path under Settings → Preferences.");return;}
    auto original=model->document;Document input=original;
    for(int i=0;i<input.records.size();++i){input.records[i].name="hx"+QString::number(i);input.records[i].sequence=ungapped(input.records[i].sequence);if(input.records[i].sequence.isEmpty()){QMessageBox::information(this,"Alignment","An all-gap row cannot be aligned.");return;}}
    temp_=std::make_unique<QTemporaryDir>();if(!temp_->isValid()){QMessageBox::warning(this,"Alignment","Cannot create job directory.");return;}
    QString in=temp_->filePath("input.fasta"),out=temp_->filePath("output.fasta");QFile f(in);if(!f.open(QIODevice::WriteOnly)){QMessageBox::warning(this,"Alignment",f.errorString());return;}f.write(fasta(input).toUtf8());f.close();
    QStringList args=tool=="mafft"?QStringList{"--auto",original.alphabet=="Protein"?"--amino":"--nuc",in}:QStringList{"-align",in,"-output",out,original.alphabet=="Protein"?"-amino":"-nt"};
    process_=new QProcess(this);auto job=process_;
#ifdef Q_OS_UNIX
    job->setChildProcessModifier([]{ ::setpgid(0,0); });
#endif
    job->setWorkingDirectory(temp_->path());if(tool=="mafft")job->setStandardOutputFile(out);
    table->setEnabled(false);molecule_->setEnabled(false);history->setActive(false); // menu undo disabled explicitly below
    // QUndoStack::setActive(false) disables undo/redo without disabling the File menu.
    log_->clear();
    const QByteArray sourceBytes=fasta(input).toUtf8();
    const QString digest=QString::fromLatin1(QCryptographicHash::hash(
        sourceBytes,QCryptographicHash::Sha256).toHex());
    log_->appendPlainText(QString("UTC: %1\nTool: %2\nExecutable: %3\n"
        "Arguments: %4\nInput SHA256: %5\n")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),tool,exe,args.join(" "),digest));
    tabs_->setCurrentIndex(2);
    if(tool=="muscle")connect(job,&QProcess::readyReadStandardOutput,this,[this,job]{auto text=QString::fromUtf8(job->readAllStandardOutput());if(log_->document()->blockCount()>2000)log_->clear();log_->appendPlainText(text.right(16000));});
    connect(job,&QProcess::readyReadStandardError,this,[this,job]{auto text=QString::fromUtf8(job->readAllStandardError());if(log_->document()->blockCount()>2000)log_->clear();log_->appendPlainText(text.right(16000));});
    auto finish=[this,job,original,out,tool](int code,QProcess::ExitStatus status){
        if(process_!=job)return;process_=nullptr;table->setEnabled(true);molecule_->setEnabled(true);history->setActive(true);
        if(code==0&&status==QProcess::NormalExit&&!job->property("cancelled").toBool()){try{auto aligned=readDocument(out);auto d=acceptAlignment(original,aligned);commit(d,"Apply local alignment");log_->appendPlainText("\nValidated and applied. Original names, features and row order preserved. Undo available.");}catch(const std::exception &e){log_->appendPlainText("Rejected output: "+QString::fromUtf8(e.what()));}}
        else {
        if(tool=="muscle" && log_->toPlainText().contains("sizeof(void *)"))log_->appendPlainText("This MUSCLE build has a known ARM64 pointer-width detection issue. Prefer MAFFT, or build MUSCLE with scripts/build_muscle_arm64.sh and configure its executable path.");
        log_->appendPlainText(job->property("cancelled").toBool()?"Job cancelled. Document unchanged.":"Job failed. Document unchanged. Exit code: "+QString::number(code));
        }
        const QString folder=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if(QDir().mkpath(folder)){
            QFile audit(QDir(folder).filePath("analysis-history.log"));
            if(audit.open(QIODevice::WriteOnly|QIODevice::Append)){
                audit.write(("\n===== ALIGNMENT RUN =====\n"+
                    log_->toPlainText()+"\n").toUtf8());
                audit.close();
            }
        }
        job->deleteLater();temp_.reset();statusBar()->showMessage("Alignment job finished; see Local tools log.");
    };
    connect(job,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,finish);
    connect(job,&QProcess::errorOccurred,this,[this,finish](QProcess::ProcessError error){if(error==QProcess::FailedToStart){log_->appendPlainText("Executable could not start (check architecture and permissions).");finish(-1,QProcess::CrashExit);}});
    QTimer::singleShot(600000,job,[this,job]{if(process_==job){log_->appendPlainText("Job reached the 10-minute preview limit.");cancelAlignment();}});
    job->start(exe,args);
}
void Editor::cancelAlignment(){
    if(!process_)return;auto job=process_;job->setProperty("cancelled",true);
#ifdef Q_OS_UNIX
    qint64 pid=job->processId();if(pid>0)::kill(-pid,SIGTERM);
#else
    job->terminate();
#endif
    QTimer::singleShot(1500,job,[this,job]{if(process_!=job)return;
#ifdef Q_OS_UNIX
        qint64 pid=job->processId();if(pid>0)::kill(-pid,SIGKILL);
#else
        job->kill();
#endif
    });
}
void Editor::exportMap(const QString &p){QSvgGenerator svg;svg.setFileName(p);int height=qMax(720,680+int((map_->record.features.size()+2)/3)*20);svg.setSize(QSize(1000,height));svg.setViewBox(QRect(0,0,1000,height));svg.setTitle(map_->record.name);QPainter painter;if(!painter.begin(&svg))throw std::runtime_error("Cannot write SVG file.");map_->draw(painter,QRectF(0,40,1000,600));painter.setPen(QColor("#29283c"));painter.drawText(QRect(30,10,940,30),Qt::AlignCenter,map_->record.name);int y=650;for(int i=0;i<map_->record.features.size();++i){const auto &f=map_->record.features[i];painter.setPen(featureColor(i));painter.drawText(30+(i%3)*320,y+(i/3)*20,QString("%1  [%2–%3] %4").arg(f.name).arg(f.start+1).arg(f.end).arg(f.reverse?"←":"→"));}painter.end();}
void Editor::closeEvent(QCloseEvent *e){if(busy()){QMessageBox::information(this,"Alignment running","Wait for the alignment to finish before closing HelixEdit.");e->ignore();return;}if(canReplace()){if(!preview_)QFile::remove(recoveryPath_);e->accept();}else e->ignore();}
}
