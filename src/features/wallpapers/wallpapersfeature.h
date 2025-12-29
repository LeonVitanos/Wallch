/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2025, The Wallch Team.
 Original Authors (2010-2015): Alexandros Solanos, Leon Vitanos
 Modernization & New Code (2025-): Leon Vitanos

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef WALLPAPERSFEATURE_H
#define WALLPAPERSFEATURE_H

#include <QObject>

class WallpaperManager;
class TimerManager;
class FileManager;

class WallpapersFeature : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Stopped,
        Running,
        Paused
    };
    Q_ENUM(State)

    explicit WallpapersFeature(WallpaperManager *wallpaperManager,
                               TimerManager *timerManager,
                               FileManager *fileManager,
                               QObject *parent = nullptr);

    void getDelay();
    void changeWallpaperNow();
    bool getPicturesLocation(bool init);

    bool previousWasClicked_ = false;
    bool startedWithJustChange_ = false;

    bool start();
    void pause();
    void resume();
    void stop();
    State state() const;

Q_SIGNALS:
    void stateChanged(WallpapersFeature::State newState);

private:
    WallpaperManager *m_wallpaperManager;
    TimerManager *m_timerManager;
    FileManager *m_fileManager;

    State m_state = State::Stopped;
    void setState(State newState);
};

#endif // WALLPAPERSFEATURE_H
