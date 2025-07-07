#include "wallpapersfeature.h"
#include "wallpapermanager.h"
#include "timermanager.h"
#include "settingsmanager.h"
#include "filemanager.h"
#include "glob.h"

WallpapersFeature::WallpapersFeature(WallpaperManager *wallpaperManager,
                                     TimerManager *timerManager,
                                     FileManager *fileManager,
                                     QObject *parent)
    : QObject(parent)
    , m_wallpaperManager(wallpaperManager)
    , m_timerManager(timerManager)
    , m_fileManager(fileManager)
{
}

bool WallpapersFeature::initialize()
{
    if (m_isInitialized) {
        return true;
    }

    if (!getPicturesLocation(true)) {
        return false;
    }

    if (gv.randomImagesEnabled) {
        m_wallpaperManager->setRandomMode(true);
    }

    getDelay();
    m_timerManager->secondsRemaining_ = 0;
    Global::resetSleepProtection(m_timerManager->secondsRemaining_);

    m_isInitialized = true;
    return true;
}

void WallpapersFeature::getDelay()
{
    m_timerManager->totalSeconds_=settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();
}

void WallpapersFeature::changeWallpaperNow(){

    if(m_wallpaperManager->wallpapersCount() < LEAST_WALLPAPERS_FOR_START && !startedWithJustChange_){
        Global().notifyNotEnoughPics();
        return;
    }
    if(previousWasClicked_){
        previousWasClicked_=false;
        m_wallpaperManager->setBackground(m_wallpaperManager->getPreviousWallpaper(), true, true, 1);
    }
    else
    {
        QString image = m_wallpaperManager->getNextWallpaper();

        m_wallpaperManager->addToPreviousWallpapers(image);

        m_wallpaperManager->setBackground(image, true, true, 1);
    }
}

bool WallpapersFeature::getPicturesLocation(bool init)
{
    if (!startedWithJustChange_ && init)
        m_fileManager->resetWatchFolders();

    m_fileManager->picturesLocationChanged();

    if (gv.randomImagesEnabled) {
        m_wallpaperManager->setRandomMode(true);
    }

    // Check for errors
    if ( (startedWithJustChange_ && m_wallpaperManager->wallpapersCount() == 0) ||
        (!startedWithJustChange_ && m_wallpaperManager->wallpapersCount() < LEAST_WALLPAPERS_FOR_START) )
    {
        Global().notifyNotEnoughPics();
        return false;
    }

    return true;
}

bool WallpapersFeature::isPaused() const
{
    return m_isPaused;
}

bool WallpapersFeature::setPaused(bool paused)
{
    m_isPaused = paused;

    if (!m_isPaused) {
        if (!m_isInitialized) {
            if (!initialize()) {
                return false;
            }
        }
        m_timerManager->start();
    } else {
        m_timerManager->stop();
    }

    Q_EMIT pausedStateChanged(m_isPaused);
    return true;
}
