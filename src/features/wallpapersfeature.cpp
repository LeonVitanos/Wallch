#include "wallpapersfeature.h"
#include "wallpapermanager.h"
#include "timermanager.h"
#include "settingsmanager.h"
#include "glob.h"


WallpapersFeature::WallpapersFeature(WallpaperManager *wallpaperManager,
                                     TimerManager *timerManager,
                                     QObject *parent)
    : QObject(parent)
    , m_wallpaperManager(wallpaperManager)
    , m_timerManager(timerManager)
{
}

void WallpapersFeature::start()
{
    getDelay();
    m_timerManager->secondsRemaining_ = 0;
    Global::resetSleepProtection(m_timerManager->secondsRemaining_);
    m_timerManager->start();
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
