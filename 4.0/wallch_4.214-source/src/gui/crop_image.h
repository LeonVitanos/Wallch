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

#ifndef CROP_IMAGE_H
#define CROP_IMAGE_H

#include <QDialog>
#include <QLabel>
#include <QGridLayout>
#include <QMouseEvent>

class CropLabel : public QLabel
{
    Q_OBJECT

public:
    explicit CropLabel(QWidget *parent = 0);
    void setSelectionRect(const QRect &selectionRect){ selectionRect_=selectionRect; }
    QRect &selectionRect(){ return selectionRect_; }

private:
        QRect selectionRect_;

protected:
    void paintEvent(QPaintEvent *e);
};

namespace Ui {
class crop_image;
}

class CropImage : public QDialog
{
    Q_OBJECT
    
public:
    explicit CropImage(QImage *image, const QRect &curCropRect, QWidget *parent = 0);
    ~CropImage();
    CropLabel *image_label;
    
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent();
    void keyPressEvent ( QKeyEvent * event );
    void keyReleaseEvent ( QKeyEvent * event );

private:
    Ui::crop_image *ui;
    QVBoxLayout *mainLayout_;
    QHBoxLayout *buttonsLayout_;
    QImage *websiteImage_;
    bool selectionStarted_, shiftKeyDown_;
    int imageWidth_, imageHeight_, pointX_, pointY_;
    void createButtonsLayout();
    void moveScroll(short direction);
    void sendNewCoordinates(QRect coords);

private Q_SLOTS:
    void on_cancel_clicked();
    void on_ok_clicked();

Q_SIGNALS:
    void coordinates(QRect coords);
};

#endif // CROP_IMAGE_H
