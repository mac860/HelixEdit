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
    return QString(R"(
      QWidget {font-family:"Inter","Noto Sans","Segoe UI","DejaVu Sans";font-size:12px;color:%3;background:%1;}
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
