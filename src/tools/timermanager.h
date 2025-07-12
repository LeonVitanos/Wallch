
#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <QObject>
#include <QTimer>

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
