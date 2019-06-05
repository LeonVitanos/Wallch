/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef WALLPAPERMANAGER_H
#define WALLPAPERMANAGER_H

#define QT_NO_KEYWORDS

#include <QStringList>

class WallpaperManager
{
public:
    WallpaperManager();

private:
    QStringList allWallpapers_;
    QStringList previousWallpapers_;
    static const short maxPreviousCount_=30;
    short currentPreviousCount_=0;
    short currentWallpaperIndex_=-1;
    short currentRandomWallpaperIndex_=0;
    QList<int> randomWallpapersRow_;
    bool randomMode_=true;

    short randomWallpaperIndexButNot(short indexToExclude);

public:
    void addToPreviousWallpapers(const QString &wallpaper);
    QString getPreviousWallpaper();
    QString getNextWallpaper();
    void setRandomMode(bool randomMode, short indexToExclude=-1, short indexToInclude =-1);
    void generateRandomImages(int firstOne=-1);
    QString randomButNotCurrentWallpaper();
    void setCurrentWallpapers(const QStringList &wallpapers);
    void addWallpaper(const QString &wallpaper);
    void addWallpapers(const QString &wallpapers);
    QStringList getCurrentWallpapers();
    void clearWallpapers();
    static QString currentBackgroundWallpaper();
    static void openFolderOf(QString image = "");
    short wallpapersCount();
    void convertRandomToNormal();
    bool randomModeEnabled();
    void startOver();
    short currentWallpaperIndex();
    void setRandomWallpaperAsBackground();
    void setNextToBe(short index);
    static void setBackground(const QString &image, bool changeAverageColor, bool showNotification, short feature);
    short indexOfCurrentBackgroundWallpaper();
};

#endif // WALLPAPERMANAGER_H
