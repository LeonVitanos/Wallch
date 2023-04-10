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

#ifndef LEPOINT_H
#define LEPOINT_H
#define QT_NO_KEYWORDS

#include <QDialog>
#include <QGraphicsPixmapItem>

#include "markitem.h"
#include "tryhard.h"

namespace Ui {
class lepoint;
}

class LEPoint : public QDialog
{
    Q_OBJECT

public:
    explicit LEPoint(QWidget *parent = 0);
    ~LEPoint();

private:
    bool alteringIndexesFromCode_ = false;
    short previousIconComboIndex = 0;
    const QStringList lePointImages_ = QStringList() << "http://i.imgur.com/ZfayMkO.jpg" << "http://i.imgur.com/xEHuNbe.jpg" << "http://melloristudio.com/wallch/le_point.jpg";
    TryHard *tryFetch_ = NULL;
    bool fetchFailed_ = false;

    void setOptionsValues(MarkItem *item);
    bool isValidImage(const QString &filename);
    QString basenameOf(const QString &path);
    void applyCustomIconToCombobox(const QString &path);
    void backgroundImageReady(const QPixmap &background);
    void showTools();
    void hideTools();
    void enableTools();
    void disableTools();

private Q_SLOTS:
    void pointItemFocusChanged(QGraphicsItem *newFocusItem, QGraphicsItem *oldFocusItem, Qt::FocusReason reason);
    void removeItem();
    void sceneChanged();
    void cannotFetchLeImage();
    void leImageFetchSuccess(const QByteArray &array);
    void on_scaleSlider_valueChanged(int value);
    void on_rotationSlider_valueChanged(int value);
    void on_addButton_clicked();
    void on_iconCombo_currentIndexChanged(int index);
    void on_ok_clicked();
    void on_cancel_clicked();

private:
    Ui::lepoint *ui;
    QGraphicsScene *scene_;
    QList<MarkItem*> marks_;
    MarkItem* lastSelectedItem_ = NULL;
    int backgroundWidth_, backgroundHeight_;

Q_SIGNALS:
    void lePreferencesChanged();
};

#endif // LEPOINT_H
