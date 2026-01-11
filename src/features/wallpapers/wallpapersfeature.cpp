/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

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

    m_wallpaperManager->setRandomMode(gv.randomImagesEnabled);

    // Check for errors
    if ( (startedWithJustChange_ && m_wallpaperManager->wallpapersCount() == 0) ||
        (!startedWithJustChange_ && m_wallpaperManager->wallpapersCount() < LEAST_WALLPAPERS_FOR_START) )
    {
        Global().notifyNotEnoughPics();
        return false;
    }

    return true;
}

WallpapersFeature::State WallpapersFeature::state() const
{
    return m_state;
}

bool WallpapersFeature::start()
{
    if (m_state != State::Stopped) {
        return true;
    }

    if (!getPicturesLocation(true)) {
        setState(State::Stopped);
        return false;
    }

    getDelay();
    Global::debug("Your Desktop Background will change every " + QString::number(m_timerManager->totalSeconds_) + " seconds.");
    m_timerManager->secondsRemaining_ = 0;

    Global::resetSleepProtection(m_timerManager->secondsRemaining_);
    m_timerManager->start();
    setState(State::Running);
    return true;
}

void WallpapersFeature::pause()
{
    if (m_state == State::Running) {
        m_timerManager->stop();
        setState(State::Paused);
    }
}

void WallpapersFeature::resume()
{
    if (m_state == State::Paused) {
        Global::resetSleepProtection(m_timerManager->secondsRemaining_);
        m_timerManager->start();
        setState(State::Running);
    }
}

void WallpapersFeature::stop()
{
    if (m_state != State::Stopped) {
        m_timerManager->stop();
        setState(State::Stopped);
        m_wallpaperManager->startOver();
    }
}

void WallpapersFeature::setState(State newState) {
    if (m_state == newState) return;
    m_state = newState;
    Q_EMIT stateChanged(m_state);
}
