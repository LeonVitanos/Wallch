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

#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QColor>
#include <QOperatingSystemVersion>
#include <QImage>

#ifdef Q_OS_WIN
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>
#endif

struct ColoringType {
    enum Value {
        Solid, Horizontal, Vertical
    };
};

extern ColoringType::Value currentShading;

class ColorManager
{
public:
    ColorManager();
    static QString getPrimaryColor(bool forcePlainColor=false);
    static void setPrimaryColor(const QString &colorName, bool forcePlainColor=false);
    static QString getSecondaryColor();
    static void setSecondaryColor(const QString &colorName);
#ifdef Q_OS_LINUX
    static QString getColor(short num, bool forcePlainColor=false);
    static void setColor(short num, QString colorName, bool forcePlainColor=false);
#endif
    static ColoringType::Value getColoringType();
    static void changeCurrentShading();
    static QImage createVerticalHorizontalImage(int width, int height);
    static short getCurrentTheme();
};

#endif // COLORMANAGER_H
