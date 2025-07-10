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
#include "silenced_qtconcurrentrun.h" // IWYU pragma: keep
#include "settingsmanager.h"
#include "websitefeature.h"
#include "potdfeature.h"

#include <QScreen>
#include <QActionGroup>

#include <getopt.h>

#ifdef Q_OS_WIN
    #include <cmath>
    #include <Windows.h>
#else
# ifdef Q_OS_LINUX
    #include "desktopenvironment.h"
# endif
#endif

#define ARG_NOT_REQ 0
#define ARG_REQ 1
#define ARG_OPT 2

#define CHECK_INTERNET_INTERVAL 10000

NonGuiManager::NonGuiManager(FeatureController *featureController,
                             TimerManager *timerManager,
                             WallpaperManager *wallpaperManager,
                             ImageFetcher *imageFetcher,
                             FileManager *fileManager,
                             DialogHelper *dialogHelper,
                             Global *globalParser,
                             WallpapersFeature *wallpapersFeature,
                             LiveEarthFeature *liveEarthFeature,
                             WebsiteFeature *websiteFeature,
                             PotdFeature *potdFeature,
                             QObject *parent)
    : QObject(parent)
    , featureController_(featureController)
    , timerManager_(timerManager)
    , wallpaperManager_(wallpaperManager)
    , imageFetcher_(imageFetcher)
    , fileManager_(fileManager)
    , dialogHelper_(dialogHelper)
    , globalParser_(globalParser)
    , wallpapersFeature_(wallpapersFeature)
    , liveEarthFeature_(liveEarthFeature)
    , websiteFeature_(websiteFeature)
    , potdFeature_(potdFeature)
{
    connect(imageFetcher_, SIGNAL(success(QString)), this, SLOT(onlineBackgroundReady(QString)));
}

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

void NonGuiManager::onlineBackgroundReady(QString image){
    wallpaperManager_->setBackground(image, true, featureController_->isPotdRunning(), (featureController_->isPotdRunning() ? 3 : 2));
}

void NonGuiManager::readPictures(const QString &folder){
    Global::debug("Wallch is reading pictures from your folder: '"+folder+"'");
    fileManager_->addWallpapersFromDirectory(folder);

    if(wallpaperManager_->wallpapersCount() == 0){
        Global().notifyNotEnoughPics();
        doAction("--stop");
    }
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

//TODO:BROKENONLINEFEATURES
void NonGuiManager::waitForInternetConnection(){
    Global::debug("Checking for internet connection...");

    if(featureController_->isPotdRunning()){
        potdFeature_->start();
    }
    else if(featureController_->isLiveEarthRunning()){
        liveEarthFeature_->start();
    }
    else if(featureController_->isWebsiteRunning()){
        websiteFeature_->start();
    }
}

void NonGuiManager::connectToServer()
{
    localServer_ = new QLocalServer(this);

    if(!localServer_->listen(QString(SOCKET_SERVER_NAME))){
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
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
    connect(socket_, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)), this, SLOT(socketError()));
    socket_->connectToServer(QString(SOCKET_SERVER_NAME));
}

void NonGuiManager::socketError(){
    // This function is called when we fail to send a message to another instance.
    if (quitAfterMessagingMainApplication_) {
        // The failure implies the other instance has crashed.

        Global::debug("Could not connect to running instance. Assuming it has crashed.");

        if (messageToSendToServer_ == "--focus") {
            Global::debug("Recovering by launching a new GUI window.");
            startProgramNormalGui();
        } else {
            processArguments(QStringList() << QString() << messageToSendToServer_);
        }
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

        timerManager_->stop();

        fileManager_->resetWatchFolders();

        Global::debug("Focus was requested! Continuing the process to the Graphical Interface");

        //if(websiteSnapshot_)
            //disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));


        mainWindowLaunched_=true;

        MainWindow *w = new MainWindow(alreadyRunsMem_,
                                       globalParser_,
                                       imageFetcher_,
                                       websiteSnapshot_,
                                       wallpaperManager_,
                                       dialogHelper_,
                                       timerManager_,
                                       featureController_);

        connectMainwindowWithExternalActions(w);
        w->show();
    }
    else if(message == "--earth"){
        startFeature(1);
    }
    else if(message == "--potd"){
        startFeature(2);
    }
    else if(message == "--website"){
        startFeature(3);
    }
    else if(message == "--start"){
        startFeature(0);
    }
    else if(message == "--change"){
        if(mainWindowLaunched_){
            Q_EMIT signalOnce();
            return;
        }

        QString parentFolder = fileManager_->getCurrentWallpapersFolder();
        if(!QDir(parentFolder).exists()){
            globalParser_->desktopNotify(tr("Folder doesn't exist for the process to continue."), false, "info");
            return;
        }

        wallpaperManager_->clearWallpapers();

        Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
            fileManager_->addWallpapersFromDirectory(curFolder);
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

        if(!featureController_->isWallpapersRunning() && !wallpapersFeature_->isPaused()){
            Global::error("Cannot pause to any other process rather than Wallpapers!");
            return;
        }
        if(!wallpapersFeature_->isPaused()){
            Global::debug("Pausing the Wallpapers process.");
            timerManager_->stop();
            wallpapersFeature_->setPaused(true);
        }
        else
        {
            Global::resetSleepProtection(timerManager_->secondsRemaining_);
            Global::debug("Continuing from the pause...");
            wallpapersFeature_->setPaused(false);
            timerManager_->start();
        }
    }
    else if(message == "--stop"){
        if(mainWindowLaunched_){
            Q_EMIT closeWhatsRunning();
            return;
        }

        timerManager_->stop();

        if(featureController_->isWallpapersRunning())
        {
            wallpaperManager_->startOver();

            wallpapersFeature_->setPaused(false);

            timerManager_->secondsRemaining_=timerManager_->totalSeconds_;

            timerManager_->saveSecondsLeftNow(false);
        }
        else if(featureController_->isLiveEarthRunning() || featureController_->isWebsiteRunning()){
            timerManager_->secondsRemaining_=timerManager_->totalSeconds_;
        }

        if(featureController_->isLiveEarthRunning() || featureController_->isPotdRunning()){
            imageFetcher_->abort();
        }

        changeRunningFeature(FeatureController::Feature::None);
    }
    else if(message == "--next"){
        if (featureController_->isWallpapersRunning()) {
            if (mainWindowLaunched_) {
                Q_EMIT signalNext();
                return;
            }

            timerManager_->resetTimer();
        }
        else {
            Global::error("Υou can use 'next' only in Wallpapers mode.");
        }
    }
    else if(message == "--previous"){
        if(mainWindowLaunched_){
            Q_EMIT signalPrevious();
            return;
        }

        if(featureController_->isWallpapersRunning()){
            wallpapersFeature_->previousWasClicked_=true;
            timerManager_->resetTimer();
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
        preferences_ = new Preferences(featureController_);
        preferences_->setModal(true);
        preferences_->setAttribute(Qt::WA_DeleteOnClose);
        connect(preferences_, SIGNAL(destroyed()), this, SLOT(preferencesDestroyed()));
        preferences_->show();
    }
    else if(message == "--properties")
        dialogHelper_->showPropertiesDialog();
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
        SettingsManager::setDefaultFolder(folder);

        if(featureController_->isWallpapersRunning())
            fileManager_->researchFolders();
    }
    else{
        Global::error("This message is unknown to me.");
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

    FeatureController::Feature launchFeature = featureController_->launchFeature();
    if (parts.at(0) == "e") {
        if (launchFeature != FeatureController::Feature::LiveEarth) return;
    } else if (parts.at(0) == "w") {
        if (launchFeature != FeatureController::Feature::Website) return;
    } else {
        if (launchFeature == FeatureController::Feature::Website || launchFeature == FeatureController::Feature::LiveEarth) return;
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

    (void)time_then.addSecs(independence_change_seconds_left);

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

void NonGuiManager::connectMainwindowWithExternalActions(MainWindow *w){
    connect(this, &NonGuiManager::signalOnce, w, &MainWindow::justChangeWallpaper);
    connect(this, &NonGuiManager::signalPause, w, &MainWindow::handleStartButtonClick);
    connect(this, &NonGuiManager::signalPrevious, w, &MainWindow::handlePreviousButtonClick);
    connect(this, &NonGuiManager::signalNext, w, &MainWindow::handleNextButtonClick);
    connect(this, &NonGuiManager::signalStart, w, &MainWindow::handleStartButtonClick);
    connect(this, &NonGuiManager::signalActivateLivearth, w, &MainWindow::handleActivateLiveEarthClick);
    connect(this, &NonGuiManager::signalActivatePotd, w, &MainWindow::handleActivatePotdClick);
    connect(this, &NonGuiManager::signalActivateLiveWebsite, w, &MainWindow::handleActivateWebsiteClick);
    connect(this, &NonGuiManager::closeWhatsRunning, w, &MainWindow::closeWhatsRunning);
    connect(this, &NonGuiManager::signalShowPreferences, w, &MainWindow::handlePreferencesAction);
    connect(this, &NonGuiManager::signalShowAbout, w, &MainWindow::handleAboutAction);
    connect(this, &NonGuiManager::signalQuit, w, &MainWindow::doQuit);
    connect(this, &NonGuiManager::signalDeleteCurrent, wallpaperManager_, &WallpaperManager::deleteCurrentBackgroundImage);
    connect(this, &NonGuiManager::signalAddFolderForMonitor, w, &MainWindow::addFolderForMonitor);
    connect(this, &NonGuiManager::signalFocus, w, &MainWindow::showNormal);
    connect(this, &NonGuiManager::signalHideOrShow, w, &MainWindow::hideOrShow);
    connect(w, &MainWindow::signalUncheckRunningFeatureOnTray, this, [this]() {Q_EMIT trayUncheckRequested();});
    connect(w, &MainWindow::signalRecreateTray, this, [this]() {Q_EMIT trayNeedsUpdate();});
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

void NonGuiManager::installTranslator(){
    if(QApplication::instance()){
        QTranslator *translator = new QTranslator(this);
        QString translationsFolder;

#ifdef Q_OS_LINUX
        translationsFolder = QString::fromStdString(PREFIX)+"/share/wallch/translations/";
#else
        translationsFolder = QDir::currentPath()+"/translations/";
#endif
        QString currentLanguage = settings->value("language_file", "system_default").toString();

        if(currentLanguage == "system_default")
            (void)translator->load(QLocale::system(), "wallch", "_", translationsFolder, ".qm");
        else
            (void)translator->load(translationsFolder+"wallch_"+currentLanguage+".qm");

        QApplication::installTranslator(translator);
    }
}

void NonGuiManager::viralSettingsOperations(){
    installTranslator();

    SettingsManager::initializeSettings();

#ifdef Q_OS_LINUX
    DesktopEnvironment::setCurrentDE();
#endif
}

void NonGuiManager::startProgramNormalGui(){
    connectToServer();

    Q_EMIT trayNeedsUpdate();
    mainWindowLaunched_=true;
    MainWindow *mainWindow = new MainWindow(alreadyRunsMem_,
                                            globalParser_,
                                            imageFetcher_,
                                            websiteSnapshot_,
                                            wallpaperManager_,
                                            dialogHelper_,
                                            timerManager_,
                                            featureController_);

    connectMainwindowWithExternalActions(mainWindow);
    mainWindow->show();
}

int NonGuiManager::processArguments(const QStringList &arguments)
{
    int argc = arguments.count();

    if(arguments.contains("--start")){
        if(alreadyRuns()){
            messageServer("--start", true);
            return 0;
        }

        bool gotPicLocation = wallpapersFeature_->getPicturesLocation(true);

        Global::resetSleepProtection(timerManager_->secondsRemaining_);
        if(gotPicLocation){
            wallpapersFeature_->getDelay();
            Global::debug("Your Desktop Background will change every "+QString::number(timerManager_->totalSeconds_)+" seconds.");

            if(wallpaperManager_->wallpapersCount()<LEAST_WALLPAPERS_FOR_START){
                Global::error("Too few pictures for image changing. You need at least " + QString::number(LEAST_WALLPAPERS_FOR_START) + ".");
                globalParser_->desktopNotify(tr("You cannot change wallpapers if they are less than 2!"), false, "info");
                return 1;
            }

            if(gv.randomImagesEnabled){
                wallpaperManager_->setRandomMode(true);
            }

            changeRunningFeature(FeatureController::Feature::Wallpapers);
        }
        connectToServer();
        Q_EMIT trayNeedsUpdate();

        if(gotPicLocation){
            timerManager_->start();
        }

        return 0;
    }
    else if(arguments.contains("--earth")){
        if(argc > 2){
            Global::error("Argument --earth doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--earth", true);
            return 0;
        }
        featureController_->setLaunchFeature(FeatureController::Feature::LiveEarth);
        changeRunningFeature(FeatureController::Feature::LiveEarth);
        timerManager_->secondsRemaining_ = LIVEARTH_INTERVAL;
        Global::resetSleepProtection(timerManager_->secondsRemaining_);

        connectToServer();
        Q_EMIT trayNeedsUpdate();

        liveEarthFeature_->start();

        return 0;
    }
    else if(arguments.contains("--potd")){
        if(argc > 2){
            Global::error("Argument --potd doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--potd", true);
            return 0;
        }
        featureController_->setLaunchFeature(FeatureController::Feature::PictureOfTheDay);
        changeRunningFeature(FeatureController::Feature::PictureOfTheDay);

        connectToServer();
        Q_EMIT trayNeedsUpdate();

        potdFeature_->start();

        return 0;
    }
    else if(arguments.contains("--website")){
        if(argc>2){
            Global::error("Argument --website doesn't take any other options!");
            showUsage(1);
        }
        if(alreadyRuns()){
            messageServer("--website", true);
            return 0;
        }
        featureController_->setLaunchFeature(FeatureController::Feature::Website);
        changeRunningFeature(FeatureController::Feature::Website);
        timerManager_->secondsRemaining_=0;
        Global::resetSleepProtection(timerManager_->secondsRemaining_);

        connectToServer();
        Q_EMIT trayNeedsUpdate();

        websiteSnapshot_ = new WebsiteSnapshot();

        websiteFeature_->start();
        return 0;
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
            return 1;
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
        featureController_->setLaunchFeature(FeatureController::Feature::None);

        connectToServer();
        Q_EMIT trayNeedsUpdate();

        return 0;
    }
    else if(arguments.contains("--focus")){
        if(alreadyRuns()){
            messageServer("--focus", true);
        }
        return 0;
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
                    return 0;
                }
                else
                {
                    //just set the folder as the default folder for monitoring
                    SettingsManager::setDefaultFolder(currentFolder);
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
        // First-level parsing for one-off commands that exit immediately
#ifdef Q_OS_WIN
        if(settings->value("startup_timeout", 3).toInt()!=0)
            Sleep(settings->value("startup_timeout", 3).toInt()*1000);
#endif

        const struct option longopts[] = {
            {"version", ARG_NOT_REQ, 0, 'v'},
            {"help", ARG_NOT_REQ, 0, 'h'},
            {"change", ARG_OPT, 0, 'c'},
            {0, 0, 0, 0},
        };

        int index;
        int iarg=0;

        opterr=0; // Turn off getopt error message
        optind=1; // Reset getopt for safe re-parsing

        while(iarg != -1)
        {
            iarg = getopt_long(argc, argv, "c:vh", longopts, &index);
            switch (iarg)
            {
            case 'h':
                showUsage(0);
                return 1;
            case 'v':
                Global::debug("Wallch - Wallpaper Changer, Version " + QString::number(APP_VERSION, 'f', 3));
                return 1;
            case 'c':
            {
                wallpapersFeature_->startedWithJustChange_=true;

                if(optarg){
                    if(index!=2 || argc > 3)
                        showUsage(1);

                    QString directoryToReadFrom=QString(optarg);
                    if(QDir(directoryToReadFrom).exists())
                        readPictures(QDir(directoryToReadFrom).absolutePath());
                    else{
                        Global::error(directoryToReadFrom+" - No such directory.");
                        return 1;
                    }
                }
                else {
                    if(!wallpapersFeature_->getPicturesLocation(true))
                        return 1;
                }

                wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
                return 1;
            }
            }
        }

        // Second-level parsing for long-running commands

        FeatureController::Feature launchFeature = featureController_->launchFeature();
        if ( (launchFeature == FeatureController::Feature::LiveEarth ||
             launchFeature == FeatureController::Feature::Website ||
             launchFeature != FeatureController::Feature::PictureOfTheDay) &&
            (settings->value("independent_interval_enabled", true).toBool()) )
        {
            setIndependentInterval(settings->value("seconds_left_interval_independence", INTERVAL_INDEPENDENCE_DEFAULT_VALUE).toString());
        }

        // processArguments will parse the remaining arguments and start the correct feature.
        return processArguments(QCoreApplication::arguments());

    }
    else{
        // No arguments were provided, start the full GUI

        if(alreadyRuns()){
            messageServer("--focus", true);
            return 0;
        }

        startProgramNormalGui();

        return 0;
    }
}

void NonGuiManager::startFeature(int featureId) {
    // Step 1: Handle the GUI-mode toggle
    if (mainWindowLaunched_) {
        bool isRunning = (featureId == 0 && featureController_->isWallpapersRunning() && !wallpapersFeature_->isPaused()) ||
                         (featureId == 1 && featureController_->isLiveEarthRunning()) ||
                         (featureId == 2 && featureController_->isPotdRunning()) ||
                         (featureId == 3 && featureController_->isWebsiteRunning());

        if (isRunning) {
            Q_EMIT closeWhatsRunning();
        } else {
            if (featureId == 0) Q_EMIT signalStart();
            else if (featureId == 1) Q_EMIT signalActivateLivearth();
            else if (featureId == 2) Q_EMIT signalActivatePotd();
            else if (featureId == 3) Q_EMIT signalActivateLiveWebsite();
        }
        return;
    }

    if (featureId == 0 && featureController_->isWallpapersRunning() && wallpapersFeature_->isPaused()) {
        doAction("--pause"); // This will un-pause it
        return;
    }

    // Step 2: Handle the command-line toggle
    bool wasRunning = (featureId == 0 && featureController_->isWallpapersRunning()) ||
                      (featureId == 1 && featureController_->isLiveEarthRunning()) ||
                      (featureId == 2 && featureController_->isPotdRunning()) ||
                      (featureId == 3 && featureController_->isWebsiteRunning());

    doAction("--stop");

    if (wasRunning) {
        Global::debug("Toggled feature off.");
        SettingsManager::updateStartup(featureController_->currentFeature());
        return;
    }

    // --- Step 3: Start the new feature
    switch (featureId) {
        case 0: changeRunningFeature(FeatureController::Feature::Wallpapers); break;
        case 1: changeRunningFeature(FeatureController::Feature::LiveEarth); break;
        case 2: changeRunningFeature(FeatureController::Feature::PictureOfTheDay); break;
        case 3: changeRunningFeature(FeatureController::Feature::Website); break;
    }

    if (featureId == 0) { // --start
        if (!wallpapersFeature_->setPaused(false)) {
            doAction("--stop");
        }
    } else if (featureId == 1) { // --earth
        liveEarthFeature_->start();
    } else if (featureId == 2) { // --potd
        potdFeature_->start();
    } else if (featureId == 3) { // --website
        websiteFeature_->start();
    }
}

void NonGuiManager::changeRunningFeature(FeatureController::Feature feature){
    featureController_->setCurrentFeature(feature);
    timerManager_->setCurrentFeature(feature);
}
