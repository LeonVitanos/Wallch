
#include "timermanager.h"

TimerManager::TimerManager(QObject *parent):
    QObject(parent)
{
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
