
#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QColor>
#include "glob.h"

#ifdef Q_OS_WIN
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>
#endif

struct ColoringType {
    enum Value {
        NoneColor, SolidColor, HorizontalColor, VerticalColor
    };
};

class ColorManager
{
public:
    ColorManager();
    static QString getPrimaryColor();
    static void setPrimaryColor(const QString &colorName);
#ifdef Q_OS_UNIX
    static QString getSecondaryColor();
    static void setSecondaryColor(const QString &colorName);
    static ColoringType::Value getColoringType();
#endif
};

#endif // COLORMANAGER_H
