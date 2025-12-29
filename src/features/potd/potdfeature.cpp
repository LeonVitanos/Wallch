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

#include "potdfeature.h"
#include "imagefetcher.h"
#include "wallpapermanager.h"
#include "timermanager.h"
#include "glob.h"
#include "settingsmanager.h"
#include <QDateTime>

PotdFeature::PotdFeature(ImageFetcher *imageFetcher,
                         WallpaperManager *wallpaperManager,
                         TimerManager *timerManager,
                         QObject *parent)
    : QObject(parent)
    , m_imageFetcher(imageFetcher)
    , m_wallpaperManager(wallpaperManager)
    , m_timerManager(timerManager)
{
}

void PotdFeature::start()
{
    QString lastDaySet = settings->value("last_day_potd_was_set", "").toString();
    QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
    if (settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow != lastDaySet) {
        // The previous time that potd changed was another day, or settings have changed
        m_imageFetcher->setFetchType(FetchType::POTD);
        m_imageFetcher->fetch();
    } else {
        potdSetSameImage();
    }
    gv.doNotToggleRadiobuttonFallback = false;

    connect(m_timerManager, &TimerManager::timeToChangeWallpaper, this, &PotdFeature::checkPicOfDay);
    m_timerManager->start(true);
}

void PotdFeature::stop()
{
    m_timerManager->stop();
    disconnect(m_timerManager, &TimerManager::timeToChangeWallpaper, this, &PotdFeature::checkPicOfDay);
    m_imageFetcher->abort();
}

void PotdFeature::potdSetSameImage()
{
    Global::debug("Picture Of The Day should already be in your hard drive.");
    QString filename = Global::getFilename(gv.wallchHomePath + POTD_IMAGE + "*");

    if (filename.isEmpty()) {
        Global::error("Wallch will now attempt to download again the Picture Of The Day.");
        settings->setValue("last_day_potd_was_set", "");
        settings->setValue("previous_img_url", "retry");
        settings->sync();
        m_imageFetcher->setFetchType(FetchType::POTD);
        m_imageFetcher->fetch();
    }
    m_wallpaperManager->setBackground(filename, true, true, 3);
}

void PotdFeature::checkPicOfDay()
{
    if (Global::timeNowToString() == "00:00") {
        if (m_justUpdatedPotd) {
            return;
        }
        QString lastDaySet = settings->value("last_day_potd_was_set", "").toString();
        QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        if (settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow != lastDaySet) {
            m_justUpdatedPotd = true;
            m_imageFetcher->setFetchType(FetchType::POTD);
            m_imageFetcher->fetch();
        } else {
            // the day is the same, setting the same image.
            potdSetSameImage();
        }
    } else {
        m_justUpdatedPotd = false;
    }
}
