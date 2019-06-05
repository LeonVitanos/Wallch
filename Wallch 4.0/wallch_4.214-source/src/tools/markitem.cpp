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

#include <QPainter>
#include <QCursor>
#include <QRect>
#include <QMenu>
#include <QGraphicsScene>

#include "markitem.h"

MarkItem::MarkItem()
{
    this->setFlag(QGraphicsItem::ItemIsMovable, true);
    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    this->setFlag(QGraphicsItem::ItemIsFocusable, true);
    this->setFlag(QGraphicsItem::ItemIsSelectable, true);
    this->setFocus(Qt::MouseFocusReason);
    this->setAcceptHoverEvents(true);
}

void MarkItem::hoverEnterEvent(QGraphicsSceneHoverEvent *){
    setCursor(QCursor(Qt::SizeAllCursor));
    this->update(this->boundingRect());
}

void MarkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *){
    this->update(this->boundingRect());
}

void MarkItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
    QMenu menu;
     QAction *deleteAction = menu.addAction("Delete");
     QAction *selectedAction = menu.exec(event->screenPos());
     if(deleteAction == selectedAction){
         this->scene()->removeItem(this);
     }
}

void MarkItem::setPointType(PointItemType::Value type, const QString &path /* = QString()*/){
    pointType_ = type;

    switch(type){
    default:
    case PointItemType::Point1:
        imagePath_ = ":/images/point1.png";
        break;
    case PointItemType::Point2:
        imagePath_ = ":/images/point2.png";
        break;
    case PointItemType::CustomPoint:
        imagePath_ = path;
        break;
    }

    this->setPixmap(QPixmap(imagePath_));
}

PointItemType::Value MarkItem::getType(){
    return pointType_;
}

QString MarkItem::imagePath(){
    return imagePath_;
}

bool MarkItem::willFinallyBeVisible(){
    return this->scene()->sceneRect().intersects(this->sceneBoundingRect());
}
