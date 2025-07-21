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
#include "featurecontroller.h"

class WallpaperManager;
class Properties;
class Preferences;
class About;

class DialogHelper : public QObject
{
    Q_OBJECT

public:
    explicit DialogHelper(WallpaperManager *wallpaperManager,
                          FeatureController *featureController,
                          QObject *parent = nullptr);

public Q_SLOTS:
    void showPropertiesDialog(int currentIndex = -1, const QString &filePath = QString());
    void showPreferencesDialog();
    void showAboutDialog();

private Q_SLOTS:
    void propertiesDestroyed();
    void preferencesDestroyed();
    void aboutDestroyed();

private:
    WallpaperManager *m_wallpaperManager;
    FeatureController *m_featureController;

    Properties *m_properties = nullptr;
    Preferences *m_preferences = nullptr;
    About *m_about = nullptr;

Q_SIGNALS:
    void preferencesDialogCreated(Preferences* dialog);
};

#endif // DIALOGHELPER_H
