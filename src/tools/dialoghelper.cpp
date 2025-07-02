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
#include <QMessageBox>

// Save the wallpaperManager in the constructor
DialogHelper::DialogHelper(WallpaperManager *wallpaperManager, QObject *parent):
    QObject(parent),
    m_wallpaperManager(wallpaperManager)
{}

// The function is now much cleaner
void DialogHelper::showPropertiesDialog(int currentIndex, const QString &filePath)
{
    if(m_propertiesShown)
        return;

    // It uses its OWN member variable now, not a passed-in one
    QString imagePath = !filePath.isEmpty() ? filePath :
                            currentIndex > -1 ? m_wallpaperManager->allWallpapers_.at(currentIndex) : m_wallpaperManager->currentBackgroundWallpaper();

    if (imagePath.isEmpty())
        QMessageBox::warning(0, tr("Properties"), tr("Error fetching the image file path."));
    else if(WallpaperManager::imageIsNull(imagePath))
        QMessageBox::warning(0, tr("Properties"), "\"" + imagePath + "\" " + tr("maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
    else {
        m_propertiesShown=true;
        // Pass its own member variable to the Properties window
        m_properties = new Properties(currentIndex, m_wallpaperManager, filePath);
        m_properties->setModal(true);
        m_properties->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_properties, &Properties::destroyed, this, &DialogHelper::propertiesDestroyed);
        m_properties->show();
    }
}

void DialogHelper::propertiesDestroyed(){
    m_propertiesShown=false;
}
