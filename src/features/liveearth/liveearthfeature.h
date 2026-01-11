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

#ifndef LIVEARTHFEATURE_H
#define LIVEARTHFEATURE_H

#include <QObject>

class ImageFetcher;
class TimerManager;

class LiveEarthFeature : public QObject
{
    Q_OBJECT
public:
    explicit LiveEarthFeature(ImageFetcher *imageFetcher,
                              TimerManager *timerManager,
                              QObject *parent = nullptr);

    void start();
    void stop();

private:
    ImageFetcher *m_imageFetcher;
    TimerManager *m_timerManager;
};

#endif // LIVEARTHFEATURE_H
