
#include "colormanager.h"
#include <QPainter>

#ifdef Q_OS_LINUX
    #include "desktopenvironment.h"
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
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color"+QString(num))){
                QStringList colors = DesktopEnvironment::runCommand("xfconf-query", false, QStringList() << entry);
                QList<int> rgbColors;
                Q_FOREACH(QString color, colors){
                    bool ok=false;
                    int currentColor=color.toInt(&ok);
                    if(!ok)
                        continue;
                    rgbColors.append(int((256.0*currentColor)/65535.0));
                }
                if(rgbColors.count()!=4)
                    continue;
                QColor finalColor;
                finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
                return finalColor.name();
            }
        }
    }

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
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color"+QString(num)))
                DesktopEnvironment::runXfconf(QStringList() << entry << "-t" << "uint" << "-s" << colorValues.at(0)
                                                            << "-t" << "uint" << "-s" << colorValues.at(1)
                                                            << "-t" << "uint" << "-s" << colorValues.at(2)
                                                            << "-t" << "uint" << "-s" << colorValues.at(3));
        }
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
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color-style")){
                QString colorStyle=DesktopEnvironment::runCommand("xfconf-query", false, QStringList() << entry, true).at(0);
                if(colorStyle=="0" || colorStyle=="3")
                    return ColoringType::Solid;
                else if(colorStyle=="1")
                    return ColoringType::Horizontal;
                else if(colorStyle=="2")
                    return ColoringType::Vertical;
            }
        }
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
#else
# ifdef Q_OS_WIN
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
# endif
#endif
    }

    return !(theme.contains("dark", Qt::CaseInsensitive) || theme.contains("ambiance", Qt::CaseInsensitive));
}
