#pragma once
#include <QString>
#include <QSettings>
#include <QApplication>

namespace helix {
// HelixEdit's own compact visual language. No third-party application assets.
inline bool isDarkTheme(const QString &name) {
    return name=="Dark" || name=="Dracula";
}
inline QString themeStyle(const QString &name) {
    const bool dark=isDarkTheme(name);
    const QString bg=name=="Dracula"?"#282a36":(dark?"#151922":"#f4f7fb");
    const QString panel=name=="Dracula"?"#333545":(dark?"#202735":"#ffffff");
    const QString text=dark?"#f4f5f9":"#253145";
    const QString muted=dark?"#acb3c6":"#607083";
    const QString accent=name=="Dracula"?"#bd93f9":(dark?"#50cbb1":"#356fe4");
    const QString border=dark?"#40495a":"#d9e1ed";
    const QString hover=dark?"#3b4559":"#eaf1ff";
    QString base=QString(R"(
      QWidget {font-family:"Inter","Noto Sans","Segoe UI","DejaVu Sans";font-size:12px;color:%3;background:%1;}
      QLabel,QCheckBox,QRadioButton {background:transparent;border:0;}
      QMainWindow,QDialog {background:%1;color:%3;}
      QMenuBar,QMenu,QStatusBar,QToolBar {background:%2;color:%3;border:0;}
      QMenuBar::item {padding:4px 9px;}
      QMenuBar::item:selected,QMenu::item:selected {background:%7;color:%3;border-radius:4px;}
      QToolBar {border-bottom:1px solid %6;spacing:3px;padding:3px 6px;}
      QToolBar QToolButton {padding:4px 7px;min-height:22px;}
      QToolButton,QPushButton {background:%2;border:1px solid %6;border-radius:5px;padding:5px 9px;color:%3;}
      QToolButton:hover,QPushButton:hover {background:%7;border-color:%5;}
      QToolButton:checked,QPushButton:checked {background:%7;border-color:%5;color:%5;}
      QToolButton:disabled,QPushButton:disabled {color:%4;}
      QLineEdit,QComboBox,QPlainTextEdit,QListWidget,QTreeWidget {
        background:%2;color:%3;border:1px solid %6;border-radius:5px;padding:4px;
        selection-background-color:%5;selection-color:#ffffff;}
      QLineEdit:focus,QComboBox:focus,QPlainTextEdit:focus,QListWidget:focus,QTreeWidget:focus {
        border:1px solid %5;}
      QComboBox::drop-down {border:0;width:18px;}
      QTabWidget::pane {border:1px solid %6;border-radius:6px;background:%2;}
      QTabBar::tab {padding:7px 13px;margin-right:2px;color:%4;background:%1;border-bottom:2px solid transparent;}
      QTabBar::tab:selected {color:%5;background:%2;border-bottom:2px solid %5;font-weight:600;}
      QTabBar::tab:hover {background:%7;}
      QHeaderView::section {background:%2;border:0;border-right:1px solid %6;border-bottom:1px solid %6;padding:4px;color:%4;}
      QTableView {background:%2;color:%3;border:0;gridline-color:%6;selection-background-color:%5;selection-color:#ffffff;}
      QGroupBox {background:%2;border:1px solid %6;border-radius:6px;margin-top:10px;padding:9px 6px 6px;}
      QGroupBox::title {subcontrol-origin:margin;left:8px;padding:0 4px;color:%4;font-weight:600;}
      QSplitter::handle {background:%6;}
      QWidget#projectSidebar {background:%2;border-right:1px solid %6;}
      QWidget#projectSidebar QLabel#sidebarHeading {font-weight:700;color:%5;}
      QListWidget#sequenceNavigator::item {padding:4px 6px;border-bottom:1px solid %6;}
      QListWidget#sequenceNavigator::item:selected {background:%5;color:#ffffff;border-radius:4px;}
      QListWidget#featureNavigator::item {padding:3px 5px;border-bottom:1px solid %6;}
      QListWidget#featureNavigator::item:selected {background:%7;border-radius:4px;}
      QWidget#mapToolStrip {background:%2;}
      QTabWidget#commandRibbon::pane {border:0;background:%2;}
      QTabWidget#commandRibbon QTabBar::tab {padding:4px 11px;}
      QTabWidget#commandRibbon QToolButton {padding:4px 7px;margin:0 1px;border-radius:4px;}
      QScrollBar:vertical {background:%1;width:10px;margin:0;}
      QScrollBar::handle:vertical {background:%6;min-height:26px;border-radius:4px;}
      QScrollBar:horizontal {background:%1;height:10px;margin:0;}
      QScrollBar::handle:horizontal {background:%6;min-width:26px;border-radius:4px;}
      QScrollBar::add-line,QScrollBar::sub-line {width:0;height:0;}
      QToolTip {background:%2;color:%3;border:1px solid %5;padding:4px;}
    )").arg(bg,panel,text,muted,accent,border,hover);
    if(dark)return base;
    // Glass-inspired appearance uses alpha-blended panels and layered gradients.
    // Native desktop background blur is not provided by Qt Widgets stylesheets.
    return base+QStringLiteral(R"(
      QMainWindow {
        background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
            stop:0 #e9f4ff,stop:0.35 #faf8ff,stop:0.75 #f1faff,stop:1 #f9edff);
      }
      QMainWindow > QWidget {
        background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
            stop:0 #edf7ff,stop:0.48 #fcfbff,stop:1 #edf5ff);
      }
      QMenuBar,QToolBar {
        background:qlineargradient(x1:0,y1:0,x2:0,y2:1,
            stop:0 rgba(255,255,255,250),stop:1 rgba(242,247,255,233));
        color:#26364d;
        border-bottom:1px solid #cbdcf2;
      }
      QStatusBar {background:rgba(249,252,255,244);border-top:1px solid #d7e4f5;}
      QWidget#projectSidebar {
        background:rgba(255,255,255,228);
        border:1px solid rgba(151,177,227,160);
        border-radius:15px;
      }
      QWidget#projectSidebar QLabel#sidebarHeading {color:#5c4ace;}
      QTabWidget#studioTabs::pane {
        background:rgba(255,255,255,234);
        border:1px solid #cfdaf1;
        border-radius:15px;
      }
      QTabWidget#commandRibbon::pane {
        background:rgba(255,255,255,193);
        border:1px solid rgba(138,175,227,140);
        border-radius:12px;
      }
      QTabWidget#studioTabs QTabBar::tab {
        background:rgba(235,244,255,200);
        color:#526781;
        padding:8px 13px;
        border-top-left-radius:9px;
        border-top-right-radius:9px;
      }
      QTabWidget#studioTabs QTabBar::tab:selected {
        background:rgba(255,255,255,250);
        color:#4d49bf;
        border-bottom:3px solid #55beca;
      }
      QTabWidget#commandRibbon QTabBar::tab:selected {
        background:rgba(101,104,234,40);
        color:#4d49c4;
        border-bottom:2px solid #7467ec;
      }
      QGroupBox {
        background:rgba(255,255,255,227);
        border:1px solid rgba(163,187,224,175);
        border-radius:13px;
        margin-top:11px;
        padding:12px 8px 9px;
      }
      QGroupBox::title {color:#485f83;font-weight:700;}
      QWidget#mapToolStrip {
        background:rgba(237,247,255,218);
        border:1px solid #d3e4f7;
        border-radius:11px;
      }
      QToolButton,QPushButton {
        background:qlineargradient(x1:0,y1:0,x2:0,y2:1,
            stop:0 #ffffff,stop:1 #eff5ff);
        border:1px solid #ccd9ee;
        border-radius:9px;
        color:#30486b;
        padding:6px 10px;
      }
      QToolButton:hover,QPushButton:hover {
        background:qlineargradient(x1:0,y1:0,x2:0,y2:1,
            stop:0 #ecfbff,stop:1 #ececff);
        border-color:#6b8ce6;
        color:#4943bf;
      }
      QToolButton:checked,QPushButton:checked {
        background:rgba(94,218,201,68);
        border-color:#42bba8;
        color:#155a66;
      }
      QLineEdit,QComboBox,QPlainTextEdit,QListWidget,QTreeWidget,QTableView {
        background:rgba(255,255,255,243);
        border:1px solid #cfdcee;
        border-radius:9px;
        selection-background-color:#6478ec;
        selection-color:#ffffff;
      }
      QHeaderView::section {
        background:rgba(225,241,255,237);
        color:#465d80;
        border-bottom:1px solid #cbdbee;
      }
      QToolTip {
        background:#ffffff;color:#233755;
        border:1px solid #93b5ef;border-radius:8px;
      }
      QMenu {background:#ffffff;border:1px solid #d5e0f2;border-radius:9px;}
      QMenu::item:selected {background:#e6f1ff;color:#3941a5;}
    )");
}
inline void applyTheme(const QString &name) {
    qApp->setStyleSheet(themeStyle(name));
    QSettings settings("HelixEdit","HelixEdit");
    settings.setValue("appearance/theme",name);
}
inline QString savedTheme() {
    const QString name=QSettings("HelixEdit","HelixEdit").value("appearance/theme","Dark").toString();
    return (name=="Light"||name=="Dark"||name=="Dracula")?name:"Dark";
}
}
