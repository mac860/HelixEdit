#include "editor.h"
#include "themes.h"
#include "orf.h"
#include "restriction.h"
#include "primer.h"
#include <QtSvg/QSvgGenerator>
#include <QDesktopServices>
#include <cmath>
#include <stdexcept>
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
    p.save(); p.setRenderHint(QPainter::Antialiasing);p.fillRect(bounds,QColor("#ffffff"));
    QString seq=ungapped(record.sequence); const int n=seq.size();
    if(!n) {p.drawText(bounds,Qt::AlignCenter,"Open a sequence to view its map");p.restore();return;}
    p.setPen(QColor("#29283c"));
    if(!record.circular || alphabet=="Protein") {
        const qreal x=bounds.left()+45,y=bounds.center().y()-25,w=bounds.width()-90;
        p.setPen(QPen(QColor("#d7d3e6"),6));p.drawLine(QPointF(x,y),QPointF(x+w,y));
        p.setPen(QColor("#6d6682"));p.drawText(QRectF(x,y-50,w,28),Qt::AlignCenter,"LINEAR  /  "+QString::number(n)+(alphabet=="Protein"?" aa":" bp"));
        for(int j=0;j<=4;++j) {qreal xx=x+w*j/4.;p.drawLine(QPointF(xx,y-6),QPointF(xx,y+6));p.drawText(QRectF(xx-30,y+12,60,20),Qt::AlignCenter,QString::number(j?int(n*j/4.):1));}
        for(int i=0;i<record.features.size();++i) {
            const auto &f=record.features[i];qreal left=x+w*f.start/n,right=x+w*f.end/n, yy=y+55+(i%3)*29;
            p.setPen(Qt::NoPen);p.setBrush(featureColor(i));
            QPolygonF arrow; qreal head=qMin(8.,(right-left)/2);
            if(f.reverse) arrow<<QPointF(right,yy-7)<<QPointF(left+head,yy-7)<<QPointF(left,yy)<<QPointF(left+head,yy+7)<<QPointF(right,yy+7);
            else arrow<<QPointF(left,yy-7)<<QPointF(right-head,yy-7)<<QPointF(right,yy)<<QPointF(right-head,yy+7)<<QPointF(left,yy+7);
            p.drawPolygon(arrow);
        }
    } else {
        QPointF center(bounds.center().x(),bounds.center().y()-4);qreal radius=qMin(bounds.width(),bounds.height())*.30;
        QRectF ring(center.x()-radius,center.y()-radius,radius*2,radius*2);
        p.setPen(QPen(QColor("#edeaf6"),16));p.drawEllipse(ring);
        for(int j=0;j<12;++j) {
            double angle=j*2*M_PI/12-M_PI/2;QPointF unit(std::cos(angle),std::sin(angle));
            p.setPen(QPen(QColor("#bcb5d2"),1));p.drawLine(center+unit*(radius+16),center+unit*(radius+23));
            p.setPen(QColor("#77708b"));QPointF pos=center+unit*(radius+39);p.drawText(QRectF(pos.x()-28,pos.y()-10,56,20),Qt::AlignCenter,QString::number(int(n*j/12.)+1));
        }
        for(int i=0;i<record.features.size();++i) {
            const auto &f=record.features[i];auto color=featureColor(i);
            double a=90.-360.*f.start/n,span=-360.*(f.end-f.start)/n;
            p.setPen(QPen(color,15,Qt::SolidLine,Qt::FlatCap));p.drawArc(ring,int(a*16),int(span*16));
            double tip=(f.reverse?f.start:f.end)*2*M_PI/n-M_PI/2;
            QPointF unit(std::cos(tip),std::sin(tip)), tangent(-std::sin(tip),std::cos(tip));if(f.reverse)tangent=-tangent;
            QPointF pt=center+unit*radius;p.setPen(Qt::NoPen);p.setBrush(color);
            p.drawPolygon(QPolygonF{pt+tangent*7,pt-tangent*5+unit*10,pt-tangent*5-unit*10});
        }
        QFont font=p.font();font.setPointSize(22);font.setBold(true);p.setFont(font);p.setPen(QColor("#332958"));
        p.drawText(QRectF(center.x()-90,center.y()-35,180,40),Qt::AlignCenter,QLocale().toString(n));
        font.setPointSize(10);font.setBold(false);p.setFont(font);p.setPen(QColor("#77708b"));
        p.drawText(QRectF(center.x()-90,center.y()+4,180,40),Qt::AlignCenter,"bp  /  CIRCULAR");
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
    setWindowTitle("HelixEdit");resize(1180,760);setMinimumSize(860,580);
    model=new AlignmentModel(this);history=new QUndoStack(this);history->setUndoLimit(30);
    applyTheme(savedTheme());
    auto appearance=menuBar()->addMenu("&Appearance");
    auto themeGroup=new QActionGroup(this);
    themeGroup->setExclusive(true);
    for(const QString &theme : {QString("Light"),QString("Dark"),QString("Dracula")}) {
        auto action=appearance->addAction(theme);
        action->setCheckable(true);
        action->setChecked(theme==savedTheme());
        themeGroup->addAction(action);
        connect(action,&QAction::triggered,this,[theme]{applyTheme(theme);});
    }
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
    auto addgeneMenu=fileMenu->addMenu("Addgene plasmids");
    addgeneMenu->addAction("Find plasmid by Addgene ID…",this,[this]{
        bool ok=false;
        const int id=QInputDialog::getInt(this,"Addgene plasmid","Addgene plasmid ID",10878,1,999999999,1,&ok);
        if(!ok)return;
        const QUrl url(QString("https://www.addgene.org/%1/").arg(id));
        QDesktopServices::openUrl(url);
        QMessageBox::information(this,"Import from Addgene",
            "Sign in to your Addgene account in the browser, then download the GenBank (.gb/.gbk) sequence from the plasmid page.\\n\\n"
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
    fileMenu->addAction("Export map as SVG…",this,[this]{auto p=QFileDialog::getSaveFileName(this,"Export vector map",{},"SVG (*.svg)");if(!p.isEmpty())runSafely([&]{exportMap(p);});});
    fileMenu->addAction("Load teaching demo",this,[this]{if(canReplace())setDocument(demo());});
    fileMenu->addSeparator();fileMenu->addAction("Quit",QKeySequence::Quit,this,&QWidget::close);
    auto edit=menuBar()->addMenu("&Edit");
    auto undo=history->createUndoAction(this,"Undo");undo->setShortcut(QKeySequence::Undo);edit->addAction(undo);
    auto redo=history->createRedoAction(this,"Redo");redo->setShortcut(QKeySequence::Redo);edit->addAction(redo);
    edit->addAction("Add sequence…",this,&Editor::addSequence);
    edit->addAction("Copy selection as FASTA",QKeySequence::Copy,this,[this]{
        auto ranges=table->selectionModel()->selection();if(ranges.size()!=1){statusBar()->showMessage("Select one rectangular block to copy as FASTA.");return;}
        const auto range=ranges.first();Document block;block.alphabet=model->document.alphabet;
        for(int row=range.top();row<=range.bottom();++row){auto r=model->document.records[row];r.sequence=r.sequence.mid(range.left(),range.width());r.features.clear();r.circular=false;if(!r.sequence.isEmpty())block.records.append(r);}
        QGuiApplication::clipboard()->setText(fasta(block));statusBar()->showMessage("Selected block copied as sequence-only FASTA.");
    });
    edit->addAction("Paste new FASTA records",QKeySequence::Paste,this,[this]{if(busy())return;runSafely([&]{auto incoming=parseFasta(QGuiApplication::clipboard()->text());auto d=model->document;d.records+=incoming.records;commit(d,"Paste sequences");});});
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
        const QString message=QString("Remove %1 selected sequence(s)?\\n\\nThis can be undone.")
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
            QString report="Sequence: "+record.name+"\\nCoordinates: 1-based inclusive, forward reference\\nComplete ATG-start ORFs only\\n\\n";
            if(orfs.isEmpty())report+="No matching ORFs found.";
            for(const auto &orf:orfs) {
                report+=QString("Frame %1  |  %2–%3  |  %4 aa\\n%5\\n\\n")
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
    auto tools=menuBar()->addMenu("&Tools");
    tools->addAction("Align with MAFFT",this,[this]{align("mafft");});
    tools->addAction("Align with MUSCLE 5",this,[this]{align("muscle");});
    tools->addAction("Configure executables…",this,&Editor::toolSettings);
    auto help=menuBar()->addMenu("&Help");help->addAction("About HelixEdit",this,[this]{QMessageBox::about(this,"HelixEdit 0.1.0","Independent, offline sequence editor.\nOriginal implementation inspired by familiar sequence workflows.\nNot affiliated with BioEdit or SnapGene.\n\nDevelopment preview: verify results before research use.\nSee README for format and analysis limitations.");});
    auto bar=addToolBar("Workspace");bar->setMovable(false);bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto brand=new QLabel("<b style='color:#7450cd;font-size:21px'>◉ HelixEdit</b> <span style='color:#9b8eaf'> / sequence studio</span>   ");bar->addWidget(brand);
    bar->addAction(actOpen);bar->addAction(actSave);bar->addSeparator();bar->addAction(undo);bar->addAction(redo);
    bar->addAction("+ Sequence",this,&Editor::addSequence);
    auto spacer=new QWidget;spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);bar->addWidget(spacer);
    auto offline=new QLabel("<b style='color:#159884'>● OFFLINE</b>   <span style='color:#9d90ae'>PREVIEW 0.1</span>  ");bar->addWidget(offline);
    auto root=new QWidget;setCentralWidget(root);auto layout=new QVBoxLayout(root);layout->setContentsMargins(18,13,18,8);layout->setSpacing(9);
    // Compact, category-based ribbon: each button triggers the existing menu action,
    // keeping keyboard shortcuts, enablement and behavior in one place.
    auto ribbon=new QTabWidget(root);
    ribbon->setObjectName("commandRibbon");
    ribbon->setDocumentMode(true);
    ribbon->setMaximumHeight(112);
    ribbon->setMinimumHeight(92);
    ribbon->setStyleSheet(
        "QTabWidget#commandRibbon::pane {border:1px solid rgba(148,120,210,100);"
        "border-radius:13px;background:rgba(139,104,218,25);}"
        "QTabWidget#commandRibbon QTabBar::tab {padding:7px 17px;margin-right:3px;"
        "border:1px solid rgba(142,125,193,70);border-radius:9px;"
        "background:rgba(130,110,180,30);font-weight:600;}"
        "QTabWidget#commandRibbon QTabBar::tab:selected {"
        "background:rgba(138,103,226,90);border:1px solid rgba(138,103,226,140);}"
        "QTabWidget#commandRibbon QToolButton {border:1px solid rgba(160,140,200,70);"
        "border-radius:11px;padding:5px 8px;background:rgba(255,255,255,30);}"
        "QTabWidget#commandRibbon QToolButton:hover {"
        "background:rgba(125,103,224,100);border:1px solid rgba(125,103,224,170);}"
        "QTabWidget#commandRibbon QToolButton:pressed {background:rgba(95,74,195,130);}"
    );
    auto addRibbonCategory=[&](const QString &name,
                               const QVector<QPair<QString,QAction*>> &commands){
        auto page=new QWidget(ribbon);
        page->setObjectName("ribbonPage");
        const QString tint=name=="Home"?"rgba(46,174,221,35)":
            name=="Sequence"?"rgba(39,202,144,35)":
            name=="Plasmid"?"rgba(231,120,177,38)":"rgba(166,115,244,38)";
        page->setStyleSheet("QWidget#ribbonPage {background:"+tint+
                            ";border-radius:11px;}");
        auto row=new QHBoxLayout(page);
        row->setContentsMargins(8,4,8,4);
        row->setSpacing(6);
        for(const auto &command:commands){
            if(!command.second)continue;
            auto button=new QToolButton(page);
            button->setText(command.first);
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setIconSize(QSize(22,22));
            button->setMinimumWidth(78);
            button->setMaximumWidth(132);
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
    auto addA=actionFor(edit,"Add sequence…",QStyle::SP_FileIcon);
    auto renameA=actionFor(edit,"Rename active sequence…",QStyle::SP_FileDialogDetailedView);
    auto removeA=actionFor(edit,"Remove selected sequences…",QStyle::SP_TrashIcon);
    auto reverseA=actionFor(edit,"Reverse complement selected rows",QStyle::SP_BrowserReload);
    auto gapA=actionFor(edit,"Insert gap at selected column",QStyle::SP_ArrowRight);
    auto translateA=actionFor(analysis,"Translate active sequence…",QStyle::SP_MediaPlay);
    auto orfA=actionFor(analysis,"Find open reading frames…",QStyle::SP_FileDialogDetailedView);
    auto consensusA=actionFor(analysis,"Consensus",QStyle::SP_DialogApplyButton);
    auto restrictA=actionFor(analysis,"Common restriction sites",QStyle::SP_FileDialogContentsView);
    auto mafftA=actionFor(tools,"Align with MAFFT",QStyle::SP_BrowserReload);
    auto muscleA=actionFor(tools,"Align with MUSCLE 5",QStyle::SP_BrowserReload);
    auto configA=actionFor(tools,"Configure executables…",QStyle::SP_FileDialogDetailedView);
    auto addgeneA=actionFor(addgeneMenu,"Find plasmid by Addgene ID…",QStyle::SP_DirLinkIcon);
    auto importA=actionFor(addgeneMenu,"Import Addgene GenBank into project…",QStyle::SP_DialogOpenButton);
    setRibbonIcon(undo,QStyle::SP_ArrowBack);
    setRibbonIcon(redo,QStyle::SP_ArrowForward);
    addRibbonCategory("Home",{{"Open",openA},{"Save",saveA},
                              {"Undo",undo},{"Redo",redo},{"Export",exportA}});
    addRibbonCategory("Sequence",{{"Add",addA},{"Rename",renameA},{"Remove",removeA},{"Insert gap",gapA},
                                  {"Reverse",reverseA},{"Translate",translateA},{"Find ORFs",orfA}});
    addRibbonCategory("Plasmid",{{"Addgene ID",addgeneA},{"Import GenBank",importA},
                                 {"Restriction",restrictA}});
    addRibbonCategory("Analysis",{{"MAFFT",mafftA},{"MUSCLE",muscleA},
                                  {"Consensus",consensusA},{"ORFs",orfA},{"Tool settings",configA}});
    layout->addWidget(ribbon);
    auto strip=new QHBoxLayout;auto title=new QLabel("<b style='font-size:19px'>Your sequences, in focus.</b>");strip->addWidget(title);strip->addStretch();
    molecule_=new QComboBox;molecule_->addItems({"DNA","RNA","Protein"});molecule_->setToolTip("Verify molecule type; short proteins cannot always be inferred from letters.");strip->addWidget(molecule_);
    search_=new QLineEdit;search_->setPlaceholderText("Find exact motif in active row…");search_->setMaximumWidth(245);strip->addWidget(search_);
    auto find=new QPushButton("Find next");strip->addWidget(find);layout->addLayout(strip);
    notice_=new QLabel;notice_->setWordWrap(true);notice_->setStyleSheet("background:#fff0d8;color:#86602b;border-radius:7px;padding:7px;");layout->addWidget(notice_);
    auto split=new QSplitter;layout->addWidget(split,1);tabs_=new QTabWidget;split->addWidget(tabs_);
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
    auto mapPage=new QWidget;auto ml=new QVBoxLayout(mapPage);mapTitle_=new QLabel;mapTitle_->setStyleSheet("font-size:16px;font-weight:bold;padding:12px;");ml->addWidget(mapTitle_);
    map_=new MapView;ml->addWidget(map_,1);
    auto primer3Button=new QPushButton("Design with Primer3");
    connect(primer3Button,&QPushButton::clicked,this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"Primer3","Primer3 requires a nucleotide template.");return;
        }
        const QString executable=QStandardPaths::findExecutable("primer3_core");
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
        const QByteArray payload=("SEQUENCE_ID=HelixEdit_template\\nSEQUENCE_TEMPLATE="+sequence+
            "\\nPRIMER_TASK=generic\\nPRIMER_PICK_LEFT_PRIMER=1\\nPRIMER_PICK_RIGHT_PRIMER=1\\n"
            "PRIMER_NUM_RETURN=5\\nPRIMER_PRODUCT_SIZE_RANGE=100-1000\\n=\\n").toUtf8();
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
        showAnalysis("Primer3 design output",result+"\\n"+errors+
            "\\nReview primer specificity and thermodynamics independently before experimental use.");
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
        else editor->setPlainText("name,sequence,notes\\n");
        auto actions=new QHBoxLayout;
        auto mapButton=new QPushButton("Map primers to active plasmid");
        auto saveButton=new QPushButton("Save library");
        actions->addWidget(mapButton);actions->addWidget(saveButton);layout->addLayout(actions);
        auto output=new QPlainTextEdit;output->setReadOnly(true);layout->addWidget(output);
        connect(saveButton,&QPushButton::clicked,&dlg,[=]{
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
            QString result=QString("%1 · %2 primers\\n\\n").arg(record.name).arg(primers.size());
            for(const auto &primer:primers){
                auto hits=primerHits(record,primer.sequence);
                result+=QString("%1  [%2 nt; GC %3%; Wallace ~%4 C]  %5 hits\\n")
                    .arg(primer.name).arg(primer.sequence.size())
                    .arg(primerGc(primer.sequence),0,'f',1).arg(primerWallaceTm(primer.sequence))
                    .arg(hits.size());
                for(const auto &hit:hits)
                    result+=QString("  %1–%2  %3%4\\n").arg(hit.start+1).arg(hit.end)
                        .arg(hit.reverse?"reverse":"forward")
                        .arg(hit.end>ungapped(record.sequence).size()?" (wraps origin)":"");
            }
            output->setPlainText(result);
        });
        dlg.exec();
    });
    auto mapButtons=new QHBoxLayout;
    mapButtons->addWidget(primerButton);
    mapButtons->addWidget(primer3Button);
    auto restrictionButton=new QPushButton("Restriction sites");
    mapButtons->addWidget(restrictionButton);
    connect(restrictionButton,&QPushButton::clicked,this,[this]{
        if(model->document.alphabet=="Protein"){
            QMessageBox::information(this,"Restriction sites","Select a nucleotide project first.");
            return;
        }
        const auto &record=model->document.records[activeRow()];
        const auto sites=findRestrictionSites(record);
        QString report=QString("Sequence: %1\\nTopology: %2\\n"
            "Recognition coordinates: 1-based inclusive\\n"
            "Prototype: eight exact recognition motifs; no cut offsets or digest simulation.\\n\\n")
            .arg(record.name,record.circular?"Circular":"Linear");
        if(sites.isEmpty())report+="No sites found for the current motif set.";
        for(const auto &site:sites)
            report+=QString("%1: %2–%3%4\\n").arg(site.enzyme)
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
    auto cfg=new QPushButton("Configure package paths…");tl->addWidget(cfg);connect(cfg,&QPushButton::clicked,this,&Editor::toolSettings);
    log_=new QPlainTextEdit;log_->setReadOnly(true);log_->setPlaceholderText("Job logs appear here. No alignment tools are bundled.");tl->addWidget(log_,1);
    tabs_->addTab(toolPage,"03  Local tools");
    auto side=new QWidget;side->setMinimumWidth(205);side->setMaximumWidth(290);auto sl=new QVBoxLayout(side);sl->setContentsMargins(9,0,0,0);
    auto overview=new QGroupBox("DOCUMENT");auto ov=new QVBoxLayout(overview);stats_=new QLabel;stats_->setWordWrap(true);ov->addWidget(stats_);sl->addWidget(overview);
    auto sel=new QGroupBox("SELECTION");auto se=new QVBoxLayout(sel);selection_=new QLabel;selection_->setWordWrap(true);se->addWidget(selection_);sl->addWidget(sel);
    auto annotations=new QGroupBox("FEATURES / ACTIVE ROW");auto an=new QVBoxLayout(annotations);features_=new QListWidget;an->addWidget(features_);sl->addWidget(annotations,1);
    auto tip=new QLabel("<b style='color:#7c5ce7'>Small app. Clear workflow.</b><br><br>Save .helix projects to retain features and topology. FASTA exports contain sequence only.");tip->setWordWrap(true);sl->addWidget(tip);split->addWidget(side);split->setSizes({860,240});
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
    autosave_=new QTimer(this);autosave_->setInterval(30000);connect(autosave_,&QTimer::timeout,this,[this]{if(!preview_&&!history->isClean())try{
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
    if(auto legend=findChild<QLabel*>("residueLegend"))legend->setText(d.alphabet=="Protein"?"   Protein colours: hydrophobic · basic · acidic · polar · other   /   Double-click a residue to edit":"   A adenine   ·   C cytosine   ·   G guanine   ·   T/U thymine / uracil   /   Double-click a residue to edit");
    QSignalBlocker block(molecule_);molecule_->setCurrentText(d.alphabet);
    stats_->setText(QString("<b style='font-size:25px;color:#7450cd'>%1</b> sequences<br><br><b>%2</b> alignment columns<br><br>%3 · local document").arg(d.records.size()).arg(QLocale().toString(helix::width(d))).arg(d.alphabet));
    notice_->setVisible(!d.warnings.isEmpty());notice_->setText(d.warnings.join(" "));updateSelection();
    statusBar()->showMessage(QString("%1 sequences   ·   %2 columns   ·   .helix retains features   ·   30-edit undo history").arg(d.records.size()).arg(helix::width(d)));
}
void Editor::updateSelection(){
    if(model->document.records.isEmpty())return;
    int row=activeRow();const auto &r=model->document.records[row];auto ranges=table->selectionModel()->selection();
    QString pos=ranges.isEmpty()?"No selected cells":QString("Alignment columns %1–%2").arg(ranges.first().left()+1).arg(ranges.first().right()+1);
    selection_->setText(QString("<b>%1</b><br><br>%2 residues<br>%3<br><br>GC: %4").arg(r.name.toHtmlEscaped()).arg(ungapped(r.sequence).size()).arg(pos).arg(model->document.alphabet=="Protein"?"n/a":number(gcPercent(r.sequence))));
    map_->record=r;map_->alphabet=model->document.alphabet;map_->update();mapTitle_->setTextFormat(Qt::PlainText);mapTitle_->setText(r.name);
    features_->clear();for(int i=0;i<r.features.size();++i){const auto &f=r.features[i];auto item=new QListWidgetItem(QString("%1\n%2–%3  %4").arg(f.name).arg(f.start+1).arg(f.end).arg(f.reverse?"←":"→"));item->setForeground(featureColor(i));features_->addItem(item);}
    if(r.features.isEmpty()){auto item=new QListWidgetItem("No annotations\nAdd features from Sequence map.");item->setFlags(Qt::NoItemFlags);features_->addItem(item);}
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
void Editor::selectFeature(int idx){const auto &r=model->document.records[activeRow()];if(idx<0||idx>=r.features.size())return;auto f=r.features[idx];int ungap=0,left=-1,right=-1;for(int i=0;i<r.sequence.size();++i)if(r.sequence[i]!='-'){if(ungap==f.start)left=i;if(ungap==f.end-1)right=i;++ungap;}if(left<0||right<0)return;int row=activeRow();QItemSelection sel(model->index(row,left),model->index(row,right));table->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);table->scrollTo(model->index(row,left));tabs_->setCurrentIndex(0);}
void Editor::showAnalysis(const QString &title,const QString &text){QDialog d(this);d.setWindowTitle(title);auto l=new QVBoxLayout(&d);auto view=new QPlainTextEdit;view->setReadOnly(true);view->setPlainText(text);l->addWidget(view);auto b=new QDialogButtonBox(QDialogButtonBox::Close);l->addWidget(b);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);d.resize(650,400);d.exec();}
void Editor::translateSelected(){if(model->document.alphabet=="Protein"){QMessageBox::information(this,"Translate","Select DNA or RNA first.");return;}bool ok=false;int frame=QInputDialog::getInt(this,"Translation","Frame (standard genetic code, complete codons only)",1,1,3,1,&ok);if(!ok)return;runSafely([&]{const auto &r=model->document.records[activeRow()];showAnalysis("Translation",">"+r.name+" frame "+QString::number(frame)+"\n"+translate(r.sequence,frame-1)+"\n\nStandard code. Ambiguous codons → X. Trailing partial codons omitted. Stops → *. Full active row, not selected columns.");});}
void Editor::findNext(){QString motif=search_->text().trimmed().toUpper();if(motif.isEmpty())return;if(motif.contains(QRegularExpression("[^A-Z*?-]"))){statusBar()->showMessage("Search uses exact residue symbols; no regular expressions.");return;}int row=activeRow();const auto &s=model->document.records[row].sequence;int from=table->currentIndex().isValid()?table->currentIndex().column()+1:0;int at=s.indexOf(motif,from);if(at<0)at=s.indexOf(motif);if(at<0){statusBar()->showMessage("No exact match in active row. Search does not cross a circular origin.");return;}table->setCurrentIndex(model->index(row,at));QItemSelection sel(model->index(row,at),model->index(row,at+motif.size()-1));table->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);table->scrollTo(model->index(row,at));tabs_->setCurrentIndex(0);statusBar()->showMessage("Exact match at alignment column "+QString::number(at+1));}
void Editor::toolSettings(){QDialog d(this);d.setWindowTitle("Local package executables");auto l=new QFormLayout(&d);QSettings settings;QMap<QString,QLineEdit*> fields;for(const auto &key:QStringList{"mafft","muscle","blastn"}){auto field=new QLineEdit(settings.value("tools/"+key).toString());field->setPlaceholderText(QStandardPaths::findExecutable(key));fields[key]=field;l->addRow(key,field);}auto note=new QLabel("Blank = search PATH. Use an executable path, without flags.\nMAFFT 7 and MUSCLE 5 alignment adapters are implemented.\nBLAST+ path discovery is preparatory; BLAST search UI is planned.\nOn Apple Silicon, Homebrew commonly installs in /opt/homebrew/bin.");note->setWordWrap(true);l->addRow(note);auto b=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);l->addRow(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);d.resize(560,270);if(d.exec()==QDialog::Accepted)for(auto it=fields.cbegin();it!=fields.cend();++it)settings.setValue("tools/"+it.key(),it.value()->text().trimmed());}
void Editor::align(const QString &tool){
    if(busy()){QMessageBox::information(this,"Alignment running","One job at a time. The current document remains locked until it finishes.");return;}
    if(model->document.records.size()<2){QMessageBox::information(this,"Alignment","Add at least two sequences.");return;}
    QSettings settings;QString exe=settings.value("tools/"+tool).toString();if(exe.isEmpty())exe=QStandardPaths::findExecutable(tool);
#ifdef Q_OS_MACOS
    if(exe.isEmpty()&&QFileInfo::exists("/opt/homebrew/bin/"+tool))exe="/opt/homebrew/bin/"+tool;
#endif
    if(exe.isEmpty()||!QFileInfo(exe).isExecutable()){QMessageBox::information(this,"Package not found","Install "+tool+" for your architecture, then configure its executable path under Tools.");return;}
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
    auto undoActions=menuBar()->actions()[1]->menu()->actions();for(auto a:undoActions)a->setEnabled(false);
    log_->clear();log_->appendPlainText("Running "+tool+"\n"+exe+"\n"+args.join(" ")+"\n");tabs_->setCurrentIndex(2);
    if(tool=="muscle")connect(job,&QProcess::readyReadStandardOutput,this,[this,job]{auto text=QString::fromUtf8(job->readAllStandardOutput());if(log_->document()->blockCount()>2000)log_->clear();log_->appendPlainText(text.right(16000));});
    connect(job,&QProcess::readyReadStandardError,this,[this,job]{auto text=QString::fromUtf8(job->readAllStandardError());if(log_->document()->blockCount()>2000)log_->clear();log_->appendPlainText(text.right(16000));});
    auto finish=[this,job,original,out,undoActions,tool](int code,QProcess::ExitStatus status){
        if(process_!=job)return;process_=nullptr;table->setEnabled(true);molecule_->setEnabled(true);history->setActive(true);for(auto a:undoActions)a->setEnabled(true);if(undoActions.size()>=2){undoActions[0]->setEnabled(history->canUndo());undoActions[1]->setEnabled(history->canRedo());}
        if(code==0&&status==QProcess::NormalExit&&!job->property("cancelled").toBool()){try{auto aligned=readDocument(out);auto d=acceptAlignment(original,aligned);commit(d,"Apply local alignment");log_->appendPlainText("\nValidated and applied. Original names, features and row order preserved. Undo available.");}catch(const std::exception &e){log_->appendPlainText("Rejected output: "+QString::fromUtf8(e.what()));}}
        else {
        if(tool=="muscle" && log_->toPlainText().contains("sizeof(void *)"))log_->appendPlainText("This MUSCLE build has a known ARM64 pointer-width detection issue. Prefer MAFFT, or build MUSCLE with scripts/build_muscle_arm64.sh and configure its executable path.");
        log_->appendPlainText(job->property("cancelled").toBool()?"Job cancelled. Document unchanged.":"Job failed. Document unchanged. Exit code: "+QString::number(code));
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
