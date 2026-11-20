#pragma once
#include "sequence.h"
#include <QtWidgets>
#include <memory>
namespace helix {
class AlignmentModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit AlignmentModel(QObject *p=nullptr):QAbstractTableModel(p){}
    Document document;
    int columns_=0;
    int rowCount(const QModelIndex &p={}) const override { return p.isValid()?0:document.records.size(); }
    int columnCount(const QModelIndex &p={}) const override { return p.isValid()?0:columns_; }
    QVariant data(const QModelIndex &i,int role) const override;
    QVariant headerData(int section,Qt::Orientation o,int role) const override;
    Qt::ItemFlags flags(const QModelIndex &i) const override;
    bool setData(const QModelIndex &i,const QVariant &v,int role) override;
    void replace(const Document &d);
signals:
    void residueEdit(int row,int col,QChar residue);
};
class MapView : public QWidget {
    Q_OBJECT
public:
    explicit MapView(QWidget *p=nullptr):QWidget(p){ setMinimumSize(400,330); }
    Record record;
    QString alphabet="DNA";
    bool showEnzymes=true;
    bool showFeatures=true;
    qreal zoom=1.0;
    void setZoom(qreal value) {zoom=qBound<qreal>(0.65,value,1.35);update();}
    void draw(QPainter &p,const QRectF &bounds) const;
protected:
    void paintEvent(QPaintEvent *) override;
};
class Editor : public QMainWindow {
    Q_OBJECT
public:
    explicit Editor(bool preview=false);
    AlignmentModel *model;
    QTableView *table;
    QUndoStack *history;
    void applyDocument(const Document &d);
    void editResidue(int row,int col,QChar residue);
    void gapColumns(bool insert);
    void reverseSelected();
    void loadPath(const QString &path);
    void savePath(const QString &path);
    QString currentPath() const {return path_;}
    void exportMap(const QString &path);
    void setDocument(const Document &d); // new document, clears history
protected:
    void closeEvent(QCloseEvent *e) override;
private:
    bool preview_;
    QString path_,recoveryPath_;
    QTabWidget *tabs_;
    MapView *map_=nullptr;
    QLabel *stats_,*selection_,*notice_,*mapTitle_;
    QListWidget *features_;
    QListWidget *navigator_=nullptr;
    QListWidget *enzymeList_=nullptr;
    QTreeWidget *annotationTable_=nullptr;
    QComboBox *molecule_;
    QCheckBox *mapEnzymeToggle_=nullptr;
    QCheckBox *mapFeatureToggle_=nullptr;
    QLineEdit *search_;
    QPlainTextEdit *log_;
    QProcess *process_=nullptr;
    std::unique_ptr<QTemporaryDir> temp_;
    QTimer *autosave_;
    void commit(const Document &d,const QString &label);
    void refresh();
    void updateSelection();
    bool canReplace();
    bool busy() const;
    void open();
    bool save(bool as=false);
    void exportFile();
    void addSequence();
    void addFeature();
    void translateSelected();
    void findNext();
    void showSettings();
    void align(const QString &tool);
    void cancelAlignment();
    void showAnalysis(const QString &title,const QString &text);
    void selectFeature(int index);
    int activeRow() const;
    void runSafely(const std::function<void()> &fn);
};
}
