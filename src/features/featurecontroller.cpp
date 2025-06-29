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
#include "glob.h"

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
    if (gv.wallpapersRunning) {
        m_wallpapersFeature->changeWallpaperNow();
    } else if (gv.liveEarthRunning) {
        m_liveEarthFeature->start();
    } else if (gv.liveWebsiteRunning) {
        // m_websiteFeature->start();
    }
}
