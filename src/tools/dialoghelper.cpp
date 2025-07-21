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

#include "dialoghelper.h"
#include "wallpapermanager.h"
#include "properties.h"
#include "preferences.h"
#include "about.h"
#include <QMessageBox>

DialogHelper::DialogHelper(WallpaperManager *wallpaperManager,
                           FeatureController *featureController,
                           QObject *parent):
    QObject(parent),
    m_wallpaperManager(wallpaperManager),
    m_featureController(featureController)
{}

// The function is now much cleaner
void DialogHelper::showPropertiesDialog(int currentIndex, const QString &filePath)
{
    if(m_properties) {
        m_properties->raise();
        m_properties->activateWindow();
        return;
    }

    // It uses its OWN member variable now, not a passed-in one
    QString imagePath = !filePath.isEmpty() ? filePath :
                            currentIndex > -1 ? m_wallpaperManager->allWallpapers_.at(currentIndex) : m_wallpaperManager->currentBackgroundWallpaper();

    if (imagePath.isEmpty())
        QMessageBox::warning(0, tr("Properties"), tr("Error fetching the image file path."));
    else if(WallpaperManager::imageIsNull(imagePath))
        QMessageBox::warning(0, tr("Properties"), "\"" + imagePath + "\" " + tr("maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
    else {
        // Pass its own member variable to the Properties window
        m_properties = new Properties(currentIndex, m_wallpaperManager, filePath);
        m_properties->setModal(true);
        m_properties->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_properties, &Properties::destroyed, this, &DialogHelper::propertiesDestroyed);
        m_properties->show();
    }
}

void DialogHelper::propertiesDestroyed(){
    m_properties = nullptr;
}

void DialogHelper::showPreferencesDialog()
{
    if (m_preferences) {
        m_preferences->raise();
        m_preferences->activateWindow();
        return;
    }

    m_preferences = new Preferences(m_featureController);
    m_preferences->setModal(true);
    m_preferences->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_preferences, &Preferences::destroyed, this, &DialogHelper::preferencesDestroyed);
    Q_EMIT preferencesDialogCreated(m_preferences);
    m_preferences->show();
}

void DialogHelper::preferencesDestroyed()
{
    m_preferences = nullptr;
}

void DialogHelper::showAboutDialog()
{
    if (m_about) {
        m_about->raise();
        m_about->activateWindow();
        return;
    }

    m_about = new About();
    m_about->setModal(true);
    m_about->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_about, &About::destroyed, this, &DialogHelper::aboutDestroyed);
    m_about->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    m_about->show();
}

void DialogHelper::aboutDestroyed()
{
    m_about = nullptr;
}
