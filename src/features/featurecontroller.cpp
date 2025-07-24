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
#include "glob.h"

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

void FeatureController::toggleFeature(FeatureController::Feature feature)
{
    const bool wasRunningThisFeature = (m_currentFeature == feature) && !isFeaturePaused();

    // Stop whatever is currently running
    if (m_currentFeature != Feature::None) {
        Feature featureToStop = m_currentFeature;
        if (isWallpapersRunning()) {
            m_wallpapersFeature->setPaused(true);
        } else if (isLiveEarthRunning()) {
            m_liveEarthFeature->stop();
        } else if (isPotdRunning()) {
            m_potdFeature->stop();
        } else if (isWebsiteRunning()) {
            m_websiteFeature->stop();
        }
        setCurrentFeature(Feature::None);
        Q_EMIT featureStopped(featureToStop);
    }

    if (!wasRunningThisFeature) {
        // If we were running something else (or nothing), start the new feature.
        setCurrentFeature(feature);
        launchFeature(feature);
        Q_EMIT featureToggled(feature);
        Global::debug("Toggled feature on.");
    }
}

void FeatureController::launchFeature(FeatureController::Feature feature)
{
    bool success = true;
    switch (feature) {
    case Feature::Wallpapers:
        success = m_wallpapersFeature->setPaused(false);
        break;
    case Feature::LiveEarth:
        m_liveEarthFeature->start();
        // TODO: return a bool on success/failure
        break;
    case Feature::PictureOfTheDay:
        m_potdFeature->start();
        // TODO: return a bool on success/failure
        break;
    case Feature::Website:
        m_websiteFeature->start();
        // TODO: return a bool on success/failure
        break;
    case Feature::None:
        break;
    }

    if (!success) {
        Feature failedFeature = m_currentFeature;
        setCurrentFeature(Feature::None);
        Q_EMIT featureStopped(failedFeature);
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
