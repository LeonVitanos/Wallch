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

#ifndef WEBSITEFEATURE_H
#define WEBSITEFEATURE_H

#include <QObject>

class TimerManager;
class WebsiteSnapshot;
class WallpaperManager;
class QImage;

class WebsiteFeature : public QObject
{
    Q_OBJECT
public:
    explicit WebsiteFeature(TimerManager *timerManager,
                            WallpaperManager *wallpaperManager,
                            QObject *parent = nullptr);

    void start();
    void stop();

public Q_SLOTS:
    void onImageReady(QImage *image, short errorCode);

private:
    TimerManager *m_timerManager;
    WallpaperManager *m_wallpaperManager;
};

#endif // WEBSITEFEATURE_H
