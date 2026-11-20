#pragma once
#include "sequence.h"
#include <QtWidgets>
#include <functional>

namespace helix {

// A read-only, responsive view of the selected alignment row. The editable
// QTableView remains available when Wrap to Window is switched off.
class WrappedSequenceView final : public QAbstractScrollArea {
public:
    explicit WrappedSequenceView(QWidget *parent=nullptr);
    void setRecord(const Record &record,const QString &alphabet);
    void setShowComplement(bool value);
    void setShowFeatures(bool value);
    bool showComplement() const {return showComplement_;}
    bool showFeatures() const {return showFeatures_;}
    int basesPerLine() const {return basesPerLine_;}
    int lineCount() const;
    int alignmentColumnAt(const QPoint &viewportPos) const;
    int selectedColumn() const {return selectedColumn_;}
    void setSelectedColumn(int column);
    void scrollToAlignmentColumn(int column);
    std::function<void(int)> onColumnActivated;
    std::function<void(int)> onEditColumn;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Record record_;
    QString alphabet_="DNA";
    QVector<int> coordinatePrefix_;
    QVector<int> baseColumns_;
    bool showComplement_=true;
    bool showFeatures_=true;
    int basesPerLine_=70;
    int glyphWidth_=11;
    int lineHeight_=17;
    int blockHeight_=114;
    int selectedColumn_=-1;
    static constexpr int leftGutter_=62;
    static constexpr int rightGutter_=70;
    static constexpr int topGutter_=16;
    static constexpr int groupSize_=10;
    static constexpr int groupGap_=9;

    void recalculateLayout();
    int xForOffset(int offset) const;
    int alignmentFromResidue(int position) const;
    int totalHeight() const;
    QFont sequenceFont() const;
};
} // namespace helix
