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

#ifndef COLORS_GRADIENTS_H
#define COLORS_GRADIENTS_H

#define QT_NO_KEYWORDS

#include <QDialog>
#include <QPainter>
#include <QColorDialog>
#include <QFile>
#include <QIcon>
#include "glob.h"

namespace Ui {
class colors_gradients;
}

class ColorsGradients : public QDialog
{
    Q_OBJECT
    
public:
    explicit ColorsGradients(QWidget *parent = 0);
    ~ColorsGradients();
    
private Q_SLOTS:
    void on_average_color_checkbox_clicked(bool checked);
    void on_saveButton_clicked();
    void on_primary_color_button_clicked();
    void on_secondary_color_button_clicked();
    void on_change_order_clicked();
    void on_remove_background_clicked();
    void on_solid_radioButton_clicked();
    void on_vertical_radioButton_clicked();
    void on_horizontal_radioButton_clicked();

private:
    Ui::colors_gradients *ui;
    QString secondaryColor_;
    QString primaryColor_;
    QImage createVerticalHorizontalImage(const QString &type);
    void actionForSecondaryButtons(bool action);
#ifdef Q_OS_UNIX
    void setDesktopSecondaryColor(const QString &colorName);
#endif
    void setDesktopColor(const QString &colorName);
    void updateGradientsOnlyColors(bool updateLeftRightSolid);

Q_SIGNALS:
    void updateDesktopColor(QString colorName);
    void updateTv();
    void updateColorButtonSignal(QImage image);
};

#endif // COLORS_GRADIENTS_H
