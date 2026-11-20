#include "editor.h"
#include <QtSvg/QSvgRenderer>
#include <QPainter>
int main(int argc,char **argv){
    QApplication app(argc,argv);QCoreApplication::setOrganizationName("HelixEdit");QCoreApplication::setApplicationName("HelixEdit");QCoreApplication::setApplicationVersion("0.1.9");
    // Match GNOME/Wayland desktop launcher and draw SVG into a guaranteed native icon.
    QGuiApplication::setDesktopFileName("org.helixedit");
    QPixmap appLogo(128,128);appLogo.fill(Qt::transparent);
    QSvgRenderer logoRenderer(QStringLiteral(":/packaging/helixedit.svg"));
    if(logoRenderer.isValid()){
        QPainter logoPainter(&appLogo);
        logoRenderer.render(&logoPainter);
    }
    QApplication::setWindowIcon(logoRenderer.isValid()?QIcon(appLogo):QIcon(":/packaging/helixedit.svg"));
    QCommandLineParser parser;parser.setApplicationDescription("Compact, offline sequence studio");parser.addHelpOption();parser.addVersionOption();
    parser.addPositionalArgument("file","FASTA, CLUSTAL, GenBank or .helix project");
    parser.addOption({"screenshot","Render the synthetic demo offscreen to a PNG and exit.","path"});
    parser.addOption({"map-preview","Show map tab in screenshot mode."});parser.process(app);
    const bool preview=parser.isSet("screenshot");
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QLockFile lock(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/editor.lock");
    if(!preview&&!lock.tryLock(100)){QMessageBox::information(nullptr,"HelixEdit","Another HelixEdit instance is using this profile.");return 1;}
    helix::Editor editor(preview);
    if(!parser.positionalArguments().isEmpty())try{editor.loadPath(parser.positionalArguments().first());}catch(const std::exception &e){QMessageBox::warning(nullptr,"Cannot open file",e.what());}
    editor.show();
    if(preview)QTimer::singleShot(200,[&]{if(auto studio=editor.findChild<QTabWidget*>("studioTabs"))studio->setCurrentIndex(parser.isSet("map-preview")?0:1);app.processEvents();bool ok=editor.grab().save(parser.value("screenshot"));app.exit(ok?0:2);});
    return app.exec();
}
