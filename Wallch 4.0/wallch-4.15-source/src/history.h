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

#ifndef HISTORY_H
#define HISTORY_H

#include "properties.h"

#include <QTreeWidgetItem>
#include <QMenu>

namespace Ui {
    class history;
}

class History : public QDialog
{
    Q_OBJECT

public:
    explicit History(QWidget *parent = 0);
    ~History();

private:
    Ui::history *ui;
    QGroupBox *optionsGroupBox_;
    QGridLayout *optionsGroupBoxLayout_;
    QVBoxLayout *mainLayout_;
    QHBoxLayout *buttonsLayout_;
    QMenu *infoMenu_;
    QMenu *treeWidgetMenu_;
    Properties *historyProperties_;
    bool propertiesShown_;
    void readHistoryFiles();
    void addHistoryEntry(QString time, QString path, short type);
    QString numberWithLeadingZero(QString number);

private Q_SLOTS:
    void on_treeWidget_itemClicked(QTreeWidgetItem* item);
    void on_remove_history_clicked();
    void on_closeButton_clicked();
    void on_treeWidget_customContextMenuRequested();
    void on_info_customContextMenuRequested();
    void on_info_doubleClicked();
    void removeHistoryEntry();
    void showProperties();
    void openFolder();
    void copyPath();
    void copyLink();
    void launchInBrowser();
    void historyPropertiesDestroyed();
};

#endif // HISTORY_H
