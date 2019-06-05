/*WallpaperChanger -A tool for having fun changing desktop wallpapers-
Copyright (C) 2010 by Leon Vytanos and Alex Solanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.*/
#ifndef LICENSE_H
#define LICENSE_H

#include <QDialog>

namespace Ui {
    class license;
}

class license : public QDialog {
    Q_OBJECT
public:
    license(QWidget *parent = 0);
    ~license();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::license *ui;

private slots:
    void on_pushButton_clicked();
};

#endif // LICENSE_H
