/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2025, The Wallch Team.
 Original Authors (2010-2015): Alexandros Solanos, Leon Vitanos
 Modernization & New Code (2025-): Leon Vitanos

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#include "liveearthfeature.h"
#include "imagefetcher.h"
#include "timermanager.h"
#include "glob.h"

LiveEarthFeature::LiveEarthFeature(ImageFetcher *imageFetcher,
                                   TimerManager *timerManager,
                                   QObject *parent)
    : QObject(parent)
    , m_imageFetcher(imageFetcher)
    , m_timerManager(timerManager)
{
}

void LiveEarthFeature::start()
{
    m_timerManager->totalSeconds_ = LIVEARTH_INTERVAL;
    if (!gv.firstTimeout) {
        m_imageFetcher->setFetchType(FetchType::LE);
        m_imageFetcher->fetch();
    }
    m_timerManager->saveSecondsLeftNow();
    m_timerManager->start();
}

void LiveEarthFeature::stop()
{
    m_timerManager->secondsRemaining_ = 0;
    m_imageFetcher->abort();
    if (m_timerManager->isActive()) {
        m_timerManager->stop();
    }
    m_timerManager->saveSecondsLeftNow(false);
}
