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

#ifndef DESKTOPENVIRONMENT_H
#define DESKTOPENVIRONMENT_H

#include <QString>
#include <QStringList>

#ifdef Q_OS_LINUX
    struct DE {
        enum Value {
            Gnome, XFCE, LXDE, Mate, KDE
        };
    };

    extern DE::Value currentDE;
#endif

class DesktopEnvironment
{
public:
    DesktopEnvironment();
    static double getOSproductVersion();
    static QString getOSproductType();
    static QString getOSprettyName();
    static QString getOSWallpaperPath();

#ifdef Q_OS_LINUX
    static void detectCurrentDe();
    static QStringList runCommand(
        const QString &command,
        const QStringList &parameters = QStringList(),
        bool replaceNewLine = false,
        const QString &split = "\n"
    );

    // Gnome, Mate
    static QString gsettingsGet(const QString &schema, const QString &key);
    static void gsettingsSet(const QString &schema, const QString &key, const QString &value);
    static QString getPictureUriName();

    // LXDE
    static QString getPcManFmValue(const QString &key);
    static void setPcManFmValue(const QString &key, const QString &value);
    static bool runPcManFm(QStringList args);

    // XFCE
    static QStringList runXfCommand(bool backdrop = true,
                                    const QStringList &parameters = QStringList(),
                                    bool replaceNewLine = false);
    static void processXfconfQuery(const QStringList &keywordsToFind,
                                  std::function<bool(const QString&)> processor);
    static bool runXfconf(QStringList args);

    // KDE
    static bool setKdeWallpaper(const QString &image);
    static short getKdeWallpaperStyle();
    static bool setKdeWallpaperStyle(short index);
    static QString getCurrentWallpaper();
#endif
};

#endif // DESKTOPENVIRONMENT_H
