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

#ifndef NONGUI_H
#define NONGUI_H

#define QT_NO_KEYWORDS

#include <QTimer>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QObject>
#include <QPixmap>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QLocalServer>
#include <QDesktopServices>
#include <QtGlobal>
#include <QSystemTrayIcon>

#include "properties.h"
#include "preferences.h"
#include "mainwindow.h"
#include "about.h"
#include "glob.h"
#include "websitesnapshot.h"
#include "wallpapermanager.h"
#include "imagefetcher.h"
#include "timermanager.h"

#define SOCKET_SERVER_NAME "Wallch Local Socket Server"

class NonGuiManager : public QObject {
  Q_OBJECT
public:
    explicit NonGuiManager(QObject *parent = 0) : QObject(parent) {
        generalTimer_ = new QTimer(this);
    }
    int startProgram(int argc, char *argv[]);
    void doAction(const QString &message);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    WallpaperManager *wallpaperManager_ = NULL;
    TimerManager *timerManager_ = NULL;
    QFileSystemWatcher *watchFoldersMain_ = NULL;
    Preferences *preferences_;
    QTimer researchFoldersTimerMain_;
    QTimer *updateSecondsPassed_;
    QTimer *trayWheelTimer_;
    QTimer *generalTimer_ = NULL;
    QMimeData *myFile_;
    Properties *properties_;
    QSharedMemory *alreadyRunsMem_;
    QLocalServer *localServer_;
    QLocalSocket *socket_;
    QString messageToSendToServer_;
    WebsiteSnapshot *websiteSnapshot_=NULL;
    bool quitAfterMessagingMainApplication_;
    Global *globalParser_ = NULL;
    ImageFetcher *imageFetcher_ = NULL;
    bool mainWindowLaunched_ = false;
    bool startedWithNone_ = false;
    bool startedWithJustChange_ = false;
    bool startedWithLiveEarth_ = false;
    bool startedWithPotd_ = false;
    bool startedWithWebsite_ = false;
    bool previousWasClicked_ = false;
    bool propertiesShown_ = false;
    bool justUpdatedPotd_ = false;
    bool currentFolderIsAList_ = false;
#ifdef UNITY
    DbusmenuMenuitem *unityMenu_;
#endif

    QSystemTrayIcon *trayIcon_;
    QMenu *trayIconMenu_;
    QMenu *currentImageMenu_;
    QAction *showWindowAction_;
    QAction *openCurrentImageAction_;
    QAction *openCurrentImageFolderAction_;
    QAction *copyCurrentImagePathAction_;
    QAction *copyCurrentImageNameAction_;
    QAction *copyCurrentImageAction_;
    QAction *deleteCurrentImageAction_;
    QAction *openCurrentImagePropertiesAction_;
    QAction *wallpapersAction_;
    QAction *wallpapersOnceAction_;
    QAction *wallpapersPauseAction_;
    QAction *wallpapersNextAction_;
    QAction *wallpapersPreviousAction_;
    QAction *liveEarthAction_;
    QAction *pictureOfTheDayAction_;
    QAction *liveWebsiteAction_;
    QAction *preferencesAction_;
    QAction *aboutAction_;
    QAction *quitAction_;
    QTimer *doubleClick_;
    void setupTray();

#ifdef UNITY
    void updatePotdProgressMain();
    void setUnityShortcutsState(bool stopState, bool pauseState, bool nextState, bool previousState);
    void setupUnityShortcuts();
#endif
    bool currentSelectionIsASet();
    void viralSettingsOperations();
    bool getPicturesLocation(bool init);
    bool alreadyRuns();
    void getFilesFromFolder(const QString &path);
    void messageServer(const QString &message, bool quitAfterwards);
    void actionsOnWallpaperChange();
    void setIndependentInterval(const QString &independentInterval);
    void checkSettings(bool allSettings);
    void connectMainwindowWithExternalActions(MainWindow *w);
    void resetWatchFolders(bool justDelete);
    void setDefaultFolderInSettings(const QString &folder);
    QString getCurrentWallpapersFolder();
    std::string percentToProgressbar(short percentage);
    void clearCurrentLine(short previousOutputCount);
    bool loadWebsiteSnapshotPlugin();
    void connectToServer();
    void connectToUpdateSecondsSlot();
    void disconnectFromSlot();
    void connectToCheckInternet();
    void continueWithLiveEarth();
    void continueWithWebsite();
    void continueWithPotd();
    void changeWallpaperNow();
    void getDelay();
    void readPictures(const QString &folder);
    void potdSetSameImage();
    void startStatisticsTimer();
    int processArguments(QApplication *app, QStringList arguments);
    void startProgramNormalGui();

private Q_SLOTS:
    void newSocketConnection();
    void dirChanged();
    void researchDirs();
    void onlineBackgroundReady(QString image);
#ifdef UNITY
    void unityProgressbarSetEnabled(bool enabled);
#endif //#ifdef Q_OS_UNIX

    void trayActionShowWindow();
    void trayActionCopyPath();
    void trayActionCopyImage();
    void trayActionOpenCurrentImage();
    void trayActionOpenCurrentImageFolder();
    void trayActionDeleteCurrentImage();
    void trayActionOpenCurrentImageProperties();
    void trayActionWallpapers();
    void trayActionWallpapersOnce();
    void trayActionWallpapersPause();
    void trayActionWallpapersNext();
    void trayActionWallpapersPrevious();
    void trayActionLiveEarth();
    void trayActionPictureOfTheDay();
    void trayActionLiveWebsite();
    void trayActionPreferences();
    void trayActionAbout();
    void trayActionQuit();
    void createTray();
    void uncheckRunningFeatureOnTray();
    void trayActivatedActions(QSystemTrayIcon::ActivationReason reason);
    void socketConnected();
    void socketError();
    void quitNow();
    void waitForInternetConnection();
    void updateSeconds();
    void checkPicOfDay();
    void propertiesDestroyed();
    void preferencesDestroyed();
    void liveWebsiteImageReady(QImage *image, short errorCode);
    void updateSecondsPassed();

Q_SIGNALS:
    void signalOnce();
    void signalPause();
    void signalNext();
    void signalPrevious();
    void signalFocus();
    void signalHideOrShow();
    void signalStart();
    void signalActivateLivearth();
    void signalActivatePotd();
    void signalActivateLiveWebsite();
    void closeWhatsRunning();
    void signalShowPreferences();
    void signalShowAbout();
    void signalDeleteCurrent();
    void signalAddFolderForMonitor(const QString &folder);
    void signalQuit();
};

extern NonGuiManager *nongui;

#endif // NONGUI_H
