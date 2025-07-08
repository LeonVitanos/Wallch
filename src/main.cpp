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

#include "nonguimanager.h"
#include "glob.h"
#include "settingsmanager.h"
#include "features/featurecontroller.h"
#include "features/wallpapersfeature.h"
#include "features/liveearthfeature.h"
#include "tools/timermanager.h"
#include "tools/wallpapermanager.h"
#include "tools/imagefetcher.h"
#include "tools/filemanager.h"
#include "tools/glob.h"
#include "tools/dialoghelper.h"
#include "tools/traymanager.h"
#include <QApplication>
#include <QLoggingCategory>

struct GlobalVar gv;

NonGuiManager *nongui;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Disable warnings about corrupt ICC color profiles in user images.
    QLoggingCategory::setFilterRules("qt.gui.icc.warning=false");

    // 1. Initialize core utilities that have no dependencies.
    SettingsManager::initializeSettings();

    // 2. Create the low-level, shared tools and managers.
    auto globalParser = new Global();
    auto timerManager = new TimerManager(&app);
    auto wallpaperManager = new WallpaperManager(&app);
    auto imageFetcher = new ImageFetcher(&app);
    auto fileManager = new FileManager(wallpaperManager);
    auto dialogHelper = new DialogHelper(wallpaperManager, &app);

    // 3. Create the individual feature handlers, injecting their dependencies.
    auto wallpapersFeature = new WallpapersFeature(wallpaperManager, timerManager, fileManager, &app);
    auto liveEarthFeature = new LiveEarthFeature(imageFetcher, timerManager, &app);

    // 4. Create the core controllers that depend on the feature handlers.
    auto featureController = new FeatureController(wallpapersFeature, liveEarthFeature, &app);
    auto trayManager = new TrayManager(featureController, wallpaperManager, dialogHelper, &app);

    // 5. Wire up the core components. The timer should tell the FeatureController when to act.
    QObject::connect(timerManager, &TimerManager::timeToChangeWallpaper,
                     featureController, &FeatureController::onTimeToChangeWallpaper);

    // 6. Create the main application manager and INJECT the dependencies.
    NonGuiManager nonGuiManager(featureController,
                                timerManager,
                                wallpaperManager,
                                imageFetcher,
                                fileManager,
                                dialogHelper,
                                globalParser,
                                wallpapersFeature,
                                liveEarthFeature);

    QObject::connect(trayManager, &TrayManager::featureActionRequested, &nonGuiManager, &NonGuiManager::doAction);
    QObject::connect(trayManager, &TrayManager::showMainWindowRequested, &nonGuiManager, [&nonGuiManager](){nonGuiManager.doAction("--focus");});
    QObject::connect(&nonGuiManager, &NonGuiManager::trayNeedsUpdate, trayManager, &TrayManager::updateMenu);
    QObject::connect(&nonGuiManager, &NonGuiManager::trayUncheckRequested, trayManager, &TrayManager::uncheckAllActions);
    trayManager->show();

    // 7. Tell the fully constructed manager to start the program.
    int exitCode = nonGuiManager.startProgram(argc, argv);

    // 8. Let Qt run its event loop.
    if (exitCode == 0) {
        return app.exec();
    } else {
        return exitCode;
    }
}
