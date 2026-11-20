#pragma once
#include <QString>
#include <QSettings>
#include <QApplication>
namespace helix {
inline QString themeStyle(const QString &name) {
 const bool dark=name=="Dark" || name=="Dracula";
 const QString bg=dark?(name=="Dracula"?"#282a36":"#171d2a"):"#f5f3fa";
 const QString panel=dark?(name=="Dracula"?"#343746":"#222b3a"):"#ffffff";
 const QString text=dark?"#f8f8f2":"#29283c";
 const QString muted=dark?"#b8bbca":"#706480";
 const QString accent=name=="Dracula"?"#bd93f9":(dark?"#66d9c0":"#7c5ce7");
 const QString border=dark?"#4a5060":"#e6dfef";
 const QString hover=dark?"#454b5d":"#dfd5f5";
 return QString(R"(
 QWidget {font-family:"Inter","Segoe UI","DejaVu Sans";font-size:12px;color:%3;background:%1;}
 QMainWindow,QDialog {background:%1;color:%3;}
 QToolBar,QMenuBar,QMenu,QStatusBar {background:%2;color:%3;border:0;}
 QToolBar {border-bottom:1px solid %6;spacing:7px;padding:9px;}
 QToolButton,QPushButton {background:%2;border:1px solid %6;border-radius:7px;padding:7px 11px;color:%3;}
 QToolButton:hover,QPushButton:hover {background:%7;}
 QToolButton:disabled,QPushButton:disabled {color:%4;}
 QLineEdit,QComboBox,QPlainTextEdit,QListWidget {background:%2;color:%3;border:1px solid %6;border-radius:7px;padding:6px;selection-background-color:%5;}
 QTabWidget::pane {border:1px solid %6;border-radius:10px;background:%2;}
 QTabBar::tab {padding:10px 17px;color:%4;background:%1;}
 QTabBar::tab:selected {color:%5;border-bottom:3px solid %5;}
 QHeaderView::section {background:%2;border:1px solid %6;padding:5px;color:%4;}
 QTableView {background:%2;color:%3;border:0;gridline-color:%6;selection-background-color:%5;selection-color:#ffffff;}
 QGroupBox {background:%2;border:1px solid %6;border-radius:9px;margin-top:10px;padding:12px;}
 QGroupBox::title {subcontrol-origin:margin;left:12px;padding:0 5px;color:%4;}
 QSplitter::handle {background:%6;}
 QMenu::item:selected {background:%5;color:#ffffff;}
 )").arg(bg,panel,text,muted,accent,border,hover);
}
inline void applyTheme(const QString &name) {
 qApp->setStyleSheet(themeStyle(name));
 QSettings settings("HelixEdit","HelixEdit");
 settings.setValue("appearance/theme",name);
}
inline QString savedTheme() {
 const QString name=QSettings("HelixEdit","HelixEdit").value("appearance/theme","Light").toString();
 return (name=="Light"||name=="Dark"||name=="Dracula")?name:"Light";
}
}
