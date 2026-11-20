#include "wrapped_sequence.h"
#include "themes.h"

#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QFontDatabase>
#include <algorithm>
#include <limits>

namespace helix {
namespace {
QChar complement(QChar residue,bool rna) {
    switch(residue.toUpper().toLatin1()) {
        case 'A':return rna?'U':'T';
        case 'T':case 'U':return 'A';
        case 'G':return 'C';
        case 'C':return 'G';
        case 'R':return 'Y';
        case 'Y':return 'R';
        case 'S':return 'S';
        case 'W':return 'W';
        case 'K':return 'M';
        case 'M':return 'K';
        case 'B':return 'V';
        case 'V':return 'B';
        case 'D':return 'H';
        case 'H':return 'D';
        case 'N':return 'N';
        default:return residue;
    }
}
QColor baseColor(QChar residue,bool protein,bool dark) {
    if(protein) {
        if(QStringLiteral("AVLIMFWY").contains(residue))return QColor(dark?"#1c6257":"#b8f4db");
        if(QStringLiteral("KRH").contains(residue))return QColor(dark?"#53437a":"#dfd4ff");
        if(QStringLiteral("DE").contains(residue))return QColor(dark?"#6d3e51":"#ffdaea");
        if(QStringLiteral("STNQ").contains(residue))return QColor(dark?"#284e72":"#c9ecff");
        return QColor(dark?"#50495b":"#f9edcd");
    }
    if(residue=='A')return QColor(dark?"#195d4b":"#bef5dd");
    if(residue=='C')return QColor(dark?"#205277":"#c6eaff");
    if(residue=='G')return QColor(dark?"#67522f":"#fff0bb");
    if(residue=='T'||residue=='U')return QColor(dark?"#713d50":"#ffd6e6");
    return QColor(dark?"#464355":"#eae9f6");
}
QColor annotationColor(int i) {
    static const QColor colors[]={
        QColor("#f3c444"),QColor("#45dcb5"),QColor("#997cff"),
        QColor("#fb6e9c"),QColor("#64b4ff"),QColor("#ff995a")
    };
    return colors[i%6];
}
}

WrappedSequenceView::WrappedSequenceView(QWidget *parent):QAbstractScrollArea(parent) {
    setObjectName("wrappedSequenceView");
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFocusPolicy(Qt::StrongFocus);
    setToolTip("Wrapped view shows one active alignment row; select a base to locate its editable grid cell.");
    QObject::connect(verticalScrollBar(),&QScrollBar::valueChanged,viewport(),
                     [this]{viewport()->update();});
    recalculateLayout();
}

QFont WrappedSequenceView::sequenceFont() const {
    QFont font=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    font.setWeight(QFont::DemiBold);
    return font;
}

int WrappedSequenceView::lineCount() const {
    return qMax(1,(int(record_.sequence.size())+basesPerLine_-1)/basesPerLine_);
}

int WrappedSequenceView::totalHeight() const {
    const qint64 height=qint64(lineCount())*blockHeight_+2*topGutter_;
    return int(qMin<qint64>(height,std::numeric_limits<int>::max()));
}

void WrappedSequenceView::recalculateLayout() {
    const QFontMetrics fm(sequenceFont());
    glyphWidth_=qMax(9,fm.horizontalAdvance(QChar('M'))+2);
    lineHeight_=qMax(16,fm.height()+2);
    const int available=qMax(90,viewport()->width()-leftGutter_-rightGutter_-14);
    const int groupPixels=groupSize_*glyphWidth_+groupGap_;
    basesPerLine_=qMax(10,(available/groupPixels)*groupSize_);
    blockHeight_=showComplement_?123:103;
    if(showFeatures_&&!record_.features.isEmpty())blockHeight_+=36;
    verticalScrollBar()->setSingleStep(qMax(20,blockHeight_/3));
    verticalScrollBar()->setPageStep(qMax(10,viewport()->height()));
    verticalScrollBar()->setRange(0,qMax(0,totalHeight()-viewport()->height()));
    viewport()->update();
}

void WrappedSequenceView::setRecord(const Record &record,const QString &alphabet) {
    bool changed=record_.sequence!=record.sequence ||
        record_.name!=record.name || alphabet_!=alphabet ||
        record_.features.size()!=record.features.size();
    if(!changed)for(int i=0;i<record.features.size();++i) {
        const auto &before=record_.features[i],&after=record.features[i];
        if(before.name!=after.name || before.type!=after.type ||
           before.start!=after.start || before.end!=after.end ||
           before.reverse!=after.reverse) {changed=true;break;}
    }
    record_=record;
    alphabet_=alphabet;
    coordinatePrefix_.clear();
    baseColumns_.clear();
    coordinatePrefix_.reserve(record_.sequence.size()+1);
    coordinatePrefix_.append(0);
    for(int col=0;col<record_.sequence.size();++col) {
        if(record_.sequence[col]!='-')baseColumns_.append(col);
        coordinatePrefix_.append(baseColumns_.size());
    }
    if(changed) {
        selectedColumn_=qMin(selectedColumn_,int(record_.sequence.size())-1);
        recalculateLayout();
    } else viewport()->update();
}

void WrappedSequenceView::setShowComplement(bool value) {
    // Peptides have no complementary strand.
    if(showComplement_==value)return;
    showComplement_=value;
    recalculateLayout();
}

void WrappedSequenceView::setShowFeatures(bool value) {
    if(showFeatures_==value)return;
    showFeatures_=value;
    recalculateLayout();
}

int WrappedSequenceView::xForOffset(int offset) const {
    return leftGutter_ + offset*glyphWidth_ + (offset/groupSize_)*groupGap_;
}

int WrappedSequenceView::alignmentFromResidue(int position) const {
    if(position<0||position>=baseColumns_.size())return -1;
    return baseColumns_.at(position);
}

int WrappedSequenceView::alignmentColumnAt(const QPoint &pos) const {
    const int absoluteY=pos.y()+verticalScrollBar()->value()-topGutter_;
    if(absoluteY<0||record_.sequence.isEmpty())return -1;
    const int block=absoluteY/blockHeight_;
    if(block<0||block>=lineCount())return -1;
    const int within=absoluteY%blockHeight_;
    if(within<31||within>(showComplement_&&alphabet_!="Protein"?94:65))return -1;
    const int start=block*basesPerLine_;
    const int count=qMin(basesPerLine_,int(record_.sequence.size())-start);
    for(int j=0;j<count;++j) {
        if(pos.x()>=xForOffset(j)-2 && pos.x()<xForOffset(j)+glyphWidth_)
            return start+j;
    }
    return -1;
}

void WrappedSequenceView::setSelectedColumn(int column) {
    if(selectedColumn_==column)return;
    selectedColumn_=column;
    viewport()->update();
}

void WrappedSequenceView::scrollToAlignmentColumn(int column) {
    if(column<0||column>=record_.sequence.size())return;
    const int targetBlock=column/basesPerLine_;
    const int top=topGutter_+targetBlock*blockHeight_;
    const int currentTop=verticalScrollBar()->value();
    if(top<currentTop||top+blockHeight_>currentTop+viewport()->height())
        verticalScrollBar()->setValue(qMax(0,top-blockHeight_));
    setSelectedColumn(column);
}

void WrappedSequenceView::resizeEvent(QResizeEvent *event) {
    QAbstractScrollArea::resizeEvent(event);
    recalculateLayout();
}

void WrappedSequenceView::mousePressEvent(QMouseEvent *event) {
    if(event->button()==Qt::LeftButton) {
        const int col=alignmentColumnAt(event->pos());
        if(col>=0) {
            setSelectedColumn(col);
            if(onColumnActivated)onColumnActivated(col);
            event->accept();
            return;
        }
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void WrappedSequenceView::paintEvent(QPaintEvent *) {
    QPainter p(viewport());
    p.setRenderHint(QPainter::Antialiasing,true);
    const bool dark=isDarkTheme(savedTheme());
    const QColor background(dark?"#151c28":"#f7faff");
    const QColor card(dark?"#222b3b":"#ffffff");
    const QColor titleColor(dark?"#f1f5fc":"#253750");
    const QColor muted(dark?"#a4b3c8":"#6b7f9b");
    const QColor rule(dark?"#73849b":"#a1b2c9");
    p.fillRect(viewport()->rect(),background);
    const QFont mono=sequenceFont();
    QFont small=mono;small.setPointSize(8);small.setWeight(QFont::Normal);
    const int scrolled=verticalScrollBar()->value();
    const int first=qMax(0,(scrolled-topGutter_)/blockHeight_);
    const int last=qMin(lineCount()-1,
        (scrolled+viewport()->height()-topGutter_)/blockHeight_+1);
    if(record_.sequence.isEmpty()) {
        p.setPen(muted);
        p.drawText(viewport()->rect(),Qt::AlignCenter,
                   "No residues in the active sequence");
        return;
    }
    for(int line=first;line<=last;++line) {
        const int start=line*basesPerLine_;
        const int stop=qMin(int(record_.sequence.size()),start+basesPerLine_);
        if(stop<=start)continue;
        const int top=topGutter_+line*blockHeight_-scrolled;
        const QRectF panel(5,top,qMax(10,viewport()->width()-10),blockHeight_-8);
        p.setPen(QPen(dark?QColor("#36445a"):QColor("#d8e5f4"),1));
        p.setBrush(card);
        p.drawRoundedRect(panel,10,10);
        const int rulerY=top+23;
        const int strandY=top+35;
        const int seqY=top+38;
        const int axisY=top+66;
        const int reverseY=top+74;
        const int lastX=xForOffset(stop-start);
        p.setFont(small);p.setPen(muted);
        p.drawText(QRect(8,top+7,44,15),Qt::AlignRight,
                   QString::number(coordinatePrefix_.at(start)+1));
        p.drawText(QRect(viewport()->width()-70,top+7,62,15),
                   Qt::AlignRight,QString::number(coordinatePrefix_.at(stop)));
        p.drawText(QRect(14,seqY-1,39,20),Qt::AlignCenter,
                   alphabet_=="Protein"?"AA":"5′");
        if(showComplement_&&alphabet_!="Protein")
            p.drawText(QRect(14,reverseY-1,39,20),Qt::AlignCenter,"3′");
        // Aligned-column scale with ungapped base coordinates above major ticks.
        p.setPen(QPen(rule,1));
        p.drawLine(QPointF(leftGutter_-2,axisY),QPointF(lastX-2,axisY));
        for(int local=0;local<=stop-start;local+=groupSize_) {
            const int x=xForOffset(local);
            p.drawLine(QPointF(x-2,axisY-5),QPointF(x-2,axisY+5));
            const int coord=coordinatePrefix_.at(qMin(start+local,stop))+1;
            if(local<stop-start) {
                p.setFont(small);p.setPen(muted);
                p.drawText(QRect(x-15,rulerY-7,45,16),Qt::AlignLeft,
                           QString::number(coord));
                p.setPen(QPen(rule,1));
            }
        }
        p.setFont(mono);
        const QFontMetrics metrics(mono);
        for(int col=start;col<stop;++col) {
            const int local=col-start;
            const int x=xForOffset(local);
            const QChar base=record_.sequence.at(col).toUpper();
            const QRectF nucleotide(x-1,seqY-2,glyphWidth_+1,lineHeight_+2);
            p.setPen(Qt::NoPen);
            p.setBrush(col==selectedColumn_
                           ?QColor(dark?"#ffe589":"#7f6af0")
                           :baseColor(base,alphabet_=="Protein",dark));
            p.drawRoundedRect(nucleotide,3,3);
            p.setPen(col==selectedColumn_
                ?QColor(dark?"#2c2741":"#ffffff")
                :QColor(dark?"#f7f9fc":"#23354b"));
            p.drawText(nucleotide,Qt::AlignCenter,QString(base));
            if(showComplement_&&alphabet_!="Protein") {
                const QChar c=complement(base,alphabet_=="RNA");
                p.setPen(Qt::NoPen);
                p.setBrush(dark?QColor("#2b394c"):QColor("#eaf1fc"));
                p.drawRoundedRect(QRectF(x-1,reverseY-2,glyphWidth_+1,lineHeight_+2),2,2);
                p.setPen(dark?QColor("#bed2ea"):QColor("#4a5e7a"));
                p.drawText(QRectF(x-1,reverseY-2,glyphWidth_+1,lineHeight_+2),
                           Qt::AlignCenter,QString(c));
            }
        }
        if(showFeatures_&&!record_.features.isEmpty()) {
            const int featureY=top+(showComplement_&&alphabet_!="Protein"?105:84);
            for(int i=0;i<record_.features.size();++i) {
                const auto &feature=record_.features[i];
                if(feature.start<0||feature.end<=feature.start ||
                   feature.end>baseColumns_.size())continue;
                const int begin=alignmentFromResidue(feature.start);
                const int finish=alignmentFromResidue(feature.end-1)+1;
                const int visibleBegin=qMax(start,begin);
                const int visibleFinish=qMin(stop,finish);
                if(visibleFinish<=visibleBegin)continue;
                const qreal x=xForOffset(visibleBegin-start);
                const qreal w=qMax<qreal>(3,xForOffset(visibleFinish-start)-x-3);
                const qreal y=featureY+(i%2)*15;
                p.setPen(Qt::NoPen);
                p.setBrush(annotationColor(i));
                p.drawRoundedRect(QRectF(x,y,w,12),3,3);
                if(w>55) {
                    p.setFont(small);
                    p.setPen(QColor("#222638"));
                    const QString label=QFontMetrics(small)
                        .elidedText(feature.name,Qt::ElideRight,int(w)-10);
                    p.drawText(QRectF(x+5,y,w-9,12),Qt::AlignVCenter,label);
                }
            }
        }
    }
}
} // namespace helix
