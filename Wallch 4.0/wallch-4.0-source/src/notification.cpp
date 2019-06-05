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
    //actions in order for the dialog to look like a popup, to stay on top, and not to take focus
    Qt::WindowFlags flags = Qt::ToolTip | Qt::WindowStaysOnTopHint; //Qt::X11BypassWindowManagerHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_X11NetWmWindowTypeNotification, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    ui->setupUi(this);

    closeTimer_ = new QTimer(this);
    connect(closeTimer_, SIGNAL(timeout()), this, SLOT(closeNotification()));
    closeTimer_->setSingleShot(true);

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

    ui->message->setText(message);
    ui->image->setPixmap(QPixmap(image));

    this->resize(this->minimumWidth(), this->minimumHeight());
}

void Notification::closeNotification()
{
    close();
}

void Notification::enterEvent(QEvent *) {
  //if (mode_ == Mode_Popup)
    setWindowOpacity(0.25);
}

void Notification::leaveEvent(QEvent *) {
  setWindowOpacity(1.0);
}

void Notification::mousePressEvent(QMouseEvent* ) {
  //if (mode_ == Mode_Popup)
    close();
}
