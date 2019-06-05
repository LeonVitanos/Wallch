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

#include "properties.h"
#include "preferences.h"
#include "mainwindow.h"
#include "about.h"
#include "glob.h"
#include "websitesnapshot.h"

#define SOCKET_SERVER_NAME "Wallch Local Socket Server"

class nonGUI : public QTimer {
  Q_OBJECT
public:
    explicit nonGUI(QObject *parent = 0) : QTimer(parent) {}
    QTimer indicatorScrollTimer_;

    int startProgram(int argc, char *argv[]);
    void doAction(const QString &message);

private:
    QFileSystemWatcher *watchFoldersMain_=NULL;
    Preferences *preferences_;
    QTimer researchFoldersTimerMain_;
    QTimer *updateSecondsPassed_;
    Properties *properties_;
    QSharedMemory *alreadyRunsMem_;
    QLocalServer *localServer_;
    QLocalSocket *socket_;
    QString messageToSendToServer_;
    WebsiteSnapshot *websiteSnapshot_=NULL;
    bool quitAfterMessage_;
    Global *globalParser_ = new Global(true);
    int totalSeconds_;
    int secondsLeft_;
    int currentImageIndex_;
    int wallpaperClocksTotalSeconds_;
    bool mainWindowLaunched_=false;
    bool startedWithJustChange_=false;
    bool startedWithLiveEarth_=false;
    bool startedWithPotd_=false;
    bool startedWithClock_=false;
    bool startedWithWebsite_=false;
    bool previousWasClicked_=false;
    bool wallpaperClocksMonthChecked_=false;
    bool wallpaperClocksDayOfMonthChecked_=false;
    bool wallpaperClocksDayOfWeekChecked_=false;
    bool wallpaperClocksAmPmChecked_=false;
    bool wallpaperClocksHourChecked_=false;
    bool wallpaperClocksMinutesChecked_=false;
    bool propertiesShown_=false;
    bool justUpdatedPotd_=false;
    QStringList previousPictures_;
    QStringList allPictures_;
#ifdef ON_LINUX
    DbusmenuMenuitem *unityMenu_;
    GtkWidget *showAppOption_, *deleteCurrentImageOption_, *showPreferencesOption_, *showAboutOption_, *quitAppOption_,
              *copyPathOfCurImageOption_, *copyNameOfCurImageOption_, *openPathOfCurImageOption_, *deleteCurImageOption_,
              *propertiesOfCurImageOption_;

#endif //#ifdef ON_LINUX

#ifdef ON_WIN32
    QSystemTrayIcon *trayIcon_;
    QMenu *trayIconMenu_;
    QMenu *currentImageMenu_;
    QAction *showWindowAction_;
    QAction *copyCurrentImagePathAction_;
    QAction *copyCurrentImageNameAction_;
    QAction *copyCurrentImageAction_;
    QAction *openCurrentImageFolderAction_;
    QAction *deleteCurrentImageAction_;
    QAction *openCurrentImagePropertiesAction_;
    QAction *wallpapersAction_;
    QAction *wallpapersOnceAction_;
    QAction *wallpapersPauseAction_;
    QAction *wallpapersNextAction_;
    QAction *wallpapersPreviousAction_;
    QAction *liveEarthAction_;
    QAction *pictureOfTheDayAction_;
    QAction *wallpaperClocksAction_;
    QAction *liveWebsiteAction_;
    QAction *preferencesAction_;
    QAction *aboutAction_;
    QAction *quitAction_;
    QTimer *doubleClick_;
    void setupTray();
#else
    void setupIndicator();
    void updatePotdProgressMain();
    void setUnityShortcutsState(bool stopState, bool pauseState, bool nextState, bool previousState);
    void setupUnityShortcuts();
#endif
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
    void fixCacheSizeGlobalCallback();
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
    bool continueWithClock();
    void continueWithPotd(bool checkInternetConnection);
    void changeWallpaperNow();
    void getDelay();
    void readPictures(const QString &folder);
    void potdSetSameImage();
    void startStatisticsTimer();

private Q_SLOTS:
    void newSocketConnection();
    void dirChanged();
    void researchDirs();
#ifdef ON_LINUX
    void unityProgressbarSetEnabled(bool enabled);
    void indicatorBackToNormal();
    void changeIndicatorIcon(QString theme);
#else
    void trayActionShowWindow();
    void trayActionCopyPath();
    void trayActionCopyName();
    void trayActionCopyImage();
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
    void trayActionWallpaperClocks();
    void trayActionLiveWebsite();
    void trayActionPreferences();
    void trayActionAbout();
    void trayActionQuit();
    void createTray();
    void uncheckRunningFeatureOnTray();
    void trayActivatedActions(QSystemTrayIcon::ActivationReason reason);
#endif //#ifdef ON_LINUX
    void socketConnected();
    void socketError();
    void quitNow();
    void waitForInternetConnection();
    void updateSeconds();
    void checkPicOfDay();
    void propertiesDestroyed();
    void preferencesDestroyed();
    void fixCacheSizeThreaded();
    void liveWebsiteImageReady(QImage *image, short errorCode);
    void updateSecondsPassed();

Q_SIGNALS:
    void signalOnce();
    void signalPause();
    void signalNext();
    void signalPrevious();
    void signalFocus();
    void signalStart();
    void signalActivateLivearth();
    void signalActivatePotd();
    void signalActivateWallClocks();
    void signalActivateLiveWebsite();
    void closeWhatsRunning();
    void signalShowPreferences();
    void signalShowAbout();
    void signalDeleteCurrent();
    void signalInstallWallClock(QString wallClock);
    void signalAddFolderForMonitor(const QString &folder);
    void signalQuit();
};

extern nonGUI nongui;

#endif // NONGUI_H
