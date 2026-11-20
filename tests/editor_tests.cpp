#include "editor.h"
#include "themes.h"
#include "source_discovery.h"
#include "restriction.h"
#include <algorithm>
#include <QtSvg/QSvgRenderer>
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
    void bundledExamplesAreAccessibleAndBounded() {
        struct Spec {QString file;QString alphabet;int count;};
        const QVector<Spec> specs={
            {":/examples/samples/dna_orchid_6.fasta","DNA",6},
            {":/examples/samples/rna_transcripts_3.fasta","RNA",3},
            {":/examples/samples/proteins_5.fasta","Protein",5}
        };
        Editor editor(true);
        auto menu=editor.findChild<QMenu*>("exampleMenu");
        QVERIFY(menu);
        QCOMPARE(int(menu->actions().size()),int(specs.size()));
        for(int i=0;i<specs.size();++i){
            QFile resource(specs[i].file);
            QVERIFY2(resource.open(QIODevice::ReadOnly),qPrintable(specs[i].file));
            auto doc=parseFasta(QString::fromUtf8(resource.readAll()));
            doc.alphabet=specs[i].alphabet;
            validateDocument(doc);
            QCOMPARE(int(doc.records.size()),specs[i].count);
            for(const auto &record:doc.records){
                QVERIFY2(!record.name.contains("gi|"),qPrintable(record.name));
                QVERIFY(record.name.size()<48);
                QVERIFY(ungapped(record.sequence).size()>0);
                QVERIFY2(ungapped(record.sequence).size()<=400,qPrintable(record.name));
                if(doc.alphabet=="RNA") {
                    QVERIFY(record.sequence.contains('U'));
                    QVERIFY(!record.sequence.contains('T'));
                }
            }
            menu->actions().at(i)->trigger();
            auto chart=editor.findChild<CompositionView*>("compositionChart");
            QVERIFY(chart);
            QVERIFY(chart->ungappedLength()>0);
            auto tabs=editor.findChild<QTabWidget*>("studioTabs");
            QVERIFY(tabs);
            QCOMPARE(tabs->currentIndex(),1); // Open FASTA in the visual Sequence tab.
            QCOMPARE(editor.model->document.alphabet,specs[i].alphabet);
            QCOMPARE(int(editor.model->document.records.size()),specs[i].count);
            QCOMPARE(editor.currentPath(),QString());
        }
    }
    void themeAwareSequenceColorsAndBundledIcon() {
        const QString previous=savedTheme();
        Editor editor(true);
        const QModelIndex item=editor.model->index(0,0);
        QVERIFY(item.isValid());
        applyTheme("Light");
        const QColor light=editor.model->data(item,Qt::BackgroundRole).value<QColor>();
        applyTheme("Dark");
        const QColor dark=editor.model->data(item,Qt::BackgroundRole).value<QColor>();
        QVERIFY(light.isValid());
        QVERIFY(dark.isValid());
        QVERIFY2(light!=dark,"Residues should use distinct light and dark color palettes");
        QSvgRenderer icon(QStringLiteral(":/packaging/helixedit.svg"));
        QVERIFY2(icon.isValid(),"The original SVG icon must be embedded in Qt resources");
        applyTheme(previous);
    }
    void graphicalStatisticsDashboard() {
        Editor editor(true);
        QAction *stats=nullptr;
        for(auto *action:editor.findChildren<QAction*>()) {
            if(action->text()=="Sequence statistics…"){stats=action;break;}
        }
        QVERIFY(stats);
        bool dialogOpened=false,hasChart=false;
        QTimer::singleShot(0,&editor,[&]{
            auto *table=editor.findChild<QTableWidget*>("sequenceStatisticsTable");
            if(!table)return;
            dialogOpened=true;
            hasChart=table->rowCount()>0 &&
                table->findChild<QProgressBar*>("gcBar")!=nullptr;
            if(auto *dialog=qobject_cast<QDialog*>(table->window()))dialog->reject();
        });
        stats->trigger();
        QVERIFY(dialogOpened);
        QVERIFY(hasChart);
    }
    void sourceDiscoveryHonorsExistingCheckout() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath(".git")));
        QVERIFY(QDir().mkpath(temp.filePath("scripts")));
        for(const auto &fileName:QStringList{
            "CMakeLists.txt","scripts/build_ubuntu_deb.sh"}) {
            QFile file(temp.filePath(fileName));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("# offline source discovery test\n");
            file.close();
        }
        QSettings settings("HelixEdit","HelixEdit");
        const QVariant before=settings.value("updater/source");
        settings.setValue("updater/source",temp.path());
        QCOMPARE(discoverSourceFolder(),QDir(temp.path()).canonicalPath());
        if(before.isValid())settings.setValue("updater/source",before);
        else settings.remove("updater/source");
    }
    void v020ActivityRailAndContextViews() {
        Editor editor(true);
        auto *rail=editor.findChild<QFrame*>("activityRail");
        auto *tools=editor.findChild<QFrame*>("workspaceToolPanel");
        auto *toolStack=editor.findChild<QStackedWidget*>("workspaceToolStack");
        auto *studio=editor.findChild<QTabWidget*>("studioTabs");
        auto *ribbon=editor.findChild<QTabWidget*>("commandRibbon");
        auto *sequence=editor.findChild<QToolButton*>("activitySequence");
        auto *plasmid=editor.findChild<QToolButton*>("activityPlasmid");
        auto *analysis=editor.findChild<QToolButton*>("activityAnalysis");
        auto *project=editor.findChild<QToolButton*>("toggleProjectExplorer");
        auto *collapse=editor.findChild<QToolButton*>("toggleWorkspaceTools");
        QVERIFY(rail);QVERIFY(tools);QVERIFY(toolStack);QVERIFY(studio);
        QVERIFY(ribbon);QVERIFY(sequence);QVERIFY(plasmid);QVERIFY(analysis);
        QVERIFY(project);QVERIFY(collapse);
        QCOMPARE(toolStack->count(),3);
        QCOMPARE(studio->count(),7);
        QVERIFY(studio->tabBar()->isHidden());
        QVERIFY(ribbon->isHidden());
        QCOMPARE(studio->currentIndex(),0);
        QVERIFY(plasmid->isChecked());
        QCOMPARE(toolStack->currentIndex(),1);
        sequence->click();
        QCOMPARE(studio->currentIndex(),1);
        QCOMPARE(toolStack->currentIndex(),0);
        QVERIFY(sequence->isChecked());
        auto *sequenceView=editor.findChild<QToolButton*>("viewSequence");
        QVERIFY(sequenceView);
        auto *enzymeView=editor.findChild<QToolButton*>("viewEnzymes");
        auto *featureView=editor.findChild<QToolButton*>("viewFeatures");
        auto *primerView=editor.findChild<QToolButton*>("viewPrimers");
        QVERIFY(enzymeView);QVERIFY(featureView);QVERIFY(primerView);
        plasmid->click();
        QCOMPARE(studio->currentIndex(),0);
        enzymeView->click();
        QCOMPARE(studio->currentIndex(),2);
        QVERIFY(plasmid->isChecked());
        featureView->click();
        QCOMPARE(studio->currentIndex(),3);
        primerView->click();
        QCOMPARE(studio->currentIndex(),4);
        analysis->click();
        QCOMPARE(studio->currentIndex(),6);
        QCOMPARE(toolStack->currentIndex(),2);
        auto *historyView=editor.findChild<QToolButton*>("viewHistory");
        QVERIFY(historyView);
        historyView->click();
        QCOMPARE(studio->currentIndex(),5);
        const auto commands=editor.findChildren<QToolButton*>("contextActionAnalysis");
        QVERIFY2(commands.size()>=6,"Analysis tools must remain accessible on the left");
        collapse->setChecked(false);
        QVERIFY(tools->isHidden());
        collapse->setChecked(true);
        QVERIFY(!tools->isHidden());
        project->setChecked(true);
        QVERIFY(project->isChecked());
        const QString light=themeStyle("Light");
        const QString dark=themeStyle("Dark");
        const QString dracula=themeStyle("Dracula");
        for(const QString &stylesheet:{light,dark,dracula}) {
            QVERIFY(stylesheet.contains("QFrame#activityRail"));
            QVERIFY(stylesheet.contains("workspaceToolPanel"));
            QVERIFY(stylesheet.contains("activitySequence:checked"));
            QVERIFY(stylesheet.contains("activityAnalysis:checked"));
        }
    }
    void compactToolbarAndUnobtrusiveNotices() {
        Editor editor(true);
        auto *quick=editor.findChild<QToolBar*>("quickActions");
        auto *ribbon=editor.findChild<QTabWidget*>("commandRibbon");
        auto *search=editor.findChild<QLineEdit*>("quickSearch");
        auto *molecule=editor.findChild<QComboBox*>("compactMoleculeSelector");
        auto *find=editor.findChild<QPushButton*>("quickFind");
        auto *notices=editor.findChild<QLabel*>("warningStatusIndicator");
        auto *offline=editor.findChild<QLabel*>("offlineStatus");
        QVERIFY(quick);QVERIFY(ribbon);QVERIFY(search);
        QVERIFY(molecule);QVERIFY(find);QVERIFY(notices);QVERIFY(offline);
        QCOMPARE(quick->toolButtonStyle(),Qt::ToolButtonIconOnly);
        QVERIFY(ribbon->maximumHeight()<=55);
        QCOMPARE(notices->parentWidget(),static_cast<QWidget*>(editor.statusBar()));
        QCOMPARE(offline->parentWidget(),static_cast<QWidget*>(editor.statusBar()));
        QVERIFY(!editor.centralWidget()->findChild<QLabel*>("warningStatusIndicator"));
        QCOMPARE(quick->findChild<QLineEdit*>("quickSearch"),search);
        QCOMPARE(quick->findChild<QComboBox*>("compactMoleculeSelector"),molecule);
    }
    void settingsCentralizePreferencesAndAboutCredits() {
        Editor editor(true);
        auto settingsAction=editor.findChild<QAction*>("openSettings");
        QVERIFY(settingsAction);
        QVERIFY(editor.menuBar());
        bool foundSettingsMenu=false,foundAppearanceMenu=false;
        for(const auto *action:editor.menuBar()->actions()){
            if(action->text()=="&Settings")foundSettingsMenu=true;
            if(action->text()=="&Appearance")foundAppearanceMenu=true;
        }
        QVERIFY(foundSettingsMenu);
        QVERIFY(!foundAppearanceMenu);
        QString savedPrevious=QSettings("HelixEdit","HelixEdit")
            .value("appearance/theme","Dark").toString();
        bool appeared=false, controlsReady=false;
        QTimer::singleShot(0,&editor,[&]{
            auto *dialog=editor.findChild<QDialog*>("helixSettingsDialog");
            if(!dialog)return;
            appeared=true;
            auto *pages=dialog->findChild<QTabWidget*>("preferencesTabs");
            auto *theme=dialog->findChild<QComboBox*>("themePreference");
            auto *source=dialog->findChild<QLineEdit*>("updateSourcePreference");
            auto *autosave=dialog->findChild<QComboBox*>("autosavePreference");
            auto *enzyme=dialog->findChild<QCheckBox*>("enzymeLabelsPreference");
            controlsReady=pages && pages->count()==4 && theme && source && autosave && enzyme;
            if(controlsReady) {
                // CI must not depend on paths saved in an earlier test session.
                source->clear();
                for(auto *input:dialog->findChildren<QLineEdit*>())
                    if(input->objectName().startsWith("executable_"))input->clear();
                theme->setCurrentText("Dracula");
                auto *buttonBox=dialog->findChild<QDialogButtonBox*>();
                if(buttonBox && buttonBox->button(QDialogButtonBox::Save))
                    buttonBox->button(QDialogButtonBox::Save)->click();
                else dialog->reject();
            } else dialog->reject();
        });
        settingsAction->trigger();
        QVERIFY(appeared);
        QVERIFY(controlsReady);
        QCOMPARE(savedTheme(),QString("Dracula"));
        applyTheme(savedPrevious);
        bool aboutVisible=false,hasCredit=false,hasRepo=false,hasEmail=false;
        bool hasAppIcon=false,hasGradientHero=false,linkCopied=false;
        // The Help action owns the submenu; use its exact text rather than traversal order.
        QAction *aboutAction=nullptr;
        for(auto *menuAction:editor.menuBar()->actions()){
            if(!menuAction->menu() || menuAction->text()!="&Help")continue;
            for(auto *action:menuAction->menu()->actions())
                if(action->text()=="About HelixEdit")aboutAction=action;
        }
        QVERIFY(aboutAction);
        QTimer::singleShot(0,&editor,[&]{
            auto *dialog=editor.findChild<QDialog*>("helixAboutDialog");
            if(!dialog)return;
            aboutVisible=true;
            QString visible;
            for(const auto *label:dialog->findChildren<QLabel*>())visible+=label->text();
            hasCredit=visible.contains("Developed and edited by Wael H");
            hasRepo=visible.contains("https://github.com/mac860/HelixEdit");
            hasEmail=visible.contains("waelye@gmail.com");
            auto *icon=dialog->findChild<QLabel*>("aboutAppIcon");
            hasAppIcon=icon && !icon->pixmap().isNull();
            hasGradientHero=dialog->findChild<QFrame*>("aboutHero")!=nullptr;
            auto *copy=dialog->findChild<QPushButton*>("aboutCopyLink");
            if(copy){
                copy->click();
                linkCopied=QGuiApplication::clipboard()->text()==
                    "https://github.com/mac860/HelixEdit";
            }
            dialog->reject();
        });
        aboutAction->trigger();
        QVERIFY(aboutVisible);
        QVERIFY(hasCredit);
        QVERIFY(hasRepo);
        QVERIFY(hasEmail);
        QVERIFY(hasAppIcon);
        QVERIFY(hasGradientHero);
        QVERIFY(linkCopied);
    }
    void updaterEntryIsAvailable() {
        Editor editor(true);
        bool found=false;
        for(const auto *action:editor.findChildren<QAction*>())
            found |= action->text().contains("update from GitHub source",Qt::CaseInsensitive);
        QVERIFY(found);
    }
    void pasteFastaSelectsNewSequenceForMap() {
        Editor editor(true);
        auto *map=editor.findChild<MapView*>("plasmidMap");
        QVERIFY(map);
        const QString clipboardBefore=QGuiApplication::clipboard()->text();
        QGuiApplication::clipboard()->setText(">Addgene_NGS_Result\nAAAGAATTCTTT\n");
        // Toolbar shortens the shared QAction label to "Paste FASTA".
        // Locate the action by its stable id, not a mutable visible label.
        QAction *paste=editor.findChild<QAction*>("pasteFastaAction");
        QVERIFY(paste);
        const int oldCount=editor.model->rowCount();
        paste->trigger();
        QCOMPARE(editor.model->rowCount(),oldCount+1);
        QCOMPARE(editor.table->currentIndex().row(),oldCount);
        QCOMPARE(map->record.name,QString("Addgene_NGS_Result"));
        QCOMPARE(map->record.sequence,QString("AAAGAATTCTTT"));
        QVERIFY(!map->record.circular);
        QVERIFY(map->record.features.isEmpty());
        QGuiApplication::clipboard()->setText(clipboardBefore);
    }
    void circularMapTracksActiveSequenceAndTopology() {
        Editor editor(true);
        const auto d=parseFasta(">Addgene_NGS_Result\nAAAGAATTCTTT\n");
        editor.setDocument(d);
        auto *map=editor.findChild<MapView*>("plasmidMap");
        auto *topology=editor.findChild<QCheckBox*>("confirmedCircularTopology");
        auto *unique=editor.findChild<QCheckBox*>("uniqueCuttersFilter");
        QVERIFY(map);
        QVERIFY(topology);
        QVERIFY(unique);
        QVERIFY(!topology->isChecked());
        QVERIFY(map->record.features.isEmpty());
        topology->click();
        QVERIFY(topology->isChecked());
        QVERIFY(map->record.circular);
        auto all=findRestrictionSites(map->record);
        QVERIFY(std::any_of(all.begin(),all.end(),[](const RestrictionSite &site){
            return site.enzyme=="EcoRI" && site.start==3;
        }));
        auto render=[&]() {
            QImage image(900,640,QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            map->draw(painter,QRectF(0,0,900,640));
            return image;
        };
        const auto before=render();
        editor.editResidue(0,3,'T'); // destroys GAATTC
        auto changed=findRestrictionSites(map->record);
        QVERIFY(std::none_of(changed.begin(),changed.end(),[](const RestrictionSite &site){
            return site.enzyme=="EcoRI";
        }));
        const auto after=render();
        QVERIFY2(!(before==after),"Circular map must redraw site labels after base edits.");
        QVERIFY(map->record.features.isEmpty());
    }
    void wrapToWindowReflowsAndPreservesEditableSequence() {
        Editor e(true);
        auto view=e.findChild<WrappedSequenceView*>("wrappedSequenceView");
        auto wrap=e.findChild<QCheckBox*>("wrapToWindow");
        auto complement=e.findChild<QCheckBox*>("showComplement");
        auto featureTracks=e.findChild<QCheckBox*>("showWrappedFeatures");
        auto stack=e.findChild<QStackedWidget*>("sequenceViewStack");
        auto menuWrap=e.findChild<QAction*>("menuWrapToWindow");
        QVERIFY(view);QVERIFY(wrap);QVERIFY(complement);QVERIFY(featureTracks);QVERIFY(stack);
        QVERIFY(menuWrap);
        const QVariant previousWrap=QSettings("HelixEdit","HelixEdit").value("sequence/wrapToWindow");
        auto doc=parseFasta(">plasmid\nACGTRYSWKMBDHVN-ATGCTGAACCTG\n");
        doc.records[0].features.append(
            Feature{"test_feature","misc_feature",4,24,false});
        e.setDocument(doc);
        const QString original=e.model->document.records[0].sequence;
        auto tabs=e.findChild<QTabWidget*>("studioTabs");
        QVERIFY(tabs);
        tabs->setCurrentIndex(1);
        wrap->setChecked(true);
        QCOMPARE(stack->currentIndex(),1);
        QVERIFY(menuWrap->isChecked());
        WrappedSequenceView isolated;
        isolated.resize(850,390);
        isolated.setRecord(doc.records[0],"DNA");
        isolated.show();
        QCoreApplication::processEvents();
        const int wide=isolated.basesPerLine();
        isolated.resize(350,390);
        QCoreApplication::processEvents();
        const int narrow=isolated.basesPerLine();
        QVERIFY2(wide>narrow,"A narrower viewport must wrap into fewer bases per line");
        QVERIFY(narrow>=10);
        complement->setChecked(false);
        QVERIFY(!view->showComplement());
        complement->setChecked(true);
        QVERIFY(view->showComplement());
        featureTracks->setChecked(false);
        QVERIFY(!view->showFeatures());
        featureTracks->setChecked(true);
        QVERIFY(view->showFeatures());
        const int hit=isolated.alignmentColumnAt(QPoint(65,56));
        QVERIFY(hit>=0);
        QVERIFY(hit<original.size());
        isolated.setSelectedColumn(hit);
        QCOMPARE(isolated.selectedColumn(),hit);
        wrap->setChecked(false);
        QCOMPARE(stack->currentIndex(),0);
        QCOMPARE(e.model->document.records[0].sequence,original);
        QCOMPARE(stack->currentWidget(),static_cast<QWidget*>(e.table));
        QVERIFY(!menuWrap->isChecked());
        menuWrap->setChecked(true);
        QCOMPARE(stack->currentIndex(),1);
        QSettings settings("HelixEdit","HelixEdit");
        if(previousWrap.isValid())settings.setValue("sequence/wrapToWindow",previousWrap);
        else settings.remove("sequence/wrapToWindow");
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
