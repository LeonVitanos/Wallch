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
#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include "credits.h"
#include "license.h"

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
    license *License;
    credits *Credits;
    void closeEvent( QCloseEvent * );

private slots:
    void on_label_5_linkActivated();
    void on_pushButton_3_clicked();
    void on_pushButton_2_clicked();
    void on_closeButton_clicked();
};

#endif // ABOUT_H
