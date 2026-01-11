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
