#pragma once
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QStringList>

namespace helix {
inline bool isHelixSourceFolder(const QString &path) {
    if(path.isEmpty())return false;
    const QDir dir(path);
    return QFileInfo::exists(dir.filePath("CMakeLists.txt")) &&
           QFileInfo::exists(dir.filePath(".git")) &&
           QFileInfo::exists(dir.filePath("scripts/build_ubuntu_deb.sh"));
}
// The installed /usr/bin/helixedit binary has no inherent path back to its
// source tree. Prefer an explicitly saved path, then a shallow scan of normal
// user source/download folders. The shell updater verifies Git remote origin.
inline QString discoverSourceFolder() {
    const QString saved=QSettings("HelixEdit","HelixEdit")
        .value("updater/source").toString();
    if(isHelixSourceFolder(saved))return QDir(saved).canonicalPath();
    QStringList roots;
    roots<<QDir::currentPath()
         <<QCoreApplication::applicationDirPath()
         <<QDir::homePath()
         <<QDir(QDir::homePath()).filePath("HelixEdit")
         <<QDir(QDir::homePath()).filePath("src")
         <<QDir(QDir::homePath()).filePath("Projects")
         <<QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QStringList candidates;
    for(const auto &root:roots) {
        if(root.isEmpty())continue;
        const QDir dir(root);
        candidates<<dir.absolutePath()<<dir.filePath("HelixEdit");
        if(!dir.exists())continue;
        const auto dirs=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable,
                                          QDir::Time);
        int scanned=0;
        for(const auto &entry:dirs) {
            if(++scanned>100)break;
            candidates<<entry.absoluteFilePath()
                      <<QDir(entry.absoluteFilePath()).filePath("HelixEdit");
        }
    }
    for(const auto &candidate:candidates)if(isHelixSourceFolder(candidate))
        return QDir(candidate).canonicalPath();
    return {};
}
} // namespace helix
