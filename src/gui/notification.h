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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QDialog>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEvent>
#include <QMouseEvent>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QEnterEvent>
#endif

namespace Ui {
class Notification;
}

class Notification : public QDialog
{
    Q_OBJECT

public:
    explicit Notification(QString message, QString image, QWidget *parent = 0);
    ~Notification();
    bool opacityTimerWaiting=false;
    bool mouseOnMe=false;
    double currentOpacity=1.00;

private:
    Ui::Notification *ui;
    QTimer *closeTimer_;
    QTimer *opacityTimer_;
    QGraphicsOpacityEffect* opacityEffect_;
    void handleEnterEventLogic();

private Q_SLOTS:
    void setupNotification(QString message, QString image);
    void closeNotification();
    void lessOpacity();

protected:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif

    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // NOTIFICATION_H
