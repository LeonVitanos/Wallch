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

#ifndef DIALOGHELPER_H
#define DIALOGHELPER_H

#include <QObject>

class WallpaperManager;
class Properties;

class DialogHelper : public QObject
{
    Q_OBJECT

public:
    explicit DialogHelper(WallpaperManager *wallpaperManager, QObject *parent = nullptr);

public Q_SLOTS:
    void showPropertiesDialog(int currentIndex = -1, const QString &filePath = QString());

private Q_SLOTS:
    void propertiesDestroyed();

private:
    WallpaperManager *m_wallpaperManager;

    Properties *m_properties = nullptr;
    bool m_propertiesShown = false;
};

#endif // DIALOGHELPER_H
