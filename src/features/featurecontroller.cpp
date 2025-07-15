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
#include "websitefeature.h"
#include "potdfeature.h"
#include "settingsmanager.h"

FeatureController::FeatureController(WallpapersFeature *wallpapersFeature,
                                     LiveEarthFeature *liveEarthFeature,
                                     WebsiteFeature *websiteFeature,
                                     PotdFeature *potdFeature,
                                     QObject *parent)
    : QObject(parent)
    , m_wallpapersFeature(wallpapersFeature)
    , m_liveEarthFeature(liveEarthFeature)
    , m_websiteFeature(websiteFeature)
    , m_potdFeature(potdFeature)
{
    connect(m_wallpapersFeature, &WallpapersFeature::pausedStateChanged, this, &FeatureController::pausedStateChanged);
}

void FeatureController::launchFeature(FeatureController::Feature feature)
{
    switch (feature) {
    case Feature::Wallpapers:
        // setPaused(false) will initialize and start the timer if needed.
        if (!m_wallpapersFeature->setPaused(false)) {
            Q_EMIT launchFailed();
        }
        break;
    case Feature::LiveEarth:
        m_liveEarthFeature->start();
        break;
    case Feature::PictureOfTheDay:
        m_potdFeature->start();
        break;
    case Feature::Website:
        m_websiteFeature->start();
        break;
    case Feature::None:
        break; // Do nothing
    }
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
        m_websiteFeature->start();
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

void FeatureController::setLaunchFeature(FeatureController::Feature feature)
{
    m_launchFeature = feature;
}

FeatureController::Feature FeatureController::getLaunchFeature() const
{
    return m_launchFeature;
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

void FeatureController::onPersistencePrefixNeeded(QChar &prefix) const
{
    switch (m_currentFeature) {
    case Feature::Wallpapers:
        prefix = 'p';
        break;
    case Feature::LiveEarth:
        prefix = 'e';
        break;
    case Feature::Website:
        prefix = 'w';
        break;
    default:
        break;
    }
}
