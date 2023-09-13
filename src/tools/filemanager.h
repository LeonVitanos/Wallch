#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "glob.h"
#include "desktopenvironment.h"

#include <QMessageBox>
#include <QListWidgetItem>
#include <QFileSystemWatcher>

class FileManager : public QObject
{
    Q_OBJECT

public:
    FileManager();

    static void openFolderOf(QString image = "");
    static void openMultipleFolders(QList<QListWidgetItem*> images);
    static bool foldersAreSame(QString folder1, QString folder2);

    void resetWatchFolders();
    void addPathToWatcher(QString path);
    bool atLeastOneFolderFromTheSetExists();
    bool picturesLocationChanged();
    bool currentFolderExists();
    bool currentSelectionIsASet();
    QString getCurrentWallpapersFolder();
    QStringList getCurrentWallpaperFolders();
    QString currentFolder_;
    QStringList currentFolderList_;

#ifdef Q_OS_LINUX
    static void createDesktopFile(const QString &path, const QString &command, const QString &comment);
#endif

private:
    QFileSystemWatcher *watchFolders_;
    QTimer *researchFoldersTimer_;
    QStringList monitoredFoldersList_;
    void monitor(const QStringList &finalListOfPaths);

public Q_SLOTS:
    void folderChanged();
    void researchFolders();

Q_SIGNALS:
    void prepareToSearchFolders();
    void monitoredFoldersChanged();
    void addFilesToWallpapers(const QString path);
    void currentFolderDoesNotExist();
};

#endif // FILEMANAGER_H
