
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

extern ColoringType::Value currentShading;

class ColorManager
{
public:
    ColorManager();
    static QString getPrimaryColor();
    static void setPrimaryColor(const QString &colorName);
    static QString getSecondaryColor();
    static void setSecondaryColor(const QString &colorName);
#ifdef Q_OS_UNIX
    static ColoringType::Value getColoringType();
#endif
    static void changeCurrentShading();
    static QImage createVerticalHorizontalImage(int width, int height);
};

#endif // COLORMANAGER_H
