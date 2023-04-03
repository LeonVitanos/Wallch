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
    void on_pushButton_11_clicked();
    void on_pushButton_10_clicked();
    void on_use_current_radioButton_clicked();
    void on_image_from_disk_lock_clicked();
    void on_pushButton_12_clicked();
    void on_closeButton_clicked();
    void set_current();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void fix_buttons();

Q_SIGNALS:
    void start_live_earth();
    void stop_live_earth();

private:
    Ui::Extras *ui;
    void closeEvent( QCloseEvent * );
};

#endif // EXTRAS_H
