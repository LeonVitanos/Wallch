
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
        Solid, Horizontal, Vertical
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
#ifdef Q_OS_LINUX
    static QString getColor(short num);
    static void setColor(short num, QString colorName);
#endif
    static ColoringType::Value getColoringType();
    static void changeCurrentShading();
    static QImage createVerticalHorizontalImage(int width, int height);
    static short getCurrentTheme();
};

#endif // COLORMANAGER_H
