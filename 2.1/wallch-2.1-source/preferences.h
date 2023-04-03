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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

namespace Ui {
    class preferences;
}

class preferences : public QDialog {
    Q_OBJECT
public:
    preferences(QWidget *parent = 0);
    ~preferences();

Q_SIGNALS:
    void traytrue();
    void trayfalse();
    void togreek();
    void toenglish();
    void traytoenglish();
    void traytogreek();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::preferences *ui;
    void closeEvent( QCloseEvent * );

private Q_SLOTS:
    void on_checkBox_5_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void on_browsesoundButton_clicked();
    void on_customsoundcheckBox_clicked();
    void on_playsoundcheckBox_clicked();
    void on_aplyButton_clicked();
    void on_closeButton_clicked();
};

#endif // PREFERENCES_H
