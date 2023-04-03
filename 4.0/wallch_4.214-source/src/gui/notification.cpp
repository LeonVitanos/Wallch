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

#define QT_NO_KEYWORDS

#include "notification.h"
#include "ui_notification.h"
#include "glob.h"

#include <QTimer>
#include <QDesktopWidget>

Notification::Notification(QString message, QString image, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Notification)
{
    ui->setupUi(this);

    closeTimer_ = new QTimer(this);
    connect(closeTimer_, SIGNAL(timeout()), this, SLOT(closeNotification()));
    closeTimer_->setSingleShot(true);

    opacityTimer_ = new QTimer(this);
    connect(opacityTimer_, SIGNAL(timeout()), this, SLOT(lessOpacity()));

    setupNotification(message, image);
}

Notification::~Notification()
{
    delete ui;
}

void Notification::setupNotification(QString message, QString image)
{
    if(closeTimer_->isActive())
        closeTimer_->stop();
    closeTimer_->start(3500);

    if(this->pos()!=QPoint(20, 20)){
        this->move(20,20);
    }

    ui->message->setText(QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">")+
                         "p, li { white-space: pre-wrap; } </style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:12pt; font-weight:400; font-style:normal;\">"+
                         "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"+
                         message+"</p></body></html>");
    ui->image->setPixmap(QPixmap(image));

    this->resize(this->minimumWidth(), this->minimumHeight());
}

void Notification::closeNotification()
{
    if(!mouseOnMe){
        opacityTimer_->start(3); // 3*100=300 milleseconds of opacity of opacity effect
    }
    else{
        opacityTimerWaiting=true;
    }
}

void Notification::lessOpacity()
{
    currentOpacity-=0.01;
    setWindowOpacity(currentOpacity);
    if(currentOpacity<=0.00)
        close();
}

void Notification::enterEvent(QEvent *) {
    mouseOnMe=true;
    if(opacityTimer_->isActive())
    {
        opacityTimerWaiting=true;
        opacityTimer_->stop();
    }
    setWindowOpacity(0.50);
}

void Notification::leaveEvent(QEvent *) {
    mouseOnMe=false;
    if(opacityTimerWaiting){
        opacityTimerWaiting=false;
        currentOpacity=0.50;
        opacityTimer_->start(3);
    }
    else{
        setWindowOpacity(1.0);
    }
}

void Notification::mousePressEvent(QMouseEvent* ) {
    //if (mode_ == Mode_Popup)
    close();
}
