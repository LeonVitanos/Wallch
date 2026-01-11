/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef MARKITEM_H
#define MARKITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>

struct PointItemType {
  enum Value {
    Point1, Point2, CustomPoint
  };
};

class MarkItem : public QGraphicsPixmapItem
{
public:
    MarkItem();
    void setPointType(PointItemType::Value type, const QString &path = QString());
    PointItemType::Value getType();
    bool willFinallyBeVisible();
    QString imagePath();

private:
    PointItemType::Value pointType_;
    QString imagePath_;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

#endif // MARKITEM_H
