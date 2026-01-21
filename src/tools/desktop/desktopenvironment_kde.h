/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef DESKTOPENVIRONMENT_KDE_H
#define DESKTOPENVIRONMENT_KDE_H

#include <QString>

class desktopenvironment_kde
{
public:
    desktopenvironment_kde();
    static bool setKdeWallpaper(const QString &image);
    static short getKdeWallpaperStyle();
    static QString getCurrentWallpaper();
    static QString getKdeColor(const bool isPrimary, bool forcePlainColor=false);
    static void setKdeColor(const QString &colorName, const bool isPrimary, bool forcePlainColor=false);
    static int setKdeWallpaperStyleWithTransition(short index);

private:
    static QString s_cachedKdeGroup;
    static QString probeKdeGroup();
    static int getKdeMajorVersion();
    static QString executeKdeScript(const QString &script);
    static QString getKdeScriptTemplate(int version, const QString &image);
    static bool setKdeWallpaperStyle(short index);
};

#endif // DESKTOPENVIRONMENT_KDE_H
