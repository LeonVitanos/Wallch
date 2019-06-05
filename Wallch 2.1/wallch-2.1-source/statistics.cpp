/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vytanos

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.*/

#include "statistics.h"
#include "ui_statistics.h"
#include "glob.h"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <QDir>
#include <QSettings>

using namespace std;

long uptime_n=0,launched_n=0,images_n=0;
int _reset_=0;

statistics::statistics(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::statistics)
{
    ui->setupUi(this);

    _reset_=0;
    QSettings settings("Wallch", "Statistics");
    launched_n=settings.value("times_launched").toDouble();
    uptime_n=settings.value("time_number").toDouble();
    images_n=settings.value("images_number").toDouble();

    if(!_reset_)
        uptime_n+=__statistic__etimer_elapsed;
    update_values = new QTimer;
    connect(update_values,SIGNAL(timeout()),this,SLOT(values_update()));
    values_update();
    update_values->start(1000);
}

statistics::~statistics()
{
    delete ui;
}

void statistics::on_ok_clicked()
{
    update_values->stop();
    this->close();
}

void statistics::values_update(){
    if(ui->images_n->text().toInt()!=__statistic__images_n+images_n)
        ui->images_n->setText(QString::number(__statistic__images_n+images_n));
    ui->launchedtimes->setText(QString::number(launched_n));

    long secs=0,minutes=0,hours=0,days=0;
    days=uptime_n/86400;
    hours=uptime_n/3600;
    minutes=uptime_n/60;
    secs=uptime_n;

    if(hours>=24)
        hours%=24;
    if(minutes>=60)
        minutes%=60;
    if(secs>=60)
        secs%=60;
        ui->uptime->setText(QString::number(days) + "d " + QString::number(hours) + "h " + QString::number(minutes)+"m " + QString::number(secs)+"s");
    uptime_n++;
}

void statistics::on_reset_clicked()
{
    _reset_=1;
    __statistic__images_n=0;
    images_n=0;
    launched_n=0;
    uptime_n=0;
    QSettings settings("Wallch", "Statistics");
    settings.setValue("time_number",0);
    settings.setValue("times_launched",0);
    settings.setValue("images_number",0);
}
