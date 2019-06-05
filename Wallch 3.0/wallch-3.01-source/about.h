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

#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

namespace Ui {
    class about;
}

class about : public QDialog {
    Q_OBJECT
public:
    about(QWidget *parent = 0);
    ~about();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::about *ui;

private Q_SLOTS:
    void on_label_5_linkActivated();
    void on_pushButton_3_clicked();
    void on_pushButton_2_clicked();
    void on_closeButton_clicked();
    void on_pushButton_clicked();
};

#endif // ABOUT_H
