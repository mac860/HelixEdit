#pragma once
#include <QtWidgets>
#include <QDesktopServices>
#include <QUrl>
#include <QtSvg/QSvgRenderer>
#include <QPainter>

namespace helix {
// A standalone, independent project About view using the same native
// HelixEdit SVG icon already installed for the window and Ubuntu launcher.
inline void showAboutDialog(QWidget *parent) {
    QDialog dialog(parent);
    dialog.setObjectName("helixAboutDialog");
    dialog.setWindowTitle("About HelixEdit");
    QIcon appIcon=qApp->windowIcon();
    if(appIcon.isNull()) {
        QPixmap rendered(96,96);
        rendered.fill(Qt::transparent);
        QSvgRenderer svg(QStringLiteral(":/packaging/helixedit.svg"));
        if(svg.isValid()){
            QPainter painter(&rendered);
            svg.render(&painter);
            appIcon=QIcon(rendered);
        }
    }
    dialog.setWindowIcon(appIcon);
    dialog.setMinimumSize(525,390);
    dialog.resize(555,415);
    auto *layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(16,16,16,13);
    layout->setSpacing(10);

    const bool dark=qApp->property("helix-theme").toString()!="Light";
    auto *hero=new QFrame(&dialog);
    hero->setObjectName("aboutHero");
    hero->setStyleSheet(dark
      ? "QFrame#aboutHero{border:1px solid #5f547c;border-radius:16px;"
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #393350,stop:0.5 #28394d,stop:1 #234b55);}"
      : "QFrame#aboutHero{border:1px solid #ccdcfa;border-radius:16px;"
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f2eaff,stop:0.48 #eaf5ff,stop:1 #dcfbf2);}");
    auto *heroRow=new QHBoxLayout(hero);
    heroRow->setContentsMargins(15,13,15,13);
    heroRow->setSpacing(14);
    auto *icon=new QLabel(hero);
    icon->setObjectName("aboutAppIcon");
    icon->setFixedSize(78,78);
    icon->setPixmap(appIcon.pixmap(QSize(72,72)));
    icon->setAlignment(Qt::AlignCenter);
    heroRow->addWidget(icon);
    auto *headColumn=new QVBoxLayout;
    headColumn->setSpacing(4);
    auto *title=new QLabel("HelixEdit",hero);
    title->setObjectName("aboutName");
    QFont nameFont=title->font();nameFont.setPointSize(22);nameFont.setBold(true);
    title->setFont(nameFont);
    headColumn->addWidget(title);
    auto *subtitle=new QLabel("DNA  ·  RNA  ·  Protein  ·  Plasmids",hero);
    subtitle->setStyleSheet(dark?"color:#97e5dc;background:transparent;":"color:#3a728b;background:transparent;");
    headColumn->addWidget(subtitle);
    auto *version=new QLabel("VERSION  "+QCoreApplication::applicationVersion()+"   •   BETA",hero);
    version->setObjectName("aboutVersion");
    version->setStyleSheet(dark?"color:#cdc7fd;background:transparent;":"color:#665ac8;background:transparent;");
    headColumn->addWidget(version);
    headColumn->addStretch();
    heroRow->addLayout(headColumn,1);
    layout->addWidget(hero);

    auto *profile=new QFrame(&dialog);
    profile->setObjectName("aboutProfile");
    profile->setFrameShape(QFrame::StyledPanel);
    auto *profileLayout=new QVBoxLayout(profile);
    profileLayout->setContentsMargins(15,12,15,11);
    profileLayout->setSpacing(7);
    auto *credit=new QLabel("<b>Developed and edited by Wael H</b>",profile);
    credit->setObjectName("aboutDeveloper");
    profileLayout->addWidget(credit);
    auto *aboutText=new QLabel(
        "A lightweight, local-first scientific sequence workspace for "
        "exploration, editing, annotation and reproducible analyses.",profile);
    aboutText->setWordWrap(true);
    profileLayout->addWidget(aboutText);
    auto *contacts=new QLabel(
        "<a href='mailto:waelye@gmail.com'>waelye@gmail.com</a>"
        "  ·  <a href='https://github.com/mac860/HelixEdit'>"
        "https://github.com/mac860/HelixEdit</a>",profile);
    contacts->setObjectName("aboutLinks");
    contacts->setTextFormat(Qt::RichText);
    contacts->setTextInteractionFlags(Qt::TextBrowserInteraction);
    contacts->setOpenExternalLinks(true);
    contacts->setWordWrap(true);
    profileLayout->addWidget(contacts);
    layout->addWidget(profile);

    auto *independent=new QLabel(
        "Independent open-source beta. Not affiliated with SnapGene or BioEdit. "
        "Validate sequence annotations and computational results before research use. "
        "Licensing and build instructions are available in the repository.",&dialog);
    independent->setObjectName("aboutDisclaimer");
    independent->setWordWrap(true);
    independent->setStyleSheet(dark?"color:#b8c2d2;":"color:#5d7089;");
    layout->addWidget(independent);
    layout->addStretch();

    auto *footer=new QHBoxLayout;
    footer->setSpacing(7);
    auto *source=new QPushButton("Open GitHub", &dialog);
    source->setObjectName("aboutOpenGitHub");
    source->setIcon(dialog.style()->standardIcon(QStyle::SP_DirLinkIcon));
    QObject::connect(source,&QPushButton::clicked,&dialog,[]{
        QDesktopServices::openUrl(QUrl("https://github.com/mac860/HelixEdit"));
    });
    footer->addWidget(source);
    auto *copy=new QPushButton("Copy repository link",&dialog);
    copy->setObjectName("aboutCopyLink");
    QObject::connect(copy,&QPushButton::clicked,&dialog,[]{
        QGuiApplication::clipboard()->setText("https://github.com/mac860/HelixEdit");
    });
    footer->addWidget(copy);
    footer->addStretch();
    auto *close=new QPushButton("Close",&dialog);
    close->setObjectName("aboutClose");
    close->setDefault(true);
    QObject::connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
    footer->addWidget(close);
    layout->addLayout(footer);
    dialog.exec();
}
} // namespace helix
