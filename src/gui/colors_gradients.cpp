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

#include "colors_gradients.h"
#include "ui_colors_gradients.h"

#include <QSettings>
#include <QFileDialog>

#include "colormanager.h"
#include "settingsmanager.h"
#include "glob.h"

#ifdef Q_OS_WIN
    #include <stdio.h>
    #include <windows.h>
    #include <shlobj.h>
#else
    #include "desktopenvironment.h"
#endif

ColorsGradients::ColorsGradients(WallpaperManager *wallpaperManager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::colors_gradients)
{
    ui->setupUi(this);
    connect(ui->average_color_checkbox, &QCheckBox::clicked, this, &ColorsGradients::handleAverageColorCheck);
    connect(ui->saveButton, &QPushButton::clicked, this, &ColorsGradients::handleSaveButtonClick);
    connect(ui->primary_color_button, &QPushButton::clicked, this, &ColorsGradients::handlePrimaryColorButtonClick);
    connect(ui->secondary_color_button, &QPushButton::clicked, this, &ColorsGradients::handleSecondaryColorButtonClick);
    connect(ui->change_order, &QPushButton::clicked, this, &ColorsGradients::handleChangeOrderClick);
    connect(ui->solid_radioButton, &QRadioButton::clicked, this, &ColorsGradients::handleSolidRadioClick);
    connect(ui->horizontal_radioButton, &QRadioButton::clicked, this, &ColorsGradients::handleHorizontalRadioClick);
    connect(ui->vertical_radioButton, &QRadioButton::clicked, this, &ColorsGradients::handleVerticalRadioClick);
    connect(ui->colorModeButton, &QPushButton::clicked, this, &ColorsGradients::handleColorModeButtonClick);
    connect(ui->wallpaperModeButton, &QPushButton::clicked, this, &ColorsGradients::handleWallpaperModeButtonClick);

    wallpaperManager_ = wallpaperManager;

    if(currentShading == ColoringType::Solid)
        ui->solid_radioButton->setChecked(true);
    else if(currentShading == ColoringType::Vertical)
        ui->vertical_radioButton->setChecked(true);
    else if(currentShading == ColoringType::Horizontal)
        ui->horizontal_radioButton->setChecked(true);

    if (wallpaperManager_->getCurrentFit() == 0)
        ui->colorModeButton->setChecked(true);
    else{
        ui->solid_radioButton->setEnabled(false);
        ui->vertical_radioButton->setEnabled(false);
        ui->horizontal_radioButton->setEnabled(false);
    }

    actionForSecondaryButtons();

    //determining current primary color
    QImage image(60, 60, QImage::Format_ARGB32_Premultiplied);
    image.fill(ColorManager::getPrimaryColor(true));
    ui->primary_color_button->setIcon(QIcon(QPixmap::fromImage(image)));

    //determining secondary color
    QImage image2(60, 60, QImage::Format_ARGB32_Premultiplied);
    image2.fill(ColorManager::getSecondaryColor());
    ui->secondary_color_button->setIcon(QIcon(QPixmap::fromImage(image2)));

    updateGradientsOnlyColors(false);
    if(gv.setAverageColor){
        ui->average_color_checkbox->setChecked(true);
        handleAverageColorCheck(true);
    }
}

ColorsGradients::~ColorsGradients()
{
    delete ui;
}

void ColorsGradients::handleAverageColorCheck(bool checked)
{
    ui->primary_color_button->setEnabled(!checked);
    ui->change_order->setEnabled(!checked);
    gv.setAverageColor=ui->average_color_checkbox->isChecked();
    settings->setValue("average_color", gv.setAverageColor);
    settings->sync();
    if(gv.setAverageColor){
        QColor currentBgAverageColor = WallpaperManager::getAverageColorOf(WallpaperManager::currentBackgroundWallpaper());
        if(!currentBgAverageColor.isValid()){
            ColorManager::setPrimaryColor(currentBgAverageColor.name());
            updateGradientsOnlyColors(true);
        }
    }
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::updateGradientsOnlyColors(bool updateLeftRightSolid){
    int dim = 70; //TODO: 70 or 60?
    QImage image(dim, dim, QImage::Format_RGB32);

    if(updateLeftRightSolid){
        image.fill(QColor(ColorManager::getPrimaryColor(true)));
        ui->primary_color_button->setIcon(QIcon(QPixmap::fromImage(
            image.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::FastTransformation))));
    }

    if(!ui->solid_radioButton->isChecked()){
        if(updateLeftRightSolid){
            image.fill(QColor(ColorManager::getSecondaryColor()));
            ui->secondary_color_button->setIcon(QIcon(QPixmap::fromImage(
                image.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::FastTransformation))));
        }

        if(ui->horizontal_radioButton->isChecked()){
            image = ColorManager::createVerticalHorizontalImage(dim, dim);
            ui->result->setPixmap(QPixmap::fromImage(image));
        }
        else if(ui->vertical_radioButton->isChecked()){
            image = ColorManager::createVerticalHorizontalImage(dim, dim);
            ui->result->setPixmap(QPixmap::fromImage(image));
        }
    }

    Q_EMIT updateDesktopColor();
    Q_EMIT updateImageStyle();

    if(!updateLeftRightSolid || currentShading == ColoringType::Solid)
        return;

#ifdef Q_OS_LINUX
    if (currentDE == DE::Gnome || currentDE == DE::Mate) {
        return;
    }
#endif

    applyColorImageBackground();
}

void ColorsGradients::handleSaveButtonClick()
{
    close();
}

void ColorsGradients::handlePrimaryColorButtonClick()
{
    QColor initial = QColor::fromRgb(ui->primary_color_button->icon().pixmap(QSize(5,5), QIcon::Normal, QIcon::On).toImage().pixel(1,1));
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor color = QColorDialog::getColor(initial, this, tr("Select Color"), options);

    if(!color.isValid())
        return;

    ColorManager::setPrimaryColor(color.name(), true);
    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::handleSecondaryColorButtonClick()
{
    QColor initial = QColor::fromRgb(ui->secondary_color_button->icon().pixmap(QSize(5,5), QIcon::Normal, QIcon::On).toImage().pixel(1,1));
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor color = QColorDialog::getColor(initial, this, "Select Color", options);

    if(!color.isValid())
        return;

    ColorManager::setSecondaryColor(color.name());

    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::handleChangeOrderClick()
{
    //this turns the secondary color primary and vice versa...
    QString temp = ColorManager::getPrimaryColor(true);
    ColorManager::setPrimaryColor(ColorManager::getSecondaryColor());
    ColorManager::setSecondaryColor(temp);

    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::actionForSecondaryButtons()
{
    bool action = currentShading != ColoringType::Solid;

    if(action){
        ui->change_order->show();
        ui->secondary_color_button->show();
        ui->secondary_label->show();
        ui->result_label->show();
        ui->result->show();
    }
    else{
        ui->change_order->hide();
        ui->secondary_color_button->hide();
        ui->secondary_label->hide();
        ui->result_label->hide();
        ui->result->hide();
    }
}

void ColorsGradients::handleSolidRadioClick()
{
    if(currentShading == ColoringType::Solid)
        return;

    currentShading = ColoringType::Solid;

#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "solid");
    }
    else if(currentDE == DE::XFCE){
        changeXfColorStyle(0);
    }
    else if(currentDE == DE::KDE){
        wallpaperManager_->setCurrentFit(0);
    }
#else
    settings->setValue("ShadingType", "solid");
    wallpaperManager_->setBackground("", false, false, 0);
#endif

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::handleHorizontalRadioClick()
{
    if(currentShading == ColoringType::Horizontal)
        return;

    currentShading = ColoringType::Horizontal;

#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "horizontal");
    }
    else if(currentDE == DE::XFCE){
        changeXfColorStyle(1);
    }
    else{
        applyColorImageBackground();
    }
#else
    applyColorImageBackground();
#endif

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::applyColorImageBackground() {
    ColorManager::createVerticalHorizontalImage(gv.screenWidth, gv.screenHeight).save(gv.wallchHomePath + COLOR_IMAGE, 0, 80);
    wallpaperManager_->setBackground(gv.wallchHomePath + COLOR_IMAGE, false, false, 0);

    QString shadingStr = (currentShading == ColoringType::Horizontal) ? "horizontal" : "vertical";
    settings->setValue("ShadingType", shadingStr);

#ifdef Q_OS_LINUX
    if (currentDE == DE::KDE) {
        wallpaperManager_->setCurrentFit(1);
    }
#endif
}

void ColorsGradients::handleVerticalRadioClick()
{
    if(currentShading == ColoringType::Vertical)
        return;

    currentShading = ColoringType::Vertical;

#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate)
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "vertical");
    else if(currentDE == DE::XFCE){
        changeXfColorStyle(2);
    }
    else{
        applyColorImageBackground();
    }
#else
    applyColorImageBackground();
#endif

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::handleColorModeButtonClick()
{
    if(wallpaperManager_->getCurrentFit() == 0)
        return;

    ui->solid_radioButton->setEnabled(true);
    ui->vertical_radioButton->setEnabled(true);
    ui->horizontal_radioButton->setEnabled(true);

    wallpaperManager_->setCurrentFit(0);

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);

    Q_EMIT updateImageStyle();
    Q_EMIT updateTv();
}

void ColorsGradients::handleWallpaperModeButtonClick()
{
    if(wallpaperManager_->getCurrentFit() != 0)
        return;

    ui->solid_radioButton->setEnabled(false);
    ui->vertical_radioButton->setEnabled(false);
    ui->horizontal_radioButton->setEnabled(false);

#ifdef Q_OS_LINUX
    wallpaperManager_->setCurrentFit(2);
#else
    wallpaperManager_->setBackground(settings->value("last_wallpaper", wallpaperManager_->getPreviousWallpaper()).toString(), false, false, 0);
#endif

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);

    Q_EMIT updateTv();
}

#ifdef Q_OS_LINUX
void ColorsGradients::changeXfColorStyle(int set){
    DesktopEnvironment::processXfconfQuery({"color-style"}, [&](const QString &entry) {
        DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << QString::number(set));
        return false;
    });
}
#endif
