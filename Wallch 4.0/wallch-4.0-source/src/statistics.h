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

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QDialog>
#include <QTimer>

namespace Ui {
    class statistics;
}

class Statistics : public QDialog
{
    Q_OBJECT

public:
    explicit Statistics(QWidget *parent = 0);
    ~Statistics();

private:
    Ui::statistics *ui;
    QTimer *updateValuesTimer_;
    unsigned int totalUptime_, totalLaunchedTimes_;
    QString secondsToDateSring(unsigned int seconds);

private Q_SLOTS:
    void on_reset_clicked();
    void on_ok_clicked();
    void updateValues();
};

#endif // STATISTICS_H
