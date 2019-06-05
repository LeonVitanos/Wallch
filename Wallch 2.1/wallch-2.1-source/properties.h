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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include "QGroupBox"
#include "QGridLayout"

namespace Ui {
    class properties;
}

class properties : public QDialog
{
    Q_OBJECT

public:
    explicit properties(QString & img, QWidget *parent = 0);
    ~properties();

protected:
    void resizeEvent(QResizeEvent *event);


private:
    Ui::properties *ui;
    void closeEvent( QCloseEvent * );
    void createOptionsGroupBox();
    void createButtonsLayout();
    void updateScreenshotLabel();
    QGroupBox *optionsGroupBox;
    QPixmap originalPixmap;
    QGridLayout *optionsGroupBoxLayout;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonsLayout;

private Q_SLOTS:
    void on_pushButton_clicked();
};

#endif // PROPERTIES_H
