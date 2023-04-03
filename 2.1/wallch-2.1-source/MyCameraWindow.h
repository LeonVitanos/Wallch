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

#ifndef MYCAMERAWINDOW_H_
#define MYCAMERAWINDOW_H_

#include "QOpenCVWidget.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMovie>

class MyCameraWindow : public QWidget
{
    Q_OBJECT
    private:
        QOpenCVWidget *cvwidget;
        QTimer *update_image;
        QTimer *countdown;
        QTimer *color_changer;
        void closeEvent( QCloseEvent * );

    public:
        MyCameraWindow(QWidget *parent=0);
        QVBoxLayout *layout;
        QPushButton *capturebtn;
        QPushButton *closebtn;
        QPushButton *newbtn;
        QPushButton *savebtn;
        QLabel *blink;
        QLabel *countdowner;
        QMovie *movie;

    Q_SIGNALS:
        void save_path(QString img_path);

    private Q_SLOTS:
        void take_image();
        void image_update();
        void close_now();
        void save_image();
        void new_image();
        void update_text();
        void update_color();
};


#endif /*MYCAMERAWINDOW_H_*/
