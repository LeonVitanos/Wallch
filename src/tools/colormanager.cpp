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

#include "colormanager.h"
#include "settingsmanager.h"

#include <QPainter>
#include <QPalette>

#ifdef Q_OS_LINUX
    #include "desktopenvironment.h"
    #include "desktopenvironment_kde.h"
#endif

ColoringType::Value currentShading = ColoringType::Solid;

ColorManager::ColorManager(){}

QString ColorManager::getPrimaryColor(){
#ifdef Q_OS_LINUX
    if(currentDE == DE::LXDE)
        return DesktopEnvironment::getPcManFmValue("desktop_bg");
    else
        return getColor(1);
#else
# ifdef Q_OS_WIN
    QSettings collorSetting("HKEY_CURRENT_USER\\Control Panel\\Colors", QSettings::NativeFormat);
    if(collorSetting.value("Background").isValid()){
        QString primaryColor = collorSetting.value("Background").toString();
        QList<int> rgb;
        QString temp;
        for(short i=0; primaryColor.size() > i; i++){
            if(i==primaryColor.size()-1){
                temp.append(primaryColor.at(i));
                rgb.append(temp.toInt());
            }
            else if(primaryColor.at(i)==' '){
                rgb.append(temp.toInt());
                temp.clear();
            }
            else
                temp.append(primaryColor.at(i));
        }
        return QColor(rgb.at(0), rgb.at(1), rgb.at(2)).name();
    }
    else{
        int Elements[1] = {COLOR_BACKGROUND};
        DWORD currentColors[1];
        currentColors[0] = GetSysColor(Elements[0]);
        return QColor(GetRValue(currentColors[0]), GetGValue(currentColors[0]), GetBValue(currentColors[0])).name();
    }
# endif
#endif
    return "";
}

void ColorManager::setPrimaryColor(const QString &colorName){
#ifdef Q_OS_LINUX
    if(currentDE == DE::LXDE)
        DesktopEnvironment::setPcManFmValue("desktop_bg", colorName);
    else
        setColor(1, colorName);
#else
# ifdef Q_OS_WIN
    QColor color = colorName;
    QSettings color_setting("HKEY_CURRENT_USER\\Control Panel\\Colors", QSettings::NativeFormat);
    color_setting.setValue("Background", QString::number(color.red())+' '+QString::number(color.green())+' '+QString::number(color.blue()));
    int Elements[1] = {COLOR_BACKGROUND};
    DWORD NewColors[1];
    NewColors[0] = RGB(color.red(), color.green(), color.blue());
    SetSysColors(1, Elements, NewColors);
# else
#  ifdef Q_OS_MAC
    (void) colorName;
#  endif
# endif
#endif
}


QString ColorManager::getSecondaryColor(){
#ifdef Q_OS_LINUX
    return getColor(2);
#else
    return settings->value("secondaryColor", "black").toString();
#endif
}


void ColorManager::setSecondaryColor(const QString &colorName){
#ifdef Q_OS_LINUX
    setColor(2, colorName);
#else
    settings->setValue("secondaryColor", colorName);
#endif
}

#ifdef Q_OS_LINUX
QString ColorManager::getColor(short num){
    if(currentDE == DE::Gnome || currentDE == DE::Mate)
        return DesktopEnvironment::gsettingsGet("org.gnome.desktop.background", num == 1 ? "primary-color" : "secondary-color");
    else if(currentDE == DE::XFCE){
        QString color;
        DesktopEnvironment::processXfconfQuery({"color"+QString(num)}, [&](const QString &entry) {
            QStringList colors = DesktopEnvironment::runXfCommand(false, QStringList() << entry);
            QList<int> rgbColors;
            Q_FOREACH(QString color, colors){
                bool ok=false;
                int currentColor=color.toInt(&ok);
                if(!ok)
                    continue;
                rgbColors.append(int((256.0*currentColor)/65535.0));
            }
            if(rgbColors.count()!=4)
                return true;
            QColor finalColor;
            finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
            color = finalColor.name();
            return false;
        });
        return color;
    }
    else if(currentDE == DE::KDE)
        return desktopenvironment_kde::getKdeColor();

    return "black";
}

void ColorManager::setColor(short num, QString colorName){
    if(currentDE == DE::Gnome ||currentDE == DE::Mate)
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", num == 1 ? "primary-color" : "secondary-color", colorName);
    else if(currentDE == DE::XFCE){
        QStringList colorValues;
        QColor color=QColor(colorName);
        colorValues.append(QString::number(int((65535.0/256.0)*color.red())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.green())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.blue())));
        colorValues.append("65535");
        DesktopEnvironment::processXfconfQuery({"color"+QString(num)}, [&](const QString &entry) {
            DesktopEnvironment::runXfconf(QStringList() << entry << "-t" << "uint" << "-s" << colorValues.at(0)
                                          << "-t" << "uint" << "-s" << colorValues.at(1)
                                          << "-t" << "uint" << "-s" << colorValues.at(2)
                                          << "-t" << "uint" << "-s" << colorValues.at(3));
            return false;
        });
    }
    else if(currentDE == DE::KDE){
        desktopenvironment_kde::setKdeColor(colorName);
    }
}
#endif

ColoringType::Value ColorManager::getColoringType(){
#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        QString colorStyle=DesktopEnvironment::gsettingsGet("org.gnome.desktop.background", "color-shading-type");
        if(colorStyle.contains("solid"))
            return ColoringType::Solid;
        else if(colorStyle.contains("vertical"))
            return ColoringType::Vertical;
        else if(colorStyle.contains("horizontal"))
            return ColoringType::Horizontal;
    }
    else if(currentDE == DE::XFCE){
        ColoringType::Value result = ColoringType::Solid;
        DesktopEnvironment::processXfconfQuery({"color-style"}, [&](const QString &entry) {
            QString colorStyle = DesktopEnvironment::runXfCommand(false, QStringList() << entry, true).at(0);
            if(colorStyle=="0" || colorStyle=="3")
                result = ColoringType::Solid;
            else if(colorStyle=="1")
                result = ColoringType::Horizontal;
            else if(colorStyle=="2")
                result = ColoringType::Vertical;
            return false;
        });
        return result;
    }
    else if(currentDE == DE::LXDE)
        return ColoringType::Solid;
#else
    QString res = settings->value("ShadingType", "solid").toString();

    if(res == "solid")
        return ColoringType::Solid;
    else if(res == "vertical")
        return ColoringType::Vertical;
    else if(res == "horizontal")
        return ColoringType::Horizontal;
#endif

    return ColoringType::Solid;
}


void ColorManager::changeCurrentShading(){
    currentShading = getColoringType();
}


QImage ColorManager::createVerticalHorizontalImage(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);

    QLinearGradient gradient;
    if(currentShading == ColoringType::Horizontal){
        gradient.setStart(0, height / 2);
        gradient.setFinalStop(width, height / 2);
    }
    else {
        gradient.setStart(width / 2, 0);
        gradient.setFinalStop(width / 2, height);
    }
    gradient.setColorAt(0, QColor(getPrimaryColor()));
    gradient.setColorAt(1, QColor(getSecondaryColor()));

    QPainter painter;
    painter.begin(&image);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width, height);
    painter.drawImage(QRect(0, 0, width, height), image);

    return image;
}

short ColorManager::getCurrentTheme(){
    QString theme;
    if(settings->value("theme") != "autodetect")
        theme = settings->value("theme").toString();
    else{
#ifdef Q_OS_LINUX
        if(currentDE == DE::Gnome)
            theme = DesktopEnvironment::gsettingsGet("org.gnome.desktop.interface", "gtk-theme");
        else
            return 1;
#elif defined(Q_OS_WIN)
        QSettings appsUseLightTheme("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
        if(appsUseLightTheme.value("AppsUseLightTheme").isValid())
            return appsUseLightTheme.value("AppsUseLightTheme").toInt();
        else{
            QSettings currentTheme("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes", QSettings::NativeFormat);
            if(currentTheme.value("CurrentTheme").isValid())
                theme = currentTheme.value("CurrentTheme").toString();
            else
                return 1;
        }
#elif defined(Q_OS_MAC)
        if (QOperatingSystemVersion::current().majorVersion() >= 10) {
            return QPalette().color(QPalette::Window).lightness() >= 128;
        }
        else
            return 1;
#else
        return 1;
#endif
    }

    return !(theme.contains("dark", Qt::CaseInsensitive) || theme.contains("ambiance", Qt::CaseInsensitive));
}
