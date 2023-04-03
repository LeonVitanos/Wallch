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

#define QT_NO_KEYWORDS

#include "statistics.h"
#include "ui_statistics.h"
#include "glob.h"
#include "QDesktopWidget"

#ifdef ON_WIN32
#include <windows.h>
#endif

#include <QSettings>

Statistics::Statistics(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::statistics)
{
    ui->setupUi(this);
    //move dialog to center of the screen
    this->move(QApplication::desktop()->availableGeometry().center() - this->rect().center());

    totalLaunchedTimes_=settings->value("times_launched", 0).toUInt();
    totalUptime_=settings->value("seconds_passed", 0).toUInt();

    updateValuesTimer_ = new QTimer(this);
    connect(updateValuesTimer_,SIGNAL(timeout()),this,SLOT(updateValues()));
    updateValues();
    updateValuesTimer_->start(1000);
}

Statistics::~Statistics()
{
    delete ui;
}

void Statistics::on_ok_clicked()
{
    updateValuesTimer_->stop();
    this->close();
}

void Statistics::updateValues(){
    unsigned int currentSessionSeconds=gv.timeLaunched.secsTo(QDateTime::currentDateTime());

    ui->current_wallpapers_number->setText(QString::number(gv.wallpapersChangedCurrentSession));
    ui->current_uptime_number->setText(secondsToDateSring(currentSessionSeconds));

    ui->launchedtimes->setText(QString::number(totalLaunchedTimes_));
    ui->images_n->setText(QString::number(settings->value("images_changed", 0).toUInt()));
    ui->uptime->setText(secondsToDateSring(totalUptime_+currentSessionSeconds));
}

void Statistics::on_reset_clicked()
{
    totalLaunchedTimes_=totalUptime_=0;

    gv.appStartTime=QDateTime::currentDateTime();
    settings->setValue("seconds_passed", 0);
    settings->setValue("times_launched", 0);
    settings->setValue("images_changed", 0);
    settings->sync();
}

QString Statistics::secondsToDateSring(unsigned int seconds)
{
    unsigned int secs=0, minutes=0, hours=0, days=0;
    days=seconds/86400;
    hours=seconds/3600;
    minutes=seconds/60;
    secs=seconds;

    hours%=24;
    minutes%=60;
    secs%=60;

    return QString((days ? QString::number(days)+tr("d")+" ":"") + (hours ? QString::number(hours)+tr("h")+" ":"") + (minutes ? QString::number(minutes)+tr("m")+" ":"") + QString::number(secs)+tr("s"));
}
