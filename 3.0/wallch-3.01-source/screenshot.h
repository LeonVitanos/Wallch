/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vitanos

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

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QDialog>
#include "QGroupBox"
#include "QSpinBox"
#include "QGridLayout"


namespace Ui {

    QT_BEGIN_NAMESPACE
    class screenshot;
    class QGroupBox;
    class QSpinBox;
    class QGridLayout;
    class QHBoxLayout;
    QT_END_NAMESPACE
}

class screenshot : public QDialog
{
    Q_OBJECT

protected:
    void resizeEvent(QResizeEvent *event);

public:
    explicit screenshot(QWidget *parent = 0);
    ~screenshot();

Q_SIGNALS:
    void hide_it();
    void show_it();

private:
    Ui::screenshot *ui;
    void createOptionsGroupBox();
    void createButtonsLayout();
    void updateScreenshotLabel();
    QGroupBox *optionsGroupBox;
    QSpinBox *delaySpinBox;
    QPixmap originalPixmap;
    QGridLayout *optionsGroupBoxLayout;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonsLayout;

    private Q_SLOTS:
        void on_QuitButton_clicked();
        void on_saveScreenshot_clicked();
        void on_newScreenshotButton_clicked();
        void shootScreen();
        void updateCheckBox();

};

#endif // SCREENSHOT_H
