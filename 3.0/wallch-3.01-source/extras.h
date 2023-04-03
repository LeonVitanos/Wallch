/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vitanos

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

#ifndef EXTRAS_H
#define EXTRAS_H

#include <QDialog>
#include <QTimer>

namespace Ui {
    class Extras;
}

class Extras : public QDialog
{
    Q_OBJECT

public:
    explicit Extras(QWidget *parent = 0);
    ~Extras();

private Q_SLOTS:
    void on_closeButton_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void fix_buttons_livearth();
    void fix_buttons_photoofday();
    void on_add_clicked();
    void on_remove_clicked();
    void on_label_3_linkActivated();
    void on_label_5_linkActivated();
    QString get_ext(QString filename);
    void on_activate_photoofday_clicked();
    void on_save_pic_ofday_clicked();
    void on_deactivate_photoofday_clicked();
    void on_pushButton_clicked();
    void on_activate_monitor_clicked();
    void on_deactivate_monitor_clicked();

Q_SIGNALS:
    void start_live_earth();
    void stop_live_earth();
    void start_photo_of_day(QString hour_min);
    void stop_photo_of_day();
    void list_clear();
    void monitored_on(QString path);
    void monitored_off(QString path);

private:
    Ui::Extras *ui;
    void closeEvent( QCloseEvent * );
};

#endif // EXTRAS_H
