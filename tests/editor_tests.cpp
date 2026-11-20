#include "editor.h"
#include <QtTest>
using namespace helix;
class EditorTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){QCoreApplication::setOrganizationName("HelixEditTests");QCoreApplication::setApplicationName("IsolatedTests");QStandardPaths::setTestModeEnabled(true);}
    void editUndoSaveAndMap() {
        Editor editor(true);auto original=editor.model->document;editor.editResidue(0,0,'C');QCOMPARE(editor.model->document.records[0].sequence[0],QChar('C'));
        editor.history->undo();QCOMPARE(editor.model->document.records[0].sequence,original.records[0].sequence);
        editor.history->redo();QCOMPARE(editor.model->document.records[0].sequence[0],QChar('C'));
        QTemporaryDir tmp;editor.savePath(tmp.filePath("edited.helix"));QVERIFY(editor.history->isClean());
        editor.exportMap(tmp.filePath("map.svg"));QFile svg(tmp.filePath("map.svg"));QVERIFY(svg.open(QIODevice::ReadOnly));QVERIFY(svg.readAll().contains("<svg"));
        editor.loadPath(tmp.filePath("edited.helix"));QCOMPARE(editor.model->document.records[0].sequence[0],QChar('C'));
    }
    void compactWorkspaceAndNavigator() {
        Editor editor(true);
        auto studio=editor.findChild<QTabWidget*>("studioTabs");
        QVERIFY(studio);
        const QStringList expected={"Map","Sequence","Enzymes","Features","Primers","History","Tools"};
        QCOMPARE(studio->count(),int(expected.size()));
        for(int i=0;i<expected.size();++i)QCOMPARE(studio->tabText(i),expected[i]);
        QCOMPARE(studio->currentIndex(),0);
        auto map=editor.findChild<MapView*>("plasmidMap");
        auto navigator=editor.findChild<QListWidget*>("sequenceNavigator");
        auto annotations=editor.findChild<QTreeWidget*>("annotationExplorer");
        auto enzymes=editor.findChild<QListWidget*>("enzymeExplorer");
        QVERIFY(map);QVERIFY(navigator);QVERIFY(annotations);QVERIFY(enzymes);
        auto doc=parseFasta(">reference\nGAATTCGGATCCATGAAACCC\n>variant\nGAATTCGGATCCATGAAACCT\n");
        doc.records[1].features.append(Feature{"region","misc_feature",2,14,false});
        editor.setDocument(doc);
        QCOMPARE(navigator->count(),2);
        navigator->setCurrentRow(1);
        QCOMPARE(editor.table->currentIndex().row(),1);
        QCOMPARE(annotations->topLevelItemCount(),1);
        QVERIFY(enzymes->count()>0);
        const auto initial=map->zoom;
        auto plus=editor.findChild<QToolButton*>("mapZoomIn");
        auto reset=editor.findChild<QToolButton*>("mapZoomReset");
        QVERIFY(plus);QVERIFY(reset);
        plus->click();QVERIFY(map->zoom>initial);
        reset->click();QCOMPARE(map->zoom,initial);
        auto filter=editor.findChild<QLineEdit*>("navigatorFilter");
        QVERIFY(filter);
        filter->setText("reference");
        QVERIFY(navigator->item(1)->isHidden());
        QVERIFY(!navigator->item(0)->isHidden());
        filter->clear();
        QVERIFY(!navigator->item(1)->isHidden());
    }
    void gapsAndFeatureCoordinates() {
        Editor e(true);auto d=parseFasta(">one\nACGT\n>two\nACGT\n");d.records[0].features.append(Feature{"region","CDS",1,4,false});e.setDocument(d);
        QItemSelection sel(e.model->index(0,1),e.model->index(1,1));e.table->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);
        e.gapColumns(true);QCOMPARE(e.model->document.records[0].sequence,QString("A-CGT"));QCOMPARE(e.model->document.records[0].features[0].start,1);
        e.history->undo();e.table->selectionModel()->select(QItemSelection(e.model->index(0,1),e.model->index(0,2)),QItemSelectionModel::ClearAndSelect);e.gapColumns(false);
        QCOMPARE(e.model->document.records[0].sequence,QString("AT"));QVERIFY(e.model->document.records[0].features.isEmpty());e.history->undo();
        e.table->setCurrentIndex(e.model->index(0,0));e.reverseSelected();QCOMPARE(e.model->document.records[0].features[0].start,0);QCOMPARE(e.model->document.records[0].features[0].end,3);QVERIFY(e.model->document.records[0].features[0].reverse);
    }
    void invalidCellEditRejected() {
        Editor e(true);QVERIFY(!e.model->setData(e.model->index(0,0),"P",Qt::EditRole));QVERIFY(!e.model->setData(e.model->index(0,0),"AC",Qt::EditRole));
        QVERIFY(e.model->setData(e.model->index(0,0),"c",Qt::EditRole));QCOMPARE(e.model->document.records[0].sequence[0],QChar('C'));
    }
#ifndef Q_OS_WIN
    void nativeAligners_data() {
        QTest::addColumn<QString>("tool");QTest::newRow("MAFFT")<<QString("mafft");QTest::newRow("MUSCLE5")<<QString("muscle");
    }
    void nativeAligners() {
        QFETCH(QString,tool);auto exe=QStandardPaths::findExecutable(tool);if(exe.isEmpty())QSKIP("Optional native aligner not installed");
        QSettings settings;settings.setValue("tools/"+tool,exe);Editor e(true);
        auto original=parseFasta(">reference\nATGGCTACGTTAGGCTACGATCGATCGTAGCTAGCATCGATCGTAGCTAGC\n>variant_A\nATGGCTACGTTAGGCTACGATCGATCGTAGCTAGCATCGATCGTAGCTTGC\n>variant_B\nATGGCTACGTTAGGCTACGATCGAT---TAGCTAGCATCGATCGTAGCTAGC\n");e.setDocument(original);
        QPushButton *button=nullptr;for(auto b:e.findChildren<QPushButton*>())if(b->text()=="Align document with "+tool.toUpper())button=b;QVERIFY(button);button->click();
        QTRY_VERIFY_WITH_TIMEOUT(e.table->isEnabled(),30000);
        auto log=e.findChild<QPlainTextEdit*>("alignerLog");QVERIFY(log);QVERIFY2(e.history->count()==1,qPrintable(log->toPlainText()));
        for(int i=0;i<original.records.size();++i)QCOMPARE(ungapped(e.model->document.records[i].sequence),ungapped(original.records[i].sequence));
        settings.remove("tools/"+tool);
    }
    void alignerAdapter() {
        QTemporaryDir tmp;QString script=tmp.filePath("fake-mafft");QFile f(script);QVERIFY(f.open(QIODevice::WriteOnly));f.write("#!/bin/sh\nprintf '>hx1\\nA-G\\n>hx0\\nACG\\n'\n");f.close();QVERIFY(f.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner));
        QSettings settings;settings.setValue("tools/mafft",script);Editor e(true);e.setDocument(parseFasta(">name A\nACG\n>name B\nAG\n"));
        QPushButton *button=nullptr;for(auto b:e.findChildren<QPushButton*>())if(b->text()=="Align document with MAFFT")button=b;QVERIFY(button);button->click();
        QTRY_COMPARE_WITH_TIMEOUT(e.history->count(),1,5000);QCOMPARE(e.model->document.records[1].name,QString("name B"));QCOMPARE(e.model->document.records[1].sequence,QString("A-G"));
        e.history->undo();QCOMPARE(e.model->document.records[1].sequence,QString("AG"));
        QVERIFY(f.open(QIODevice::WriteOnly|QIODevice::Truncate));f.write("#!/bin/sh\nprintf '>hx0\\nATG\\n>hx1\\nA-G\\n'\n");f.close();button->click();
        auto log=e.findChild<QPlainTextEdit*>("alignerLog");QVERIFY(log);
        QTRY_VERIFY_WITH_TIMEOUT(log->toPlainText().contains("Rejected output"),5000);QCOMPARE(e.model->document.records[0].sequence,QString("ACG"));
        settings.remove("tools/mafft");
    }
#endif
};
QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
