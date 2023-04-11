
#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#define QT_NO_KEYWORDS
#include <QObject>

#include "glob.h"

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

private:



};

#endif // TIMERMANAGER_H
