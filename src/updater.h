#pragma once
#include <QtWidgets>
#include <QProcess>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

namespace helix {
// Explicit, user-initiated source updater. No periodic network traffic or
// unchecked shell execution: all program/arguments are passed to QProcess.
inline void openSourceUpdater(QWidget *parent) {
#ifndef Q_OS_LINUX
    QMessageBox::information(parent,"Update HelixEdit",
        "The source-to-DEB updater is currently available on Linux. "
        "On macOS, update your checked-out source and rebuild using the README instructions.");
#else
    QDialog dialog(parent);
    dialog.setWindowTitle("HelixEdit · Update from GitHub source");
    dialog.resize(745,535);
    auto layout=new QVBoxLayout(&dialog);
    auto intro=new QLabel(
        "<b>Update HelixEdit without downloading the repository again</b><br>"
        "Checks the official GitHub <b>main</b> branch. When you approve, "
        "it fast-forwards a clean local checkout, compiles and runs tests, "
        "creates the native DEB, then requests Ubuntu authentication for installation.");
    intro->setWordWrap(true);layout->addWidget(intro);
    auto sourceRow=new QHBoxLayout;
    auto sourcePath=new QLineEdit;
    sourcePath->setObjectName("updateSourcePath");
    sourcePath->setPlaceholderText("Existing Git checkout (contains CMakeLists.txt and .git)");
    QSettings settings("HelixEdit","HelixEdit");
    sourcePath->setText(settings.value("updater/source").toString());
    auto browse=new QPushButton("Browse…");
    sourceRow->addWidget(sourcePath,1);sourceRow->addWidget(browse);
    layout->addWidget(new QLabel("Existing source folder"));
    layout->addLayout(sourceRow);
    auto info=new QLabel(
        "Safety: the updater accepts only the official repository origin and a clean "
        "<b>main</b> branch. It never resets local changes, and does not build with root "
        "privileges. Authentication is requested only for installing the resulting package.");
    info->setWordWrap(true);layout->addWidget(info);
    auto progress=new QProgressBar;progress->setRange(0,0);
    progress->setTextVisible(false);progress->setVisible(false);
    layout->addWidget(progress);
    auto console=new QPlainTextEdit;console->setObjectName("updateLog");
    console->setReadOnly(true);
    console->setPlaceholderText("Update checks, build, test results and installation details will appear here.");
    layout->addWidget(console,1);
    auto commands=new QHBoxLayout;
    auto check=new QPushButton("Check GitHub");
    auto apply=new QPushButton("Update, test & install");
    auto close=new QPushButton("Close");
    apply->setDefault(true);
    commands->addWidget(check);commands->addWidget(apply);
    commands->addStretch();commands->addWidget(close);
    layout->addLayout(commands);

    QProcess process(&dialog);
    process.setProcessChannelMode(QProcess::MergedChannels);
    auto append=[console](const QString &message){
        if(message.isEmpty())return;
        console->moveCursor(QTextCursor::End);
        console->insertPlainText(message);
        console->moveCursor(QTextCursor::End);
    };
    connect(browse,&QPushButton::clicked,&dialog,[&]{
        const auto folder=QFileDialog::getExistingDirectory(&dialog,
            "Select existing HelixEdit Git checkout",sourcePath->text());
        if(!folder.isEmpty())sourcePath->setText(folder);
    });
    connect(close,&QPushButton::clicked,&dialog,&QDialog::reject);
    connect(&process,&QProcess::readyRead,&dialog,[&]{
        append(QString::fromUtf8(process.readAll()));
    });
    connect(&process,&QProcess::errorOccurred,&dialog,[&](QProcess::ProcessError){
        append("\nProcess error: "+process.errorString()+"\n");
    });
    connect(&process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            &dialog,[&](int code,QProcess::ExitStatus status){
        progress->setVisible(false);
        check->setEnabled(true);apply->setEnabled(true);
        close->setEnabled(true);browse->setEnabled(true);sourcePath->setEnabled(true);
        append(QString("\n%1 (exit %2).%3\n")
            .arg(status==QProcess::NormalExit&&code==0?"Operation finished":"Operation failed")
            .arg(code)
            .arg(code==0?" Restart HelixEdit after an installed upgrade.":" Review the output above."));
    });
    auto start=[&](const QString &mode){
        if(process.state()!=QProcess::NotRunning)return;
        const QString folder=QDir(sourcePath->text().trimmed()).canonicalPath();
        if(folder.isEmpty() || !QFileInfo::exists(QDir(folder).filePath("CMakeLists.txt")) ||
           !QFileInfo::exists(QDir(folder).filePath(".git"))) {
            QMessageBox::warning(&dialog,"Choose a Git checkout",
                "Select a HelixEdit source checkout containing both CMakeLists.txt and .git. "
                "A ZIP download cannot be updated through Git.");
            return;
        }
        const QString installed=QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath("../share/helixedit/scripts/update_from_source.sh");
        const QString local=QDir(folder).filePath("scripts/update_from_source.sh");
        const QString script=QFileInfo::exists(installed)?installed:local;
        if(!QFileInfo::exists(script)){
            QMessageBox::warning(&dialog,"Updater script missing",
                "Update from source manually once with: git pull --ff-only origin main "
                "and bash scripts/build_ubuntu_deb.sh, then install the new .deb.");
            return;
        }
        if(mode=="--apply" &&
           QMessageBox::question(&dialog,"Build and install update",
             "Fetch source changes, compile, run automated tests, and, after a successful "
             "build, request administrator authentication to replace the installed HelixEdit?\n\n"
             "Source: "+folder+"\nOfficial origin: mac860/HelixEdit (main)") != QMessageBox::Yes)
            return;
        QSettings("HelixEdit","HelixEdit").setValue("updater/source",folder);
        console->clear();
        append("Source: "+folder+"\nMode: "+mode+"\n\n");
        progress->setVisible(true);
        check->setEnabled(false);apply->setEnabled(false);close->setEnabled(false);
        browse->setEnabled(false);sourcePath->setEnabled(false);
        process.setWorkingDirectory(folder);
        process.start("/bin/bash",{script,folder,mode});
    };
    connect(check,&QPushButton::clicked,&dialog,[&]{start("--check");});
    connect(apply,&QPushButton::clicked,&dialog,[&]{start("--apply");});
    dialog.exec();
    // Closing via window decoration while a build runs cancels this process.
    if(process.state()!=QProcess::NotRunning){
        process.kill();
        process.waitForFinished(3000);
    }
#endif
}
} // namespace helix
