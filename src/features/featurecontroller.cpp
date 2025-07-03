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

#include "featurecontroller.h"
#include "wallpapersfeature.h"
#include "liveearthfeature.h"
#include "settingsmanager.h"

FeatureController::FeatureController(WallpapersFeature *wallpapersFeature,
                                     LiveEarthFeature *liveEarthFeature,
                                     QObject *parent)
    : QObject(parent)
    , m_wallpapersFeature(wallpapersFeature)
    , m_liveEarthFeature(liveEarthFeature)
{
}

void FeatureController::onTimeToChangeWallpaper()
{
    switch (m_currentFeature) {
    case FeatureController::Feature::Wallpapers:
        m_wallpapersFeature->changeWallpaperNow();
        break;
    case FeatureController::Feature::LiveEarth:
        m_liveEarthFeature->start();
        break;
    case FeatureController::Feature::Website:
        // m_websiteFeature->start();
        break;
    default:
        break;
    }
}

bool FeatureController::isWallpapersRunning() const
{
    return m_currentFeature == Feature::Wallpapers;
}

bool FeatureController::isLiveEarthRunning() const
{
    return m_currentFeature == Feature::LiveEarth;
}

bool FeatureController::isPotdRunning() const
{
    return m_currentFeature == Feature::PictureOfTheDay;
}

bool FeatureController::isWebsiteRunning() const
{
    return m_currentFeature == Feature::Website;
}

FeatureController::Feature FeatureController::currentFeature() const
{
    return m_currentFeature;
}

void FeatureController::setCurrentFeature(Feature newFeature)
{
    if (m_currentFeature == newFeature) {
        return;
    }

    m_currentFeature = newFeature;

    SettingsManager::updateStartup(m_currentFeature);
}

bool FeatureController::isFeaturePaused() const
{
    if (m_currentFeature == Feature::Wallpapers) {
        return m_wallpapersFeature->isPaused();
    }
    return false;
}

void FeatureController::setPaused(bool paused)
{
    if (m_currentFeature == Feature::Wallpapers) {
        m_wallpapersFeature->setPaused(paused);
    }
}


