/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "nonguimanager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QScreen>
#include <QActionGroup>

#include <iostream>
#include <getopt.h>
#include "desktopenvironment.h"

#ifdef Q_OS_WIN
    #include <cmath>
    #include <Windows.h>
#endif

#define ARG_NOT_REQ 0
#define ARG_REQ 1
#define ARG_OPT 2

#define UPDATE_STATISTICS_SECONDS_INTERVAL 30000
#define CHECK_INTERNET_INTERVAL 10000

bool NonGuiManager::alreadyRuns(){
    alreadyRunsMem_ = new QSharedMemory("Wallch Memory", this);
    if(alreadyRunsMem_->attach(QSharedMemory::ReadOnly)){
        alreadyRunsMem_->detach();
        return true;
    }
    if(alreadyRunsMem_->create(1)){
        return false;
    }
    return false;
}

void NonGuiManager::getDelay(){
    timerManager_->totalSeconds_=settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();
}

void NonGuiManager::dirChanged(){
    if(researchFoldersTimerMain_.isActive()){
        researchFoldersTimerMain_.stop();
    }
    researchFoldersTimerMain_.start(RESEARCH_FOLDERS_TIMEOUT);
}

void NonGuiManager::resetWatchFolders(bool justDelete){
    if(watchFoldersMain_){
        watchFoldersMain_->deleteLater();
    }
    if(!justDelete){
        watchFoldersMain_ = new QFileSystemWatcher(this);
        connect(watchFoldersMain_, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged()));
    }
}

void NonGuiManager::researchDirs(){
    if(researchFoldersTimerMain_.isActive()){
        researchFoldersTimerMain_.stop();
    }

    resetWatchFolders(false);

    if(!getPicturesLocation(false)){
        return;
    }

    if(gv.randomImagesEnabled){
        wallpaperManager_->setRandomMode(true);
    }
    else
    {
        wallpaperManager_->startOver();
    }
}

void NonGuiManager::onlineBackgroundReady(QString image){
    wallpaperManager_->setBackground(image, true, gv.potdRunning, (gv.potdRunning ? 3 : 2));
}

void NonGuiManager::getFilesFromFolder(const QString &path){
    //this function is for adding files of a monitored folder
    QDir directoryContainingPictures(path, QString(""), QDir::Name, QDir::Files);
    Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
        wallpaperManager_->addWallpaper(path+"/"+pic);
    }
}

void NonGuiManager::potdSetSameImage(){
    Global::debug("Picture Of The Day should already be in your hard drive.");
    QString filename=Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
    if(filename.isEmpty()){
        Global::error("Wallch will now attemp to download again the Picture Of The Day.");
        settings->setValue("last_day_potd_was_set", "");
        settings->setValue("previous_img_url", "retry");
        settings->sync();
        imageFetcher_->setFetchType(FetchType::POTD);
        imageFetcher_->fetch();
    }
    wallpaperManager_->setBackground(filename, true, true, 3);
}

void NonGuiManager::readPictures(const QString &folder){
    /*
     * This function returns a QStringList with all the needed pictures
     * for the process from a folder(it reads it recursively)...
     * Used ONLY with --change
     */
    Global::debug("Wallch is reading pictures from your folder: '"+folder+"'");
    //getting all the subfolders of the parent 'file' folder
    QDir directoryContainingPictures(folder, QString(""), QDir::Name, QDir::Files);
    //looping between the folders and adding each of their images.
    Q_FOREACH(QString currentFolder, Global::listFolders(folder, true, true)){
        directoryContainingPictures.setPath(currentFolder);
        Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
            wallpaperManager_->addWallpaper(currentFolder+"/"+pic);
        }
    }

    if(wallpaperManager_->wallpapersCount() == 0){
        globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
        Global::error("Wallch has not enough pictures to continue.");
        doAction("--stop");
    }
}

bool NonGuiManager::currentSelectionIsASet(){
    short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(currentFolderIndex);
    bool toReturn = settings->value("type", false).toBool();
    settings->endArray();
    return toReturn;
}

bool NonGuiManager::getPicturesLocation(bool init){

    if(!startedWithJustChange_ && init){
        resetWatchFolders(false);
        connect(&researchFoldersTimerMain_, SIGNAL(timeout()), this, SLOT(researchDirs()));
    }

    if(currentSelectionIsASet()){
        //current selection is a set of folders!
        short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
        settings->beginReadArray("pictures_locations");
        settings->setArrayIndex(currentFolderIndex);
        short setFolderCount = settings->beginReadArray(QString::number(currentFolderIndex));
        QStringList setFolders;
        bool atLeastOneExists=false;
        for(int i=0;i<setFolderCount;i++){
            settings->setArrayIndex(i);
            QString curFolder = settings->value("item", QString()).toString();
            if(QDir(curFolder).exists()){
                atLeastOneExists=true;
                setFolders << curFolder;
            }
        }
        settings->endArray();
        settings->endArray();

        if(atLeastOneExists){
            wallpaperManager_->clearWallpapers();
            Q_FOREACH(QString parentFolder, setFolders){
                Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
                    if(!startedWithJustChange_){
                        //no need to monitor anything if this instance is for changing the picture just once.
                        if(!watchFoldersMain_){
                            resetWatchFolders(false);
                        }
                        watchFoldersMain_->addPath(curFolder);
                    }
                    getFilesFromFolder(curFolder);
                }
            }
        }
    }
    else
    {
        //current selection is a simple folder
        QString parentFolder=getCurrentWallpapersFolder();
        if(!QDir(parentFolder).exists()){
            globalParser_->desktopNotify(tr("Folder doesn't exist for the process to continue."), false, "info");
            return false;
        }

        wallpaperManager_->clearWallpapers();

        Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
            if(!startedWithJustChange_){
                //no need to monitor anything if this instance is for changing the picture just once.
                if(!watchFoldersMain_){
                    resetWatchFolders(false);
                }
                watchFoldersMain_->addPath(curFolder);
            }
            getFilesFromFolder(curFolder);
        }
    }

    if(gv.randomImagesEnabled){
        wallpaperManager_->setRandomMode(true);
    }

    if(startedWithJustChange_){
        if(wallpaperManager_->wallpapersCount() == 0){
            globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
            Global::error("Wallch has not enough pictures to continue.");
            doAction("--stop");
            return false;
        }
    }
    else
    {
        if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START){
            globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
            Global::error("Wallch has not enough pictures to continue.");
            doAction("--stop");
            return false;
        }
    }
    return true;
}

void showUsage(short exitCode){
    Global::debug("Wallch " + QString::number(APP_VERSION, 'f', 3) + "\n"\
                  "Usage: wallch [OPTION]\n\n"\
                  "Wallch options:\n"\
                  "-h/--help                Show this help.\n"\
                  "--change                 Change desktop background once by picking randomly an image from the list\n"\
                  "--change[=DIRECTORY]     Change desktop background once to the wallpaper at [=DIRECTORY]\n"\
                  "--start                  Starts changing pictures from the last used list.\n"\
                  "--earth                  Starts live earth, updating every 30 minutes.\n"\
                  "--potd                   Starts picture of the day, updating once a day.\n"\
                  "--website                Starts live website.\n"\
                  "--stop                   Stops the current process, if it is active.\n"\
                  "--next                   Proceeds to the next image, if available.\n"\
                  "--previous               Proceeds to the previous image, if available.\n"\
                  "--pause                  Pauses the current process.\n"\
                  "--quit                   Quits any running instance of wallch.\n "\
                  "--version                Shows current version.");
    exit(exitCode);
}

void NonGuiManager::waitForInternetConnection(){
    Global::debug("Checking for internet connection...");

    if(gv.potdRunning){
        continueWithPotd();
    }
    else if(gv.liveEarthRunning){
        timerManager_->secondsRemaining_=LIVEARTH_INTERVAL;
        continueWithLiveEarth();
    }
    else if(gv.liveWebsiteRunning){
        continueWithWebsite();
    }
}

void NonGuiManager::actionsOnWallpaperChange(){
    if(gv.wallpapersRunning)
    {
        changeWallpaperNow();
        if(timerManager_->secondsRemaining_<=0)
            timerManager_->secondsRemaining_=timerManager_->totalSeconds_;
        if(gv.independentIntervalEnabled)
            Global::saveSecondsLeftNow(timerManager_->secondsRemaining_, 0);
    }
    else if(gv.liveEarthRunning)
    {
        imageFetcher_->setFetchType(FetchType::LE);
        imageFetcher_->fetch();
        timerManager_->secondsRemaining_ = timerManager_->totalSeconds_;
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(timerManager_->secondsRemaining_, 1);
        }
    }
    else if(gv.liveWebsiteRunning)
    {
        //websiteSnapshot_->start();
        timerManager_->secondsRemaining_ = timerManager_->totalSeconds_;
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(timerManager_->secondsRemaining_, 2);
        }
    }
}

void NonGuiManager::updateSeconds(){
    /*
     * This function runs once a second and reduces
     * the timeout_count, which, once 0, will update
     * the desktop background...
     */
    gv.runningTimeOfProcess = QDateTime::currentDateTime();
    if(timerManager_->secondsRemaining_ <= 0){
        actionsOnWallpaperChange();

        gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(timerManager_->secondsRemaining_);
    }
    else
    {
        if(timerManager_->secondsRemaining_ != gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval))
        {
            int secondsToChangingTime = gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
            if(secondsToChangingTime < 0)
            {
                actionsOnWallpaperChange();

                gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(timerManager_->secondsRemaining_);
            }
            else if (!(timerManager_->secondsRemaining_-secondsToChangingTime < -1 || timerManager_->secondsRemaining_-secondsToChangingTime > 1)){
                timerManager_->secondsRemaining_ = secondsToChangingTime;
            }
        }
    }

    timerManager_->secondsRemaining_--;
}

void NonGuiManager::checkPicOfDay(){
    if(Global::timeNowToString() == "00:00"){
        if(justUpdatedPotd_)
        {
            return;
        }
        QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
        QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
            justUpdatedPotd_ = true;
            imageFetcher_->setFetchType(FetchType::POTD);
            imageFetcher_->fetch();
        }
        else
        {
            //the day is the same, setting the same image.
            potdSetSameImage();
        }
    }
    else
    {
        justUpdatedPotd_=false;
        //time has yet to be reached!
        if(generalTimer_->isSingleShot()){
            if(generalTimer_->isActive()){
                generalTimer_->stop();
            }
            //back to normal
            generalTimer_->start(59500);
        }
    }
}

void NonGuiManager::connectToUpdateSecondsSlot(){
    if(generalTimer_ == NULL){
        generalTimer_ = new QTimer(this);
    }
    if(gv.potdRunning){
        connect(generalTimer_, SIGNAL(timeout()), this, SLOT(checkPicOfDay()));
    }
    else
    {
        connect(generalTimer_, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    }
}

void NonGuiManager::disconnectFromSlot(){
    disconnect(generalTimer_, 0, this, 0);
}

void NonGuiManager::connectToCheckInternet(){
    connect(generalTimer_, SIGNAL(timeout()), this, SLOT(waitForInternetConnection()));
}

void NonGuiManager::continueWithLiveEarth(){
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();
    timerManager_->totalSeconds_=LIVEARTH_INTERVAL;
    if(!gv.firstTimeout){
        imageFetcher_->setFetchType(FetchType::LE);
        imageFetcher_->fetch();
    }
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(timerManager_->totalSeconds_, 1);
    }
    generalTimer_->start(1000);
}

void NonGuiManager::continueWithWebsite(){
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();
    //getting the required values from the settings...

    /*
    gv.websiteWebpageToLoad=settings->value("website", "http://google.com").toString();
    gv.websiteInterval=settings->value("website_interval", 6).toInt();
    gv.websiteCropEnabled=settings->value("website_crop", false).toBool();
    gv.websiteCropArea=settings->value("website_crop_area", QRect(0, 0, gv.screenAvailableWidth, gv.screenAvailableHeight)).toRect();
    gv.websiteLoginEnabled=settings->value("website_login", false).toBool();
    gv.websiteLoginUsername=settings->value("website_username", "").toString();
    gv.websiteLoginPasswd=settings->value("website_password", "").toString();
    if(!gv.websiteLoginPasswd.isEmpty()){
        gv.websiteLoginPasswd=Global::base64Decode(gv.websiteLoginPasswd);
    }
    gv.websiteRedirect=settings->value("website_redirect", false).toBool();
    gv.websiteFinalPageToLoad=settings->value("website_final_webpage", "").toString();
    gv.websiteSimpleAuthEnabled=settings->value("website_simple_auth", false).toBool();
    gv.websiteWaitAfterFinishSeconds=settings->value("website_wait_after_finish", 3).toInt();
    gv.websiteJavascriptEnabled=settings->value("website_js_enabled", true).toBool();
    gv.websiteJavascriptCanReadClipboard=settings->value("website_js_can_read_clipboard", false).toBool();
    gv.websiteJavaEnabled=settings->value("website_java_enabled", false).toBool();
    gv.websiteLoadImages=settings->value("website_load_images", true).toBool();
    gv.websiteExtraUsernames=settings->value("website_extra_usernames", QStringList()).toStringList();
    gv.websiteExtraPasswords=settings->value("website_extra_passwords", QStringList()).toStringList();


    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));
    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));

    websiteSnapshot_->setParameters(QUrl(gv.websiteWebpageToLoad), gv.screenAvailableWidth, gv.screenAvailableHeight);
    websiteSnapshot_->setWaitAfterFinish(gv.websiteWaitAfterFinishSeconds);
    websiteSnapshot_->setJavascriptConfig(gv.websiteJavascriptEnabled, gv.websiteJavascriptCanReadClipboard);
    websiteSnapshot_->setJavaEnabled(gv.websiteJavaEnabled);
    websiteSnapshot_->setLoadImagesEnabled(gv.websiteLoadImages);
    websiteSnapshot_->setCrop(gv.websiteCropEnabled, gv.websiteCropArea);

    if(gv.websiteLoginEnabled){
        if(!gv.websiteRedirect || gv.websiteFinalPageToLoad.isEmpty()){
            gv.websiteFinalPageToLoad=gv.websiteWebpageToLoad;
        }
        if(gv.websiteSimpleAuthEnabled){
            websiteSnapshot_->setSimpleAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
        }
        else
        {
            if(gv.websiteExtraUsernames.count()>0 || gv.websiteExtraPasswords.count()>0)
            {
                websiteSnapshot_->setComplexAuthenticationWithPossibleFields(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad, gv.websiteExtraUsernames, gv.websiteExtraPasswords, true);
            }
            else
            {
                websiteSnapshot_->setComplexAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
            }
        }
    }
    else
    {
        websiteSnapshot_->disableAuthentication();
    }

    websiteSnapshot_->setTimeout(WEBSITE_TIMEOUT);

    timerManager_->totalSeconds_=Global::websiteSliderValueToSeconds(gv.websiteInterval);
    if(!gv.firstTimeout){
        websiteSnapshot_->start();
    }
    if(!timerManager_->secondsRemaining_){
        timerManager_->secondsRemaining_=timerManager_->totalSeconds_;
    }
    Global::resetSleepProtection(timerManager_->secondsRemaining_);
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(timerManager_->secondsRemaining_, 2);
    }
    generalTimer_->start(1000);*/
}

void NonGuiManager::continueWithPotd(){
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();

    //yearmonthday contains the last time that picture of the day was changed...
    QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
    QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
    if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
        //the previous time that potd changes was another day
        imageFetcher_->setFetchType(FetchType::POTD);
        imageFetcher_->fetch();
    }
    else
    {
        potdSetSameImage();
    }
    gv.doNotToggleRadiobuttonFallback=false;

    generalTimer_->start(59500);
}

void NonGuiManager::connectToServer()
{
    localServer_ = new QLocalServer(this);

    if(!localServer_->listen(QString(SOCKET_SERVER_NAME))){
#ifdef Q_OS_LINUX
        //if not in windows, remove the socket and retry...
        QLocalServer::removeServer(QString(SOCKET_SERVER_NAME));
        if(!localServer_->listen(QString(SOCKET_SERVER_NAME))){
            Global::error("Could not connect to the local socket server.");
            return;
        }
#else
        Global::error("Could not connect to the local socket server.");
        return;
#endif

    }

    connect(localServer_, SIGNAL(newConnection()), this, SLOT(newSocketConnection()));
}

void NonGuiManager::newSocketConnection(){
    QLocalSocket *clientConnection = localServer_->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32)){
        qApp->processEvents(QEventLoop::AllEvents);
        clientConnection->waitForReadyRead();
    }

    connect(clientConnection, SIGNAL(disconnected()), clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_5_2);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString message;
    in >> message;
    doAction(message);
}

void NonGuiManager::messageServer(const QString &message, bool quitAfterwards){
    messageToSendToServer_ = message;
    quitAfterMessagingMainApplication_ = quitAfterwards;
    socket_ = new QLocalSocket(this);
    socket_->abort();
    connect(socket_, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket_, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socketError()));
    socket_->connectToServer(QString(SOCKET_SERVER_NAME));
}

void NonGuiManager::socketError(){
    if(quitAfterMessagingMainApplication_){
        //it should quit, but sending the message failed. Assume that the instance failed and start a new one.
        processArguments(NULL, QStringList() << QString() << messageToSendToServer_);
    }
}

void NonGuiManager::socketConnected(){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_2);
    out << messageToSendToServer_;
    out.device()->seek(0);
    socket_->write(block);
    socket_->flush();
    if(quitAfterMessagingMainApplication_){
        QTimer::singleShot(100, Qt::CoarseTimer, this, SLOT(quitNow()));
    }
}

void NonGuiManager::quitNow(){
    qApp->exit(0);
}

void NonGuiManager::changeWallpaperNow(){

    if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START && !startedWithJustChange_){
        Global::error("Could not get pictures or not enough pictures.");
        globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
        return;
    }
    if(previousWasClicked_){
        previousWasClicked_=false;
        wallpaperManager_->setBackground(wallpaperManager_->getPreviousWallpaper(), true, true, 1);
    }
    else
    {
        QString image = wallpaperManager_->getNextWallpaper();

        wallpaperManager_->addToPreviousWallpapers(image);

        wallpaperManager_->setBackground(image, true, true, 1);
    }
}

//System tray icon Code
void NonGuiManager::setupTray()
{
    showWindowAction_ = new QAction(tr("Show"), this);
    connect(showWindowAction_, SIGNAL(triggered()), this, SLOT(trayActionShowWindow()));

    openCurrentImageAction_ = new QAction(tr("Open Image"), this);
    connect(openCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImage()));

    openCurrentImageFolderAction_ = new QAction(tr("Open Folder"), this);
    connect(openCurrentImageFolderAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImageFolder()));

    copyCurrentImageAction_ = new QAction(tr("Copy Image"), this);
    connect(copyCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionCopyImage()));

    copyCurrentImagePathAction_ = new QAction(tr("Copy Path"), this);
    connect(copyCurrentImagePathAction_, SIGNAL(triggered()), this, SLOT(trayActionCopyPath()));

    deleteCurrentImageAction_ = new QAction(tr("Delete"), this);
    connect(deleteCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionDeleteCurrentImage()));

    openCurrentImagePropertiesAction_ = new QAction(tr("Properties"), this);
    connect(openCurrentImagePropertiesAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImageProperties()));

    wallpapersAction_ = new QAction(tr("Wallpapers"), this);
    connect(wallpapersAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapers()));

    wallpapersOnceAction_ = new QAction("   "+tr("Change wallpaper once"), this);
    connect(wallpapersOnceAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersOnce()));

    wallpapersPauseAction_ = new QAction("   "+tr("Pause"), this);
    connect(wallpapersPauseAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersPause()));

    wallpapersNextAction_ = new QAction("   "+tr("Next"), this);
    connect(wallpapersNextAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersNext()));

    wallpapersPreviousAction_ = new QAction("   "+tr("Previous"), this);
    connect(wallpapersPreviousAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersPrevious()));

    liveEarthAction_ = new QAction(tr("Live Earth"), this);
    connect(liveEarthAction_, SIGNAL(triggered()), this, SLOT(trayActionLiveEarth()));

    pictureOfTheDayAction_ = new QAction(tr("Picture Of The Day"), this);
    connect(pictureOfTheDayAction_, SIGNAL(triggered()), this, SLOT(trayActionPictureOfTheDay()));

    liveWebsiteAction_ = new QAction(tr("Live Website"), this);
    connect(liveWebsiteAction_, SIGNAL(triggered()), this, SLOT(trayActionLiveWebsite()));

    preferencesAction_ = new QAction(tr("Preferences"), this);
    connect(preferencesAction_, SIGNAL(triggered()), this, SLOT(trayActionPreferences()));

    aboutAction_ = new QAction(tr("About"), this);
    connect(aboutAction_, SIGNAL(triggered()), this, SLOT(trayActionAbout()));

    quitAction_ = new QAction(tr("Exit"), this);
    connect(quitAction_, SIGNAL(triggered()), this, SLOT(trayActionQuit()));

    //a timer just to not allow constant scrolling on the tray
    trayWheelTimer_ = new QTimer(this);
    trayWheelTimer_->setSingleShot(true);

    trayIconMenu_ = new QMenu();
    createTray();

    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setContextMenu(trayIconMenu_);
    connect(trayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivatedActions(QSystemTrayIcon::ActivationReason)));
    trayIcon_->installEventFilter(this);
    trayIcon_->setIcon(QIcon(":/images/wallch.png"));
    trayIcon_->setToolTip("Wallch\nWallpaper changer");
    trayIcon_->show();
}

bool NonGuiManager::eventFilter(QObject *object, QEvent *event){
    if(object == trayIcon_ && event->type() == QEvent::Wheel){
        if(trayWheelTimer_->isActive()){
            //timeout not yet passed
            return false;
        }
        //allow another scroll event in 0.5 seconds (aka 2 changes per second limit)
        trayWheelTimer_->start(500);

        bool scrolledUp = ((QWheelEvent*) event)->delta() > 0;

        if(scrolledUp){
            doAction("--previous");
        }
        else
        {
            doAction("--next");
        }
        return true;
    }
    return false;
}

void NonGuiManager::trayActivatedActions(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::DoubleClick)
    {
        if (mainWindowLaunched_) {
            // mainwindow has launched, either focus to it or hide it
            Q_EMIT signalHideOrShow();
        } else {
            // mainwindow has yet to launch, just focus to it
            doAction("--focus");
        }
    }
}

void NonGuiManager::createTray()
{
    trayIconMenu_->clear();
    trayIconMenu_->addAction(showWindowAction_);
    currentImageMenu_ = new QMenu("Current Image");
    currentImageMenu_->addAction(openCurrentImageAction_);
    currentImageMenu_->addAction(openCurrentImageFolderAction_);
    currentImageMenu_->addAction(copyCurrentImagePathAction_);
    currentImageMenu_->addAction(copyCurrentImageAction_);
    currentImageMenu_->addAction(deleteCurrentImageAction_);
    currentImageMenu_->addAction(openCurrentImagePropertiesAction_);
    trayIconMenu_->addMenu(currentImageMenu_);
    trayIconMenu_->addSeparator();
    trayIconMenu_->addAction(wallpapersAction_);
    if(!gv.wallpapersRunning)
        trayIconMenu_->addAction(wallpapersOnceAction_);
    if(gv.wallpapersRunning)
    {
        wallpapersAction_->setCheckable(true);
        wallpapersAction_->setChecked(true);
        if(gv.processPaused){
            wallpapersPauseAction_->setText("   "+tr("Start"));
            trayIconMenu_->addAction(wallpapersPauseAction_);
        }
        else{
            wallpapersPauseAction_->setText("   "+tr("Pause"));
            trayIconMenu_->addAction(wallpapersPauseAction_);
            trayIconMenu_->addAction(wallpapersNextAction_);
            trayIconMenu_->addAction(wallpapersPreviousAction_);
        }
    }
    else if(gv.liveEarthRunning)
    {
        liveEarthAction_->setCheckable(true);
        liveEarthAction_->setChecked(true);
    }
    else if(gv.potdRunning)
    {
        pictureOfTheDayAction_->setCheckable(true);
        pictureOfTheDayAction_->setChecked(true);
    }
    else if(gv.liveWebsiteRunning)
    {
        liveWebsiteAction_->setCheckable(true);
        liveWebsiteAction_->setChecked(true);
    }
    else
    {
        uncheckRunningFeatureOnTray();
    }
    trayIconMenu_->addAction(liveEarthAction_);
    trayIconMenu_->addAction(pictureOfTheDayAction_);
    trayIconMenu_->addAction(liveWebsiteAction_);
    QActionGroup* myGroup = new QActionGroup(this);
    myGroup->addAction(wallpapersAction_);
    myGroup->addAction(liveEarthAction_);
    myGroup->addAction(pictureOfTheDayAction_);
    myGroup->addAction(liveWebsiteAction_);
    trayIconMenu_->addSeparator();
    trayIconMenu_->addAction(preferencesAction_);
    trayIconMenu_->addAction(aboutAction_);
    trayIconMenu_->addAction(quitAction_);
}

void NonGuiManager::trayActionShowWindow()
{
    doAction("--focus");
}

void NonGuiManager::trayActionCopyPath()
{
    if(WallpaperManager::currentBackgroundExists())
        QApplication::clipboard()->setText(WallpaperManager::currentBackgroundWallpaper());
}

void NonGuiManager::trayActionCopyImage()
{
    if(WallpaperManager::currentBackgroundExists())
        QApplication::clipboard()->setImage(QImage(WallpaperManager::currentBackgroundWallpaper()), QClipboard::Clipboard);
}

void NonGuiManager::trayActionOpenCurrentImage()
{
    if(!WallpaperManager::currentBackgroundExists())
        return;

    Global::openUrl("file:///"+WallpaperManager::currentBackgroundWallpaper());
}

void NonGuiManager::trayActionOpenCurrentImageFolder()
{
    if(WallpaperManager::currentBackgroundExists())
        WallpaperManager::openFolderOf();
}

void NonGuiManager::trayActionDeleteCurrentImage()
{
    doAction("--delete-current");
}

void NonGuiManager::trayActionOpenCurrentImageProperties()
{
    doAction("--properties");
}

void NonGuiManager::trayActionWallpapers()
{
    if(!gv.wallpapersRunning){
        doAction("--start");
    }
    else{
        doAction("--stop");
    }

    createTray();
}

void NonGuiManager::trayActionWallpapersOnce()
{
    doAction("--change");
}

void NonGuiManager::trayActionWallpapersPause()
{
    doAction("--pause");
}

void NonGuiManager::trayActionWallpapersNext()
{
    doAction("--next");
}

void NonGuiManager::trayActionWallpapersPrevious()
{
    doAction("--previous");
}

void NonGuiManager::trayActionLiveEarth()
{
    doAction("--earth");
    createTray();
}

void NonGuiManager::trayActionPictureOfTheDay()
{
    doAction("--potd");
    createTray();
}

void NonGuiManager::trayActionLiveWebsite()
{
    doAction("--website");
    createTray();
}

void NonGuiManager::trayActionPreferences()
{
    doAction("--preferences");
}

void NonGuiManager::trayActionAbout()
{
    doAction("--about");
}

void NonGuiManager::trayActionQuit()
{
    doAction("--quit");
}

void NonGuiManager::uncheckRunningFeatureOnTray()
{
    wallpapersAction_->setCheckable(false);
    liveEarthAction_->setCheckable(false);
    pictureOfTheDayAction_->setCheckable(false);
    liveWebsiteAction_->setCheckable(false);
}
//End of System tray icon code

void NonGuiManager::propertiesDestroyed()
{
    propertiesShown_=false;
}

void NonGuiManager::preferencesDestroyed(){
    gv.preferencesDialogShown=false;
}

void NonGuiManager::doAction(const QString &message){
    /*
     * This function controls messages coming from external sources,
     * like from the tray
     */

    if(message == "--focus"){

        if(mainWindowLaunched_){
            Global::debug("Focusing to the Wallch window.");
            Q_EMIT signalFocus();
            return;
        }

        Global::debug("Loading the Wallch window.");

        //stop any process from nonGUI (they will continue to GUI)
        if(generalTimer_->isActive()){
            generalTimer_->stop();
        }

        resetWatchFolders(true);

        Global::debug("Focus was requested! Continuing the process to the Graphical Interface");

        //if(websiteSnapshot_)
            //disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));


        mainWindowLaunched_=true;

        MainWindow *w;

        if(gv.wallpapersRunning){
            w = new MainWindow(alreadyRunsMem_, globalParser_, imageFetcher_, websiteSnapshot_,
                               wallpaperManager_, timerManager_);
        }
        else if(gv.liveWebsiteRunning || gv.liveEarthRunning){
            w = new MainWindow(alreadyRunsMem_, globalParser_, imageFetcher_, websiteSnapshot_, wallpaperManager_,
                               timerManager_);
        }
        else
        {
            w = new MainWindow(alreadyRunsMem_, globalParser_, imageFetcher_, websiteSnapshot_, wallpaperManager_,
                               timerManager_);
        }

        connectMainwindowWithExternalActions(w);
        w->show();
    }
    else if(message == "--earth"){
        if(mainWindowLaunched_){
            if(gv.liveEarthRunning){
                Global::debug("Stopping the Live Earth process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Live Earth process.");
                Q_EMIT signalActivateLivearth();
            }
            return;
        }

        if(gv.liveEarthRunning){
            Global::debug("Stopping the Live Earth process.");
            doAction("--stop");
            return;
        }

        Global::debug("Switching to Live Earth mode.");
        if(gv.liveWebsiteRunning){
            /*if(websiteSnapshot_->isLoading()){
                websiteSnapshot_->stop();
            }*/
        }
        if(gv.potdRunning){
            imageFetcher_->abort();
        }

        gv.liveWebsiteRunning=gv.potdRunning=gv.wallpapersRunning=false;
        gv.liveEarthRunning=true;
        Global::updateStartup();
        if(generalTimer_->isActive()){
            generalTimer_->stop();
        }
        this->disconnectFromSlot();
        this->connectToUpdateSecondsSlot();
        imageFetcher_->setFetchType(FetchType::LE);
        imageFetcher_->fetch();
        timerManager_->secondsRemaining_=timerManager_->totalSeconds_=LIVEARTH_INTERVAL;
        Global::resetSleepProtection(timerManager_->secondsRemaining_);
        generalTimer_->start(1000);
    }
    else if(message == "--potd"){
        if(mainWindowLaunched_){
            if(gv.potdRunning){
                Global::debug("Stopping the Picture Of The Day process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Picture Of The Day process.");
                Q_EMIT signalActivatePotd();
            }
            return;
        }

        if(gv.potdRunning){
            Global::debug("Stopping the Picture Of The Day process.");
            doAction("--stop");
            return;
        }

        Global::debug("Switching to Picture Of The Day mode.");
        if(gv.liveWebsiteRunning){
            /*if(websiteSnapshot_->isLoading()){
                websiteSnapshot_->stop();
            }*/
        }
        if(gv.liveEarthRunning){
            imageFetcher_->abort();
        }

        gv.liveEarthRunning=gv.liveWebsiteRunning=gv.wallpapersRunning=false;
        gv.potdRunning = true;
        Global::updateStartup();
        if(generalTimer_->isActive()){
            generalTimer_->stop();
        }
        this->disconnectFromSlot();
        this->continueWithPotd();
    }
    else if(message == "--website"){
        if(mainWindowLaunched_){
            if(gv.liveWebsiteRunning){
                Global::debug("Stopping the Live Website process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Live Website process.");
                Q_EMIT signalActivateLiveWebsite();
            }
            return;
        }

        if(gv.liveWebsiteRunning){
            Global::debug("Stopping the Live Website process.");
            doAction("--stop");
            return;
        }
        if(websiteSnapshot_==NULL){
            websiteSnapshot_ = new WebsiteSnapshot();
        }

        Global::debug("Switching to Live Website mode.");
        if(gv.liveEarthRunning || gv.potdRunning){
            imageFetcher_->abort();
        }

        gv.potdRunning=gv.liveEarthRunning=gv.wallpapersRunning=false;
        gv.liveWebsiteRunning=true;
        Global::updateStartup();
        if(generalTimer_->isActive()){
            generalTimer_->stop();
        }
        this->disconnectFromSlot();
        timerManager_->secondsRemaining_=0;
        this->continueWithWebsite();
    }
    else if(message == "--start"){

        if(mainWindowLaunched_){
            if(!gv.wallpapersRunning || gv.processPaused){
                Global::debug("Activating the Wallpapers process.");
                Q_EMIT signalStart();
            }
            else
            {
                Global::debug("Stopping the Wallpapers process.");
                doAction("--stop");
            }
            return;
        }

        if(gv.wallpapersRunning){
            if(gv.processPaused){
                doAction("--pause"); //continue from the pause
            }
            else
            {
                Global::debug("Stopping the Wallpapers process.");
                doAction("--stop");
            }
        }
        else
        {
            if(startedWithLiveEarth_ || startedWithWebsite_ || startedWithPotd_ || startedWithNone_){
                startedWithNone_=startedWithLiveEarth_=startedWithWebsite_=startedWithPotd_=false;
                checkSettings(true);
                if(!getPicturesLocation(true)){
                    return;
                }
                if(gv.randomImagesEnabled){
                    wallpaperManager_->setRandomMode(true);
                }
            }
            Global::debug("Switching to Wallpapers mode.");
           gv.potdRunning=gv.liveEarthRunning=gv.liveWebsiteRunning=false;
            gv.wallpapersRunning=true;
            Global::updateStartup();

            if(generalTimer_->isActive()){
                generalTimer_->stop();
            }
            this->disconnectFromSlot();
            this->connectToUpdateSecondsSlot();

            getDelay();
            timerManager_->secondsRemaining_=0;
            Global::resetSleepProtection(timerManager_->secondsRemaining_);
            generalTimer_->start(1000);
        }
    }
    else if(message == "--change"){
        if(mainWindowLaunched_){
            Q_EMIT signalOnce();
            return;
        }

        QString parentFolder=getCurrentWallpapersFolder();
        if(!QDir(parentFolder).exists()){
            globalParser_->desktopNotify(tr("Folder doesn't exist for the process to continue."), false, "info");
            return;
        }

        wallpaperManager_->clearWallpapers();

        Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
            getFilesFromFolder(curFolder);
        }

        if(gv.randomImagesEnabled){
            wallpaperManager_->setRandomMode(true);
        }

        wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
    }
    else if(message == "--pause"){

        if(mainWindowLaunched_){
            Q_EMIT signalPause();
            return;
        }

        if(!gv.wallpapersRunning && !gv.processPaused){
            Global::error("Cannot pause to any other process rather than Wallpapers!");
            return;
        }
        if(!gv.processPaused){
            Global::debug("Pausing the Wallpapers process.");
            generalTimer_->stop();
            gv.processPaused=true;
        }
        else
        {
            Global::resetSleepProtection(timerManager_->secondsRemaining_);
            Global::debug("Continuing from the pause...");
            gv.processPaused=false;
            generalTimer_->start(1000);
        }
    }
    else if(message == "--stop"){
        if(mainWindowLaunched_){
            Q_EMIT closeWhatsRunning();
            return;
        }

        if(generalTimer_->isActive()){
            generalTimer_->stop();
        }

        if(gv.wallpapersRunning)
        {
            wallpaperManager_->startOver();

            gv.processPaused=false;

            timerManager_->secondsRemaining_=timerManager_->totalSeconds_;

            if(gv.independentIntervalEnabled){
                //resetting the configuration!
                Global::saveSecondsLeftNow(-1, 0);
            }
        }
        else if(gv.liveEarthRunning || gv.liveWebsiteRunning){
            timerManager_->secondsRemaining_=timerManager_->totalSeconds_;
        }

        if(gv.liveEarthRunning || gv.potdRunning){
            imageFetcher_->abort();
        }

        gv.wallpapersRunning=gv.liveEarthRunning=gv.potdRunning=gv.liveWebsiteRunning=false;
        Global::updateStartup();
    }
    else if(message == "--next"){

        if (gv.wallpapersRunning) {

            if (mainWindowLaunched_) {
                Q_EMIT signalNext();
                return;
            }

            if(generalTimer_->isActive()){
                generalTimer_->stop();
                timerManager_->secondsRemaining_=0;
                updateSeconds();
                generalTimer_->start(1000);
            }

        } else {
            Global::error("Υou can use 'next' only in Wallpapers mode.");
        }
    }
    else if(message == "--previous"){

        if(mainWindowLaunched_){
            Q_EMIT signalPrevious();
            return;
        }

        if(gv.wallpapersRunning){
            if(generalTimer_->isActive()){
                if(gv.wallpapersRunning){
                    generalTimer_->stop();
                    timerManager_->secondsRemaining_=0;
                    previousWasClicked_=true;
                    updateSeconds();
                    generalTimer_->start(1000);
                }
            }
            else
            {
                Global::error("Wallpapers mode is not running in order to use 'previous'");
            }
        }
        else
        {
            Global::error("Υou can use 'previous' only in Wallpapers mode.");
        }
    }
    else if(message == "--preferences"){

        if(mainWindowLaunched_){
            Q_EMIT signalShowPreferences();
            return;
        }
        if(gv.preferencesDialogShown){
            return;
        }
        gv.preferencesDialogShown=true;
        preferences_ = new Preferences();
        preferences_->setModal(true);
        preferences_->setAttribute(Qt::WA_DeleteOnClose);
        connect(preferences_, SIGNAL(destroyed()), this, SLOT(preferencesDestroyed()));
        preferences_->show();
    }
    else if(message == "--properties"){
        if(propertiesShown_ || !WallpaperManager::currentBackgroundExists()){
            return;
        }
        QString imageFilename = WallpaperManager::currentBackgroundWallpaper();

        propertiesShown_=true;
        properties_ = new Properties(imageFilename, false, 0, 0);
        properties_->setModal(true);
        properties_->setAttribute(Qt::WA_DeleteOnClose);
        connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
        properties_->show();
    }
    else if(message == "--about"){

        if(mainWindowLaunched_){
            Q_EMIT signalShowAbout();
            return;
        }

        About ab(0);
        ab.exec();
    }
    else if(message == "--delete-current"){
        if(!WallpaperManager::currentBackgroundExists())
            return;
        if(mainWindowLaunched_){
            Q_EMIT signalDeleteCurrent();
            return;
        }
        if(QMessageBox::question(0, QObject::tr("Confirm deletion"), QObject::tr("Are you sure you want to permanently delete the current image?"))==QMessageBox::Yes)
        {
            if(!QFile::remove(WallpaperManager::currentBackgroundWallpaper())){
                QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("There was a problem deleting the current image. Please make sure you have the permission to delete the image or that the image exists."));
            }
        }
    }
    else if(message == "--quit"){
        if(mainWindowLaunched_){
            Q_EMIT signalQuit();
            return;
        }
        Global::debug("See ya next time!");
        // manually detach from the memory
        alreadyRunsMem_->detach();
        qApp->quit();
    }
    else if(message.startsWith("MONITOR:")){
        QString folder=message.right(message.length()-8);
        if(mainWindowLaunched_){
            Q_EMIT signalAddFolderForMonitor(folder);
            return;
        }

        //update the configuration to point to the new folder
        setDefaultFolderInSettings(folder);

        if(gv.wallpapersRunning){
            researchDirs();
        }
    }
    else{
        Global::error("This message is unknown to me.");
    }
}

QString NonGuiManager::getCurrentWallpapersFolder(){
    //read the default folder location
    short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(currentFolderIndex);
    QString parentFolder = settings->value("item", DesktopEnvironment::getOSWallpaperPath()).toString();
    settings->endArray();
    return parentFolder;
}

void NonGuiManager::setDefaultFolderInSettings(const QString &folder){
    bool alreadyInSettings=false;
    short count = settings->beginReadArray("pictures_locations");
    if(QDir(folder).exists()){
        for(short i=0; i<count; i++)
        {
            settings->setArrayIndex(i);
            if(Global::foldersAreSame(folder, settings->value("item").toString()))
            {
                settings->endArray();
                settings->setValue("currentFolder_index", i);
                alreadyInSettings=true;
                break;
            }
        }
        settings->sync();
    }
    else
    {
        settings->endArray();
        Global::error("The folder "+folder+" does not exist!");
        return;
    }
    if(!alreadyInSettings){
        //Add the folder to the index and point to it as default
        settings->endArray();
        settings->beginWriteArray("pictures_locations");
        settings->setArrayIndex(count);
        settings->setValue("item", folder);
        settings->endArray();
        settings->setValue("currentFolder_index", count);
        settings->sync();
    }
}

void NonGuiManager::setIndependentInterval(const QString &independentInterval){
    if(!independentInterval.contains(".")){
        return;
    }

    QStringList parts=independentInterval.split(".");
    if(parts.count()!=3){
        return;
    }
    if(parts.at(0)=="e"){
        //independence was referring to live earth!
        if(!startedWithLiveEarth_){
            return;
        }
    }
    else if(parts.at(0)=="w"){
        //independence was referring to live website!
        if(!startedWithWebsite_){
            return;
        }
    }
    else
    {
        //independence was referring to wallpapers
        if(startedWithWebsite_ || startedWithLiveEarth_){
            return;
        }
    }
    bool ok;
    int independence_change_seconds_left = parts.at(1).toInt(&ok);
    if(!ok){
        return;
    }
    if(independence_change_seconds_left==-1){
        return;
    }

    QDateTime time_then = QDateTime::fromString(parts.at(2), "yyyy:MM:dd:HH:mm:ss");
    if(time_then.isNull() || !time_then.isValid()){
        return;
    }

    time_then.addSecs(independence_change_seconds_left);

    int secs_diff = time_then.secsTo(QDateTime::currentDateTime());
    independence_change_seconds_left-=secs_diff;

    if(independence_change_seconds_left<0){
        independence_change_seconds_left=0;
    }
    if(independence_change_seconds_left>0){
        Global::debug("Interval independence is enabled ("+QString::number(independence_change_seconds_left)+" seconds)");
        timerManager_->secondsRemaining_=independence_change_seconds_left;
    }
}

void NonGuiManager::checkSettings(bool allSettings){

    // The least checks for the --change argument
    gv.setAverageColor = settings->value("average_color", false).toBool();
    gv.showNotification=settings->value("notification", false).toBool();
    gv.saveHistory=settings->value("history", true).toBool();
    gv.symlinks=settings->value("symlinks", false).toBool();

    if(allSettings){
        //checks all needed settings
        gv.pauseOnBattery=settings->value("pause_on_battery", false).toBool();



        gv.potdIncludeDescription = settings->value("potd_include_description", true).toBool();
        gv.leEnableTag = settings->value("le_enable_tag", false).toBool();
        gv.potdDescriptionBottom = settings->value("potd_description_bottom", true).toBool();
        gv.potdDescriptionFont = settings->value("potd_description_font", "Arial").toString();
        gv.potdDescriptionColor = settings->value("potd_text_color", "#FFFFFF").toString();
        gv.potdDescriptionBackgroundColor = settings->value("potd_background_color", "#000000").toString();
        gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (0)).toInt();
        gv.potdDescriptionRightMargin = settings->value("potd_description_right_margin", 0).toInt();
        gv.potdDescriptionBottomTopMargin = settings->value("potd_description_bottom_top_margin", 0).toInt();

        gv.typeOfInterval=settings->value("typeOfIntervals", 0).toInt();
        if(startedWithLiveEarth_ || startedWithWebsite_ || !startedWithPotd_){
            gv.independentIntervalEnabled = settings->value("independent_interval_enabled", true).toBool();
            if(gv.independentIntervalEnabled){
                //check if there is an interval to follow
                setIndependentInterval(settings->value("seconds_left_interval_independence", INTERVAL_INDEPENDENCE_DEFAULT_VALUE).toString());
            }
        }
        else
        {
            gv.independentIntervalEnabled=false;
        }

        gv.randomImagesEnabled=settings->value("random_images_enabled", true).toBool();

        if(!gv.randomImagesEnabled){
            wallpaperManager_->setRandomMode(false);
        }

        gv.firstTimeout=settings->value("first_timeout", false).toBool();

        if(gv.randomImagesEnabled){
            srand(QDateTime::currentMSecsSinceEpoch());
        }

        gv.potdOnlineUrl=settings->value("potd_online_url", POTD_ONLINE_URL).toString();
        gv.potdOnlineUrlB=settings->value("potd_online_urlB", POTD_ONLINE_URL_B).toString();
        gv.liveEarthOnlineUrl=settings->value("line_earth_online_url", LIVEARTH_ONLINE_URL).toString();
        gv.liveEarthOnlineUrlB=settings->value("live_earth_online_urlB", LIVEARTH_ONLINE_URL_B).toString();
    }
}

void NonGuiManager::connectMainwindowWithExternalActions(MainWindow *w){
    //connects MainWindow with command line messages.
    QObject::connect(nongui, SIGNAL(signalOnce()), w, SLOT(justChangeWallpaper()));
    QObject::connect(nongui, SIGNAL(signalPause()), w, SLOT(on_startButton_clicked()));
    QObject::connect(nongui, SIGNAL(signalPrevious()), w, SLOT(on_previous_Button_clicked()));
    QObject::connect(nongui, SIGNAL(signalNext()), w, SLOT(on_next_Button_clicked()));
    QObject::connect(nongui, SIGNAL(signalStart()), w, SLOT(on_startButton_clicked()));
    QObject::connect(nongui, SIGNAL(signalActivateLivearth()), w, SLOT(on_activate_livearth_clicked()));
    QObject::connect(nongui, SIGNAL(signalActivatePotd()), w, SLOT(on_activate_potd_clicked()));
    QObject::connect(nongui, SIGNAL(signalActivateLiveWebsite()), w, SLOT(on_activate_website_clicked()));
    QObject::connect(nongui, SIGNAL(closeWhatsRunning()), w, SLOT(closeWhatsRunning()));
    QObject::connect(nongui, SIGNAL(signalShowPreferences()), w, SLOT(on_action_Preferences_triggered()));
    QObject::connect(nongui, SIGNAL(signalShowAbout()), w, SLOT(on_action_About_triggered()));
    QObject::connect(nongui, SIGNAL(signalQuit()), w, SLOT(doQuit()));
    QObject::connect(nongui, SIGNAL(signalDeleteCurrent()), w, SLOT(on_actionDelete_triggered()));
    QObject::connect(nongui, SIGNAL(signalAddFolderForMonitor(const QString&)), w, SLOT(addFolderForMonitor(const QString&)));
    QObject::connect(nongui, SIGNAL(signalFocus()), w, SLOT(showNormal()));
    QObject::connect(nongui, SIGNAL(signalHideOrShow()), w, SLOT(hideOrShow()));
    QObject::connect(w, SIGNAL(signalUncheckRunningFeatureOnTray()), nongui , SLOT(uncheckRunningFeatureOnTray()));
    QObject::connect(w, SIGNAL(signalRecreateTray()), nongui , SLOT(createTray()));
}

std::string NonGuiManager::percentToProgressbar(short percentage){
    short loop_count=floor((float)(percentage/10.0));
    std::string progressbar_string;
    for(short i=0;i<loop_count;i++)
        progressbar_string+='=';
    loop_count=10-loop_count;
    for(short i=0;i<loop_count;i++)
        progressbar_string+=' ';
    return '['+progressbar_string+"] "+QString::number(percentage).toStdString()+"%";
}

void NonGuiManager::clearCurrentLine(short previousOutputCount){
    std::cout << '\r';
    for(short j=0;j<previousOutputCount;j++)
        std::cout << " ";
}

void NonGuiManager::liveWebsiteImageReady(QImage *image, short errorCode){
    if(errorCode==0){
        //no error!

        Global::remove(gv.wallchHomePath+LW_IMAGE+"*");
        QString filename=gv.wallchHomePath+LW_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+".png";

        image->save(filename);
        delete image;

        wallpaperManager_->setBackground(filename, true, true, 5);
        QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        QFile(filename).link(gv.wallchHomePath+LW_PREVIEW_IMAGE);
    }
    else
    {
        switch(errorCode){
        case 1:
            Global::error("Some of the requested pages failed to load successfully.");
            break;
        case 2:
            Global::error("Simple authentication failed. Please check your username and/or password.");
            break;
        case 3:
            Global::error("Username and/or password fields are not found. Please check that you are pointing at the login page.");
            break;
        case 4:
            Global::error("The timeout has been reached and the image has yet to be created!");
            break;
        default:
            Global::error("Unknown error! Please try with a different web page.");
            break;
        }
    }
}

void NonGuiManager::updateSecondsPassed()
{
    settings->setValue("seconds_passed", settings->value("seconds_passed", 0).toUInt()+UPDATE_STATISTICS_SECONDS_INTERVAL/1000);
}

void NonGuiManager::startStatisticsTimer(){
    updateSecondsPassed_ = new QTimer(this);
    connect(updateSecondsPassed_, SIGNAL(timeout()), this, SLOT(updateSecondsPassed()));
    updateSecondsPassed_->start(UPDATE_STATISTICS_SECONDS_INTERVAL);
}

void NonGuiManager::viralSettingsOperations(){
    if(QApplication::instance()){
        QTranslator *translator = new QTranslator(this);
        QString translationsFolder;

#ifdef Q_OS_LINUX
        translationsFolder = QString::fromStdString(PREFIX)+"/share/wallch/translations/";
#else
        translationsFolder = QDir::currentPath()+"/translations/";
#endif
        QString currentLanguage = settings->value("language_file", "system_default").toString();

        if(currentLanguage == "system_default"){
            translator->load(QLocale::system(), "wallch", "_", translationsFolder, ".qm");
        }
        else
        {
            translator->load(translationsFolder+"wallch_"+currentLanguage+".qm");
        }

        QApplication::installTranslator(translator);
    }

    if(settings->value("first-run", true).toBool()){
        settings->setValue("first-run", false);
#ifdef Q_OS_LINUX
        if(!QDir(gv.homePath+"/.config/Wallch/").removeRecursively())
            Global::error("I probably could not remove previous Wallch configurations!");
#else
        if(!QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Wallch/").removeRecursively())
            Global::error("I probably could not remove previous Wallch configurations!");

# ifdef Q_OS_WIN
        QSettings settings2("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wallch.exe", QSettings::NativeFormat);
        settings2.setValue("Default", QDir::currentPath()+"/wallch.exe");
        settings2.setValue("Path", QDir::currentPath()+"/Wallch");
        settings2.sync();
# endif
#endif
    }

    settings->setValue("times_launched", settings->value("times_launched", 0).toUInt()+1); //adding one to times launched
    settings->sync();

#ifdef Q_OS_LINUX
    DesktopEnvironment::setCurrentDE();
    gv.wallchHomePath=gv.homePath+"/.wallch/";
    gv.cachePath=gv.homePath+"/.cache/wallch/thumbs/";
#else
    gv.wallchHomePath=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Wallch/";
    gv.cachePath=gv.wallchHomePath+"/.cache/thumbs/";
#endif
    if(!QDir(gv.wallchHomePath).exists()){
        QDir().mkpath(gv.wallchHomePath);
    }
    if(!QDir(gv.cachePath).exists()){
        QDir().mkpath(gv.cachePath);
    }
    Global::updateStartup();
}

void NonGuiManager::startProgramNormalGui(){
    connectToServer();

    setupTray();
    mainWindowLaunched_=true;
    MainWindow *mainWindow = new MainWindow(alreadyRunsMem_, globalParser_, imageFetcher_, websiteSnapshot_, wallpaperManager_, 0, 0);
    connectMainwindowWithExternalActions(mainWindow);
    mainWindow->show();
    startStatisticsTimer();
}

int NonGuiManager::processArguments(QApplication *app, QStringList arguments){
    //getting custom argc and not the original in case processArguments is not called with the original process arguments
    int argc = arguments.count();

    if(arguments.contains("--start")){
        if(alreadyRuns()){
            messageServer("--start", true);
            return app == NULL ? 0 : app->exec();
        }

        checkSettings(true);

        bool gotPicLocation = getPicturesLocation(true);

        Global::resetSleepProtection(timerManager_->secondsRemaining_);
        if(gotPicLocation){
            getDelay();
            Global::debug("Your Desktop Background will change every "+QString::number(timerManager_->totalSeconds_)+" seconds.");

            if(wallpaperManager_->wallpapersCount()<LEAST_WALLPAPERS_FOR_START){
                Global::error("Too few pictures for image changing. You need at least "+QString(LEAST_WALLPAPERS_FOR_START)+".");
                globalParser_->desktopNotify(tr("You cannot change wallpapers if they are less than 2!"), false, "info");
                return 1;
            }

            if(gv.randomImagesEnabled){
                wallpaperManager_->setRandomMode(true);
            }

            gv.wallpapersRunning=true;
            Global::updateStartup();
            connectToUpdateSecondsSlot();
        }
        connectToServer();
        setupTray();

        if(gotPicLocation){
            generalTimer_->start(1000);
        }

        startStatisticsTimer();
        return app == NULL ? 0 : app->exec();
    }
    else if(arguments.contains("--earth")){
        if(argc > 2){
            Global::error("Argument --earth doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--earth", true);
            startStatisticsTimer();
            return app == NULL ? 0 : app->exec();
        }
        startedWithLiveEarth_=gv.liveEarthRunning=true;
        Global::updateStartup();
        timerManager_->secondsRemaining_ = LIVEARTH_INTERVAL;
        checkSettings(true);
        Global::resetSleepProtection(timerManager_->secondsRemaining_);

        connectToServer();
        setupTray();

        continueWithLiveEarth();

        startStatisticsTimer();
        return app == NULL ? 0 : app->exec();
    }
    else if(arguments.contains("--potd")){
        if(argc > 2){
            Global::error("Argument --potd doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--potd", true);
            return app == NULL ? 0 : app->exec();
        }
        startedWithPotd_=gv.potdRunning=true;
        Global::updateStartup();

        connectToServer();
        setupTray();

        checkSettings(true);

        continueWithPotd();

        startStatisticsTimer();
        return app == NULL ? 0 : app->exec();
    }
    else if(arguments.contains("--website")){
        if(argc>2){
            Global::error("Argument --website doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--website", true);
            startStatisticsTimer();
            return app == NULL ? 0 : app->exec();
        }
        gv.liveWebsiteRunning=startedWithWebsite_=true;
        Global::updateStartup();
        timerManager_->secondsRemaining_=0;
        checkSettings(true);
        Global::resetSleepProtection(timerManager_->secondsRemaining_);

        connectToServer();
        setupTray();

        websiteSnapshot_ = new WebsiteSnapshot();

        continueWithWebsite();
        startStatisticsTimer();
        return app == NULL ? 0 : app->exec();
    }
    else if(arguments.contains("--quit") || arguments.contains("--stop") || arguments.contains("--next") || arguments.contains("--previous") || arguments.contains("--pause")){
        if(argc > 2){
            showUsage(1);
        }

        if(alreadyRuns()){
            QString message;
            QStringList all = QStringList() << "--quit" << "--stop" << "--next" << "--previous" << "--pause";
            int count=all.count();
            for(int i=0;i<count;i++){
                if(arguments.contains(all.at(i))){
                    message=all.at(i);
                    break;
                }
            }
            messageServer(message, true);
        }
        else
        {
            Global::error("No instance seems to be running.");
            exit(1);
        }
    }
    else if(arguments.contains("--none")){
        if(arguments.indexOf("--none") != 1 || argc != 2){
            showUsage(1);
        }
        if(alreadyRuns()){
            Global::error("Wallch seems to already run! Exiting this instance!");
            return 0;
        }
        startedWithNone_ = true;

        checkSettings(true);
        connectToServer();
        setupTray();

        startStatisticsTimer();
        return app == NULL ? 0 : app->exec();
    }
    else if(arguments.contains("--focus")){
        if(app == NULL){
            //previous wallch instance crashed and tried to send the message focus to it.
            startProgramNormalGui();
        }
        else
        {
            //normally called with the --focus argument, attempt to send the message to the server
            if(alreadyRuns()){
                messageServer("--focus", true);
                startStatisticsTimer();
            }
        }
        return app == NULL ? 0 : app->exec();
    }
    else
    {
        //there were arguments, but not for starting wallch hidden... continue...
        if(QDir(QDir().absoluteFilePath(arguments.at(1))).exists()){
            //there's a folder as the first argument
            if(argc > 2){
                Global::error("There is more than a folder in the arguments. Wallch does not know what to do with the extra stuff. See -h/--help for more info");
                return 1;
            }
            else
            {
                QString currentFolder = QDir::cleanPath(QDir().absoluteFilePath(arguments.at(1)));
                if(alreadyRuns()){
                    Global::debug("Folder '"+currentFolder+"' has been sent to the already running instance of Wallch for monitoring.");
                    messageServer("MONITOR:"+currentFolder, true);
                    startStatisticsTimer();
                    return app == NULL ? 0 : app->exec();
                }
                else
                {
                    //just set the folder as the default folder for monitoring
                    setDefaultFolderInSettings(currentFolder);
                    Global::debug("Folder '"+currentFolder+"' is now the default folder for Wallch monitoring.");
                    globalParser_->desktopNotify(tr("The default folder has changed."), false, "info");
                    return 0;
                }
            }
        }
    }
    return 0;
}

int NonGuiManager::startProgram(int argc, char *argv[]){
    if(argc > 1){

#ifdef Q_OS_WIN
    if(settings->value("startup_timeout", 3).toInt()!=0)
    {
        Sleep(settings->value("startup_timeout", 3).toInt()*1000);
    }
#endif

        const struct option longopts[] =
        {
            {"version",   ARG_NOT_REQ,        0, 'v'},
            {"help",      ARG_NOT_REQ,        0, 'h'},
            {"change",     ARG_OPT,  0, 'c'},
            {0,0,0,0},
        };

        int index;
        int iarg=0;

        opterr=0; //turn off getopt error message

        while(iarg != -1)
        {
            iarg = getopt_long(argc, argv, "c:vh", longopts, &index);
            switch (iarg)
            {
            case 'h':
                showUsage(0);
                break;
            case 'v':
                Global::debug("Wallch - Wallpaper Changer, Version " + QString::number(APP_VERSION, 'f', 3));
                return 0;
                break;
            case 'c':
            {
                viralSettingsOperations();
                wallpaperManager_ = new WallpaperManager();

                /*
                 * Just change the current image. This argument always launches when called
                 * correctly (it doesn't matter whether wallch is already running or not)
                */
                startedWithJustChange_=true;
                checkSettings(false);
                if(optarg){
                    //probably the user has specified a folder for working with --change.
                    if(index!=2 || argc > 3){
                        showUsage(1);
                    }
                    QString directoryToReadFrom=QString(optarg);
                    if(QDir(directoryToReadFrom).exists()){
                        //It is a directory. Read its contents instead of the default directory.
                        readPictures(QDir(directoryToReadFrom).absolutePath());
                    }
                    else
                    {
                        Global::error(directoryToReadFrom+" - No such directory.");
                        return 1;
                    }
                }
                else
                {
                    //only --change was specified... Read the pictures of the default folder
                    if(!getPicturesLocation(true)){
                        return 1;
                    }
                }

                if(gv.setAverageColor){
                    wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);

                }
                else
                {
                    wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
                }
                return 0;
                break;
            }
            }
        }
        //second level argument searching!
        QApplication app(argc, argv);

        globalParser_ = new Global();
        imageFetcher_ = new ImageFetcher();
        connect(imageFetcher_, SIGNAL(success(QString)), this, SLOT(onlineBackgroundReady(QString)));
        wallpaperManager_ = new WallpaperManager();
        viralSettingsOperations();
        QApplication::setQuitOnLastWindowClosed(false);

        return processArguments(&app, QCoreApplication::arguments());
    }
    else
    {
        //we have no arguments, proceed
        QApplication app(argc, argv);
        globalParser_ = new Global();
        viralSettingsOperations();
        QApplication::setQuitOnLastWindowClosed(false);
        if(alreadyRuns()){
            messageServer("--focus", true);
            startStatisticsTimer();
            return app.exec();
        }
        startProgramNormalGui();
        return app.exec();
    }
    return 0;
}
