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

#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <QObject>
#include <QTimer>

#include "featurecontroller.h"

class TimerManager : public QObject
{
    Q_OBJECT

public:
    explicit TimerManager(QObject *parent = 0);
    void findSeconds(bool typeCountSeconds);
    qint64 secondsRemaining_;
    int totalSeconds_;
    int secondsInWallpapersSlider_;
    static QString secondsToMinutesHoursDays(int seconds);
    QList<int> defaultIntervals;
    QString secondsToMh(int seconds);
    QString secondsToHms(int seconds);
    QString secondsToHm(int seconds);
    void start(bool potd=false);
    void stop();
    void resetTimer();
    void saveSecondsLeftNow(bool keepIndependentInterval=true);
    void tryRestoreState(FeatureController::Feature launchFeature);

    bool isActive() const;

private:
    QTimer *m_internalTimer;
    void resetSecondsRemaining();
    void handleTimerExpiry();

Q_SIGNALS:
    void timeToChangeWallpaper();
    void persistencePrefixNeeded(QChar &prefix);

private Q_SLOTS:
    void updateSeconds();
};

#endif // TIMERMANAGER_H
