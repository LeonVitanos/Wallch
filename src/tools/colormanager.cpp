
#include "colormanager.h"
#include <QPainter>

ColoringType::Value currentShading = ColoringType::Solid;

ColorManager::ColorManager(){}

QString ColorManager::getPrimaryColor(){
    QString primaryColor;
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::Mate)
        primaryColor = Global::gsettingsGet("org.gnome.desktop.background", "primary-color");
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color1")){
                QStringList colors=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).split("\n");
                QList<int> rgbColors;
                Q_FOREACH(QString color, colors){
                    bool ok=false;
                    int currentColor=color.toInt(&ok);
                    if(!ok){
                        continue;
                    }
                    rgbColors.append(int((256.0*currentColor)/65535.0));
                }
                if(rgbColors.count()!=4){
                    continue;
                }
                QColor finalColor;
                finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
                primaryColor = finalColor.name();
                break;
            }
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE)
        primaryColor = Global::getPcManFmValue("desktop_bg");
#else
    QSettings collorSetting("HKEY_CURRENT_USER\\Control Panel\\Colors", QSettings::NativeFormat);
    if(collorSetting.value("Background").isValid()){
        primaryColor = collorSetting.value("Background").toString();
        QList<int> rgb;
        QString temp;
        for(short i=0; primaryColor.size() > i; i++)
        {
            if(i==primaryColor.size()-1)
            {
                temp.append(primaryColor.at(i));
                rgb.append(temp.toInt());
            }
            else if(primaryColor.at(i)==' ')
            {
                rgb.append(temp.toInt());
                temp.clear();
            }
            else
            {
                temp.append(primaryColor.at(i));
            }
        }
        primaryColor = QColor(rgb.at(0), rgb.at(1), rgb.at(2)).name();
    }
    else{
        int Elements[1] = {COLOR_BACKGROUND};
        DWORD currentColors[1];
        currentColors[0] = GetSysColor(Elements[0]);
        primaryColor = QColor(GetRValue(currentColors[0]), GetGValue(currentColors[0]), GetBValue(currentColors[0])).name();
    }
#endif
    return primaryColor;
}

void ColorManager::setPrimaryColor(const QString &colorName){
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::Mate){
        Global::gsettingsSet("org.gnome.desktop.background", "primary-color", colorName);
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        QStringList colorValues;
        QColor color=QColor(colorName);
        colorValues.append(QString::number(int((65535.0/256.0)*color.red())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.green())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.blue())));
        colorValues.append("65535");
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color1")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry
                                                                      << "-t" << "uint" << "-s" << colorValues.at(0)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(1)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(2)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(3));
            }
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE){
        Global::setPcManFmValue("desktop_bg", colorName);
        QStringList pids=Global::getOutputOfCommand("pidof", QStringList() << "pcmanfm").replace("\n", "").split(" ");
        Q_FOREACH(QString pid, pids){
            QString output=Global::getOutputOfCommand("ps", QStringList() << "-fp" << pid);
            if(output.contains("--desktop")){
                QProcess::startDetached("kill", QStringList() << "-9" << pid);
            }
        }
        if(QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists()){
            QProcess::startDetached("pcmanfm", QStringList() << "--desktop" << "-p" << "lubuntu");
        }
        else
        {
            QProcess::startDetached("pcmanfm", QStringList() << "--desktop");
        }
    }
#else
    QColor color = colorName;
    QSettings color_setting("HKEY_CURRENT_USER\\Control Panel\\Colors", QSettings::NativeFormat);
    color_setting.setValue("Background", QString::number(color.red())+' '+QString::number(color.green())+' '+QString::number(color.blue()));
    int Elements[1] = {COLOR_BACKGROUND};
    DWORD NewColors[1];
    NewColors[0] = RGB(color.red(), color.green(), color.blue());
    SetSysColors(1, Elements, NewColors);
#endif
}


QString ColorManager::getSecondaryColor(){
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::Mate)
        return Global::gsettingsGet("org.gnome.desktop.background", "secondary-color");
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color2")){
                QStringList colors=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).split("\n");
                QList<int> rgbColors;
                Q_FOREACH(QString color, colors){
                    bool ok=false;
                    int currentColor=color.toInt(&ok);
                    if(!ok){
                        continue;
                    }
                    rgbColors.append(int((256.0*currentColor)/65535.0));
                }
                if(rgbColors.count()!=4){
                    continue;
                }
                QColor finalColor;
                finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
                return finalColor.name();
                break;
            }
        }
    }
#else
    return settings->value("secondaryColor", "black").toString();
#endif
}


void ColorManager::setSecondaryColor(const QString &colorName){
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome ||gv.currentDE == DesktopEnvironment::Mate)
        Global::gsettingsSet("org.gnome.desktop.background", "secondary-color", colorName);
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        QStringList colorValues;
        QColor color=QColor(colorName);
        colorValues.append(QString::number(int((65535.0/256.0)*color.red())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.green())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.blue())));
        colorValues.append("65535");
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color2")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry
                                                                      << "-t" << "uint" << "-s" << colorValues.at(0)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(1)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(2)
                                                                      << "-t" << "uint" << "-s" << colorValues.at(3));
            }
        }
    }
#else
    settings->setValue("secondaryColor", colorName);
#endif
}


ColoringType::Value ColorManager::getColoringType(){
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Mate){
        QString colorStyle=Global::gsettingsGet("org.gnome.desktop.background", "color-shading-type");
        if(colorStyle.contains("solid"))
            return ColoringType::Solid;
        else if(colorStyle.contains("vertical"))
            return ColoringType::Vertical;
        else if(colorStyle.contains("horizontal"))
            return ColoringType::Horizontal;
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color-style")){
                QString colorStyle=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).replace("\n", "");
                if(colorStyle=="0" || colorStyle=="3")
                    return ColoringType::Solid;
                else if(colorStyle=="1")
                    return ColoringType::Horizontal;
                else if(colorStyle=="2")
                    return ColoringType::Vertical;
            }
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE)
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
