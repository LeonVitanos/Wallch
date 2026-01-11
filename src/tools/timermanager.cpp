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

#include "timermanager.h"
#include "settingsmanager.h"
#include "glob.h"

TimerManager::TimerManager(QObject *parent):
    QObject(parent)
{
    m_internalTimer = new QTimer(this);
    connect(m_internalTimer, SIGNAL(timeout()), this, SLOT(updateSeconds()));

    secondsRemaining_ = 0;
    totalSeconds_ = 0;
    defaultIntervals = QList<int>() << 10 << 30 << 60 << 180 << 300 << 600 << 900 << 1200 << 1800 <<
                       2700 << 3600 << 7200 << 10800 << 14400 << 21600 << 43200 << 86400;
}

void TimerManager::findSeconds(bool typeCountSeconds)
{
    int count_seconds;

    if(gv.typeOfInterval==0)
        count_seconds=defaultIntervals.at(settings->value("timeSlider", 7).toInt()-1);
    else
        count_seconds=settings->value("days_box", 0).toInt()*86400+settings->value("hours_box", 0).toInt()*3600+
                        settings->value("minutes_box", 30).toInt()*60+settings->value("seconds_box", 0).toInt();

    if(typeCountSeconds)
    {
        //true,its time to change wallpaper and we want
        //to change the seconds_left to the current value of slider and to temp save the current value of slider.
        secondsRemaining_=totalSeconds_=count_seconds;
    }
    else
    {
        //false, we just changed the value of the slider and we want the value to be converted
        //in seconds BUT  we don't want seconds_left or tmp_slider to be changed.
        secondsInWallpapersSlider_=count_seconds;
    }
}

QString TimerManager::secondsToMinutesHoursDays(int seconds)
{
    if(seconds%86400 == 0)
    {
        if(seconds/86400 == 1)
            return QString(QString::number(1)+" "+tr("day"));
        else
            return QString(QString::number(seconds/86400)+" "+tr("days"));
    }
    else if(seconds%3600 == 0)
    {
        if(seconds/3600 == 1)
            return QString(QString::number(seconds/3600)+" "+tr("hour"));
        else
            return QString(QString::number(seconds/3600)+" "+tr("hours"));
    }
    else if(seconds%60 == 0)
    {
        if(seconds/60 == 1)
            return QString(QString::number(seconds/60)+" "+tr("minute"));
        else
            return QString(QString::number(seconds/60)+" "+tr("minutes"));
    }
    else if(seconds == 1)
        return QString(QString::number(1)+" "+tr("second"));
    else
        return QString(QString::number(seconds)+" "+tr("seconds"));
}


QString TimerManager::secondsToHms(int seconds){
    int minutes_left=0, hours_left=0, finalSeconds;

    if(seconds>=60){
        minutes_left=seconds/60;
        if(minutes_left>=60)
            hours_left=minutes_left/60;
        minutes_left=minutes_left-hours_left*60;
        finalSeconds=seconds-(hours_left*3600+minutes_left*60);
    }
    else
        finalSeconds=seconds;

    if(!hours_left && !minutes_left)
        return QString::number(finalSeconds)+tr("s");
    else if(!hours_left){
        if(finalSeconds)
            return QString::number(minutes_left)+tr("m")+" " + QString::number(finalSeconds)+tr("s");
        else
            return QString::number(minutes_left)+tr("m");
    }
    else if(!minutes_left){
        if(finalSeconds)
            return QString::number(hours_left) + tr("h")+" " + QString::number(finalSeconds)+tr("s");
        else
            return QString::number(hours_left) + tr("h");
    }
    else
    {
        if(finalSeconds)
            return QString::number(hours_left) + tr("h")+" " + QString::number(minutes_left)+tr("m")+" " + QString::number(finalSeconds)+tr("s");
        else
            return QString::number(hours_left) + tr("h")+" " + QString::number(minutes_left)+tr("m");
    }
}

QString TimerManager::secondsToHm(int seconds){
    if(seconds<=60)
        return QString("1"+tr("m"));
    else if(seconds<=3600)
        return QString::number(seconds/60)+QString(tr("m"));
    else
    {
        int hours=seconds/3600;
        seconds-=hours*3600;
        int minutes=seconds/60;
        if(minutes)
            return QString::number(hours)+QString(tr("h")+" ")+QString::number(minutes)+QString(tr("m"));
        else
            return QString::number(hours)+QString(tr("h"));
    }
}

QString TimerManager::secondsToMh(int seconds)
{
    if (seconds<60)
        return QString(QString::number(seconds) + " "+tr("seconds"));
    else if(seconds==60)
        return QString("1 "+tr("minute"));
    else if(seconds<3600)
        return QString(QString::number(seconds/60) + " "+tr("minutes"));
    else if(seconds==3600)
        return QString("1 "+tr("hour"));
    else if(seconds<86400)
        return QString(QString::number(seconds/3600) + " "+tr("hours"));
    else if(seconds==86400)
        return QString("1 "+tr("day"));
    else if(seconds==604800)
        return QString("1 "+tr("week"));

    return QString("");
}

bool TimerManager::isActive() const
{
    return m_internalTimer->isActive();
}

void TimerManager::start(bool potd) {
    m_internalTimer->start(potd ? 59500 : 1000); // Ticks every second
}

void TimerManager::stop() {
    m_internalTimer->stop();
}

void TimerManager::updateSeconds(){
    /*
     * This function runs once a second and reduces
     * the timeout_count, which, once 0, will update
     * the desktop background...
     */
    gv.runningTimeOfProcess = QDateTime::currentDateTime();
    if(secondsRemaining_ <= 0){
        handleTimerExpiry();
    }
    else if(secondsRemaining_ != gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval)){
        int secondsToChangingTime = gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
        if(secondsToChangingTime < 0){
            handleTimerExpiry();
        }
        else if (!(secondsRemaining_<(secondsToChangingTime-1) || secondsRemaining_>(secondsToChangingTime + 1))){
            secondsRemaining_ = secondsToChangingTime;
        }
    }

    secondsRemaining_--;
}

void TimerManager::resetSecondsRemaining(){
    secondsRemaining_=totalSeconds_;
    saveSecondsLeftNow();
}

void TimerManager::resetTimer(){
    if(m_internalTimer->isActive()){
        m_internalTimer->stop();
        secondsRemaining_=0;
        updateSeconds();
        m_internalTimer->start();
    }
}

void TimerManager::handleTimerExpiry()
{
    resetSecondsRemaining();
    gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsRemaining_);
    Q_EMIT timeToChangeWallpaper();
}

void TimerManager::saveSecondsLeftNow(bool keepIndependentInterval){
    if(!gv.independentIntervalEnabled){
        return;
    }

    if(!keepIndependentInterval){
        SettingsManager::setIndependentIntervalValue(INTERVAL_INDEPENDENCE_DEFAULT_VALUE);
    }
    else
    {
        QChar prefix;
        Q_EMIT persistencePrefixNeeded(prefix);
        if (prefix.isNull()) {
            return;
        }

        QDateTime currentTime = QDateTime::currentDateTime();
        QString valueToSave = QString("%1.%2.%3")
                                  .arg(prefix)
                                  .arg(secondsRemaining_)
                                  .arg(currentTime.toString("yyyy:MM:dd:HH:mm:ss"));
        SettingsManager::setIndependentIntervalValue(valueToSave);
    }
}

void TimerManager::tryRestoreState(FeatureController::Feature launchFeature)
{
    QString independentInterval = SettingsManager::getIndependentIntervalValue();

    if (!independentInterval.contains(".")) {
        return;
    }

    QStringList parts = independentInterval.split(".");
    if (parts.count() != 3) {
        return;
    }

    // Check if the saved state applies to the feature we are about to launch.
    if (parts.at(0) == "e") {
        if (launchFeature != FeatureController::Feature::LiveEarth) return;
    } else if (parts.at(0) == "w") {
        if (launchFeature != FeatureController::Feature::Website) return;
    } else { // 'p' for wallpapers
        if (launchFeature == FeatureController::Feature::Website || launchFeature == FeatureController::Feature::LiveEarth) return;
    }

    // Parse the remaining time and apply it.
    bool ok;
    int independence_change_seconds_left = parts.at(1).toInt(&ok);
    if (!ok || independence_change_seconds_left == -1) {
        return;
    }

    QDateTime time_then = QDateTime::fromString(parts.at(2), "yyyy:MM:dd:HH:mm:ss");
    if (time_then.isNull() || !time_then.isValid()) {
        return;
    }

    int secs_diff = time_then.secsTo(QDateTime::currentDateTime());
    independence_change_seconds_left -= secs_diff;

    if (independence_change_seconds_left > 0) {
        Global::debug("Interval independence is enabled (" + QString::number(independence_change_seconds_left) + " seconds)");
        secondsRemaining_ = independence_change_seconds_left;
    } else {
        secondsRemaining_ = 0;
    }
}
