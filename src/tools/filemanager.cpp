#include "filemanager.h"

FileManager::FileManager(){
    //wallpaperManager_ = (wallpaperManager == NULL) ? new WallpaperManager() : wallpaperManager;

    //Timer for folder monitoring
    researchFoldersTimer_ = new QTimer(this);
    connect(researchFoldersTimer_, SIGNAL(timeout()), this, SLOT(researchFolders()));
}

void FileManager::openFolderOf(QString image){
    Global::openUrl("file:///" + Global::dirnameOf(image));
}

void FileManager::openMultipleFolders(QList<QListWidgetItem*> images){
    QStringList parentFolders;
    int selectedCount = images.count();
    short totalFolderCount=0;

    for(int i=0; i<selectedCount; i++){
        QString currentImage;

        if(gv.iconMode){
            if(images.at(i)->statusTip().isEmpty())
                currentImage = images.at(i)->toolTip();
            else
                currentImage = images.at(i)->statusTip();
        }
        else
            currentImage = images.at(i)->text();

        QString image_folder = Global::dirnameOf(currentImage);

        if(!parentFolders.contains(image_folder)){
            parentFolders << image_folder;
            totalFolderCount++;
        }
    }

    if(totalFolderCount>5 && QMessageBox::question(0, tr("Warning!"), tr("Too many folders to open.")+"<br><br>"+tr("This action is going to open")+" "+QString::number(totalFolderCount)+" "+tr("folders. Are you sure?"))!=QMessageBox::Yes)
        return;

    for(short i=0; i<totalFolderCount; i++)
        openFolderOf(parentFolders.at(i));
}

// File System Watcher

void FileManager::resetWatchFolders(){
    if(watchFolders_ && gv.mainwindowLoaded){
        delete watchFolders_;
    }
    monitoredFoldersList_.clear();
    watchFolders_ = new QFileSystemWatcher(this);
    connect(watchFolders_, SIGNAL(directoryChanged(QString)), this, SLOT(folderChanged()));
}

void FileManager::addPathToWatcher(QString path){
    watchFolders_->addPath(path);
}

void FileManager::folderChanged()
{
    if(researchFoldersTimer_->isActive())
        researchFoldersTimer_->stop();

    researchFoldersTimer_->start(1500);
}

void FileManager::researchFolders()
{
    researchFoldersTimer_->stop();

    Q_EMIT prepareToSearchFolders();

    resetWatchFolders();

    if(currentSelectionIsASet()){
        short foldersListCount=currentFolderList_.count();
        QStringList folderListSubfolders;
        for(short i=0;i<foldersListCount;i++){
            if(QDir(currentFolderList_.at(i)).exists()){
                folderListSubfolders << Global::listFolders(currentFolderList_.at(i), true, true);
            }
        }
        monitor(folderListSubfolders);
    }
    else
        monitor(Global::listFolders(currentFolder_, true, true));

    Q_EMIT monitoredFoldersChanged();
}

void FileManager::monitor(const QStringList &finalListOfPaths){
    // TODO: Check if item count is needed
    Q_FOREACH(QString path, finalListOfPaths){
        if(monitoredFoldersList_.contains(path, Qt::CaseSensitive) || !QFile::exists(path))
            return;

        addPathToWatcher(path);
        monitoredFoldersList_ << path;

        Q_EMIT addFilesToWallpapers(path);
    }
}

bool FileManager::currentFolderExists()
{
    if(currentSelectionIsASet()){
        short folders_list_count=currentFolderList_.count();
        for(short i=0;i<folders_list_count;i++){
            if(QDir(currentFolderList_.at(i)).exists()){
                return true;
                break;
            }
        }
    }
    else if(QDir(currentFolder_).exists())
        return true;

    Q_EMIT currentFolderDoesNotExist();

    return false;
}

QStringList FileManager::getCurrentWallpaperFolders(){
    if(currentSelectionIsASet())
        return currentFolderList_;
    else
        return QStringList() << currentFolder_;
}

bool FileManager::atLeastOneFolderFromTheSetExists()
{
    short foldersListCount=currentFolderList_.count();
    for(short i=0;i<foldersListCount;i++){
        if(QDir(currentFolderList_.at(i)).exists())
            return true;
    }
    return false;
}

bool FileManager::picturesLocationChanged(){
    bool selectionIsOk=false;

    if(currentSelectionIsASet())
    {
        if(atLeastOneFolderFromTheSetExists()){
            short foldersListCount=currentFolderList_.count();
            QStringList folderListSubfolders;
            for(short i=0;i<foldersListCount;i++){
                if(QDir(currentFolderList_.at(i)).exists()){
                    folderListSubfolders << Global::listFolders(currentFolderList_.at(i), true, true);
                }
            }
            monitor(folderListSubfolders);
            selectionIsOk=true;
        }
    }
    else
    {
        currentFolder_= getCurrentWallpapersFolder();
        if(QDir(currentFolder_).exists())
        {
            monitor(Global::listFolders(currentFolder_, true, true));
            selectionIsOk=true;
        }
    }

    return selectionIsOk;
}

bool FileManager::currentSelectionIsASet(){
    short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(currentFolderIndex);
    bool type = settings->value("type", false).toBool();
    settings->endArray();
    return type;
}

QString FileManager::getCurrentWallpapersFolder(){
    //read the default folder location
    short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(currentFolderIndex);
    QString parentFolder = settings->value("item", DesktopEnvironment::getOSWallpaperPath()).toString();
    settings->endArray();
    return parentFolder;
}

bool FileManager::foldersAreSame(QString folder1, QString folder2){
    if(!folder1.endsWith('/'))
        folder1+='/';

    if(!folder2.endsWith('/'))
        folder2+='/';

    return (folder1 == folder2);
}
