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

class FeatureController;
class TimerManager;
class WallpaperManager;
class ImageFetcher;
class FileManager;
class Global;
class WallpapersFeature;
class LiveEarthFeature;
class WebsiteFeature;
class PotdFeature;

#include <QTimer>
#include <QSettings>
#include <QObject>
#include <QPixmap>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtGlobal>
#include <QTranslator>

#include "preferences.h"
#include "mainwindow.h"
#include "glob.h"
#include "websitesnapshot.h"
#include "wallpapermanager.h"
#include "imagefetcher.h"
#include "timermanager.h"
#include "dialoghelper.h"
#include "wallpapersfeature.h"
#include "liveearthfeature.h"
#include "featurecontroller.h"

#define SOCKET_SERVER_NAME "Wallch Local Socket Server"

class NonGuiManager : public QObject {
  Q_OBJECT
public:
    explicit NonGuiManager(FeatureController *featureController,
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
                         QObject *parent = nullptr);
    int startProgram(int argc, char *argv[]);
    void doAction(const QString &message);

private:
    FeatureController* featureController_;
    TimerManager *timerManager_;
    WallpaperManager *wallpaperManager_;
    ImageFetcher *imageFetcher_;
    FileManager *fileManager_;
    DialogHelper *dialogHelper_;
    Global *globalParser_;
    WallpapersFeature* wallpapersFeature_;
    LiveEarthFeature* liveEarthFeature_;
    WebsiteFeature *websiteFeature_;
    PotdFeature *potdFeature_;

    Preferences *preferences_;
    WebsiteSnapshot *websiteSnapshot_=NULL;

    QMimeData *myFile_;
    QSharedMemory *alreadyRunsMem_;
    QLocalServer *localServer_;
    QLocalSocket *socket_;
    QString messageToSendToServer_;
    bool quitAfterMessagingMainApplication_;
    bool mainWindowLaunched_ = false;
    void installTranslator();
    void viralSettingsOperations();
    bool alreadyRuns();
    void messageServer(const QString &message, bool quitAfterwards);
    void setIndependentInterval(const QString &independentInterval);
    void connectMainwindowWithExternalActions(MainWindow *w);
    bool loadWebsiteSnapshotPlugin();
    void connectToServer();
    void disconnectFromSlot();
    void changeWallpaperNow();
    void getDelay();
    void readPictures(const QString &folder);
    int processArguments(const QStringList &arguments);
    void startProgramNormalGui();
    void startFeature(int featureId);
    void changeRunningFeature(FeatureController::Feature feature);
    void launchFeatureById(int featureId);

private Q_SLOTS:
    void newSocketConnection();
    void onlineBackgroundReady(QString image);
    void socketConnected();
    void socketError();
    void quitNow();
    void waitForInternetConnection();
    void preferencesDestroyed();
    void liveWebsiteImageReady(QImage *image, short errorCode);

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
    void trayNeedsUpdate();
    void trayUncheckRequested();
};

extern NonGuiManager *nongui;

#endif // NONGUI_H
