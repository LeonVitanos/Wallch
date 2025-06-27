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
}

void WallpapersFeature::getDelay()
{
    m_timerManager->totalSeconds_=settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();
}
