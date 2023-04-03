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

#ifndef QOPENCVWIDGET_H
#define QOPENCVWIDGET_H

#include <opencv/cv.h>
#include <QPixmap>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QImage>

class QOpenCVWidget : public QWidget {
    private:
        QLabel *imagelabel;
        QVBoxLayout *layout;
        
        QImage image;
        
    public:
        QOpenCVWidget(QWidget *parent = 0);
        ~QOpenCVWidget(void);
        void putImage(IplImage *);
}; 

#endif
