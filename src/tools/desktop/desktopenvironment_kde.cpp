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

#include "desktopenvironment.h"
#include "desktopenvironment_kde.h"
#include <QColor>
#include <QProcess>
#include <QDateTime>
#include "settingsmanager.h"

#ifdef Q_OS_LINUX
    #include <QtDBus/QDBusInterface>
    #include <QtDBus/QDBusMessage>
#endif

QString desktopenvironment_kde::s_cachedKdeGroup = "";

bool desktopenvironment_kde::setKdeWallpaper(const QString &image) {
    if (s_cachedKdeGroup.isEmpty()) {
        s_cachedKdeGroup = probeKdeGroup();
    }

    QString finalImage = image;

    if (image == DesktopEnvironment::getCurrentWallpaper()) {
        finalImage += "?t=" + QString::number(QDateTime::currentMSecsSinceEpoch());
    }

    int version = getKdeMajorVersion();
    QString script = getKdeScriptTemplate(version, finalImage);

    return !executeKdeScript(script).isNull();
}

int desktopenvironment_kde::getKdeMajorVersion() {
    QByteArray kVersion = qgetenv("KDE_SESSION_VERSION");
    if (!kVersion.isEmpty()) {
        return kVersion.toInt();
    }

    QDBusInterface interface("org.kde.plasmashell", "/MainApplication", "org.qtproject.Qt.LibraryInfo");
    if (interface.isValid()) {
        QString v = interface.property("version").toString();
        if (!v.isEmpty()) {
            return v.section('.', 0, 0).toInt();
        }
    }

    return 5;
}

QString desktopenvironment_kde::getKdeScriptTemplate(int version, const QString &image) {
    if (version >= 6) {
        // Modern ECMAScript (Plasma 6)
        return QString(
           "desktops().forEach(d => {"
           "    d.currentConfigGroup = ['%1', 'org.kde.image', 'General'];"
           "    d.writeConfig('Image', 'file://%2');"
           "    d.reloadConfig();"
           "})"
           ).arg(s_cachedKdeGroup, image);
    } else {
        // Legacy Javascript (Plasma 5)
        return QString(
           "var allDesktops = desktops();"
           "for (var i = 0; i < allDesktops.length; i++) {"
           "    var d = allDesktops[i];"
           "    d.currentConfigGroup = ['%1', 'org.kde.image', 'General'];"
           "    d.writeConfig('Image', 'file://%2');"
           "    d.reloadConfig();"
           "}"
           ).arg(s_cachedKdeGroup, image);
    }
}

QString desktopenvironment_kde::executeKdeScript(const QString &script) {
    QDBusInterface remoteApp("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    if (!remoteApp.isValid()) return "";

    QDBusMessage reply = remoteApp.call("evaluateScript", script);

    const QVariantList args = reply.arguments();
    QString result = args.isEmpty() ? "" : args.first().toString();

    return result;
}

QString desktopenvironment_kde::probeKdeGroup() {
    QString probeScript =
        "var d = desktops()[0];"
        "d.currentConfigGroup = ['Wallpapers', 'org.kde.image', 'General'];"
        "if (d.configKeys.length > 0) { 'Wallpapers'; } else { 'Wallpaper'; }";

    QDBusInterface remoteApp("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell");
    QDBusMessage reply = remoteApp.call("evaluateScript", probeScript);

    const QVariantList args = reply.arguments();
    QString result = args.isEmpty() ? "" : args.first().toString();
    return result.isEmpty() ? "Wallpaper" : result;
}

short desktopenvironment_kde::getKdeWallpaperStyle() {
    QString script = "var d = desktops()[0]; "
                     "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General']; "
                     "print(d.wallpaperPlugin + ':' + d.readConfig('FillMode'));";

    QString response = executeKdeScript(script).trimmed();
    QStringList parts = response.split(':');
    QString plugin = parts.value(0);
    QString fillResult = parts.value(1);

    if (plugin == "org.kde.color") return 0; // NoneStyle

    if (fillResult.isEmpty()) return 1; // Scaled and Cropped

    int mode = fillResult.toInt();
    if (mode == 0)      return 2; // Scaled
    else if (mode == 1) return 3; // Scaled, Keep Proportions
    else if (mode == 6) return 4; // Centered
    else if (mode == 3) return 5; // Tiled

    return 1;
}

int desktopenvironment_kde::setKdeWallpaperStyleWithTransition(short index) {
    int oldStyle = getKdeWallpaperStyle();

    bool success = setKdeWallpaperStyle(index);

    if (!success) return -1;

    bool wasColor = (oldStyle == 0);
    bool isColor = (index == 0);

    return (wasColor != isColor) ? 1 : 0;
}

bool desktopenvironment_kde::setKdeWallpaperStyle(short index) {
    if (s_cachedKdeGroup.isEmpty()) s_cachedKdeGroup = probeKdeGroup();

    QString plugin = "org.kde.image";
    int fillMode=1;

    switch (index) {
        case 0: plugin = "org.kde.color"; break; // None / Color
        case 1: fillMode = 2; break; // Scaled and Cropped
        case 2: fillMode = 0; break; // Scaled
        case 3: fillMode = 1; break; // Scaled, Keep Proportions
        case 4: fillMode = 6; break; // Centered
        case 5: fillMode = 3; break; // Tiled
    }

    QString script = QString(
        "var allDesktops = desktops();"
        "for (var i = 0; i < allDesktops.length; i++) {"
        "    var d = allDesktops[i];"
        "    d.wallpaperPlugin = '%1';"
        "    d.currentConfigGroup = ['%2', '%1', 'General'];"
        "    d.writeConfig('FillMode', %3);"
        "    d.reloadConfig();"
        "}"
        ).arg(plugin, s_cachedKdeGroup).arg(fillMode);

    return !executeKdeScript(script).isNull();
}

QString desktopenvironment_kde::getCurrentWallpaper() {
    QString script =
        "var d = desktops()[0];"
        "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];"
        "print(d.readConfig('Image'));";

    QString path = executeKdeScript(script).trimmed();

    if (path.startsWith("file://")) {
        path = path.mid(7);
    }

    return path;
}

void desktopenvironment_kde::setKdeColor(const QString &colorName, const bool isPrimary) {
    if(!isPrimary){
        settings->setValue("secondaryColor", colorName);
        return;
    }

    QColor color(colorName);
    QString val = QString("%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue());

    int currentStyle = getKdeWallpaperStyle();
    QString targetPlugin = (currentStyle == 0) ? "org.kde.color" : "org.kde.image";

    QString injectScript = QString(
                               "var all = desktops();"
                               "for (var i in all) {"
                               "    var d = all[i];"
                               "    d.currentConfigGroup = ['Wallpaper', '%1', 'General'];"
                               "    d.writeConfig('Color', '%2');"
                               "    d.reloadConfig();"
                               "}"
                               ).arg(targetPlugin, val);

    executeKdeScript(injectScript);

    int tempStyle = (currentStyle == 0) ? 1 : 0;
    setKdeWallpaperStyle(tempStyle);
    setKdeWallpaperStyle(currentStyle);
}

QString desktopenvironment_kde::getKdeColor(const bool isPrimary) {
    if(!isPrimary){
        return settings->value("secondaryColor", "#3daee9").toString();;
    }

    QString targetPlugin = (getKdeWallpaperStyle() == 0) ? "org.kde.color" : "org.kde.image";

    QString script = QString(
                         "var d = desktops()[0];"
                         "d.currentConfigGroup = ['Wallpaper', '%1', 'General'];"
                         "print(d.readConfig('Color'));"
                         ).arg(targetPlugin);

    QString response = executeKdeScript(script).trimmed();

    if (response.isEmpty()) {
        return "#3daee9";
    }

    QStringList parts = response.split(',');
    if (parts.size() >= 3) {
        QColor color(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
        return color.name();
    }

    return "#3daee9";
}
