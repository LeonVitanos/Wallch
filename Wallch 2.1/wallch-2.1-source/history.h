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

#ifndef HISTORY_H
#define HISTORY_H

#include <QDialog>
#include "QListWidgetItem"
#include <QTreeWidgetItem>
#include "QGroupBox"
#include "QGridLayout"

namespace Ui {
    class history;
}

class history : public QDialog
{
    Q_OBJECT

public:
    explicit history(QWidget *parent = 0);
    ~history();

private:
    Ui::history *ui;
    void createOptionsGroupBox();
    void createButtonsLayout();
    QGroupBox *optionsGroupBox;
    QGridLayout *optionsGroupBoxLayout;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonsLayout;

private Q_SLOTS:
    void on_pushButton_2_clicked();
    void on_treeWidget_itemClicked(QTreeWidgetItem* item);
    void on_pushButton_clicked();
    void on_closeButton_clicked();
    void updater();
    bool removeDir(const QString &dirName);
};

#endif // HISTORY_H
