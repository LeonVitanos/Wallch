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

#include <QDialog>
#include <QColorDialog>
#include <QFile>
#include <QIcon>
#include "wallpapermanager.h"

namespace Ui {
class colors_gradients;
}

class ColorsGradients : public QDialog
{
    Q_OBJECT
    
public:
    explicit ColorsGradients(WallpaperManager *wallpaperManager, QWidget *parent = 0);
    ~ColorsGradients();
    
private Q_SLOTS:
    void handleAverageColorCheck(bool checked);
    void handleSaveButtonClick();
    void handlePrimaryColorButtonClick();
    void handleSecondaryColorButtonClick();
    void handleChangeOrderClick();
    void handleSolidRadioClick();
    void handleVerticalRadioClick();
    void handleHorizontalRadioClick();
    void handleColorModeButtonClick();
    void handleWallpaperModeButtonClick();

private:
    Ui::colors_gradients *ui;
    WallpaperManager *wallpaperManager_;
    void actionForSecondaryButtons();
    void updateGradientsOnlyColors(bool updateLeftRightSolid);

#ifdef Q_OS_LINUX
    // XFCE
    void changeXfColorStyle(int);
#endif

Q_SIGNALS:
    void updateDesktopColor();
    void updateTv();
    void updateImageStyle();
};

#endif // COLORS_GRADIENTS_H
