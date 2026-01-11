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

#ifndef PICTURES_LOCATIONS_H
#define PICTURES_LOCATIONS_H

#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QTreeWidgetItem>

class TreeWidgetDrop : public QTreeWidget
{
    Q_OBJECT

public:
    explicit TreeWidgetDrop(QWidget *parent = 0);
    void fixPositions();

private:
    QTreeWidgetItem *pictures_item;
    QTreeWidgetItem *deBackgrounds_item;

protected:
    virtual void dropEvent(QDropEvent *event);
};

namespace Ui {
class pictures_locations;
}

class PicturesLocations : public QDialog
{
    Q_OBJECT

public:
    TreeWidgetDrop *treeWidgetDrop;
    explicit PicturesLocations(QWidget *parent = 0);
    ~PicturesLocations();

private Q_SLOTS:
    void handleAddLocationClick();
    void handleRemoveLocationClick();
    void handleFoldersTreeItemChange(QTreeWidgetItem *current);
    void handleCancelClick();
    void handleSaveClick();
    void handleResetClick();

private:
    Ui::pictures_locations *ui;

Q_SIGNALS:
    void picturesLocationsChanged();
};

#endif // PICTURES_LOCATIONS_H
