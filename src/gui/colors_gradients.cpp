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

#include "colors_gradients.h"
#include "ui_colors_gradients.h"

#include <QSettings>
#include <QFileDialog>

#include "colormanager.h"

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
    image.fill(ColorManager::getPrimaryColor());
    ui->primary_color_button->setIcon(QIcon(QPixmap::fromImage(image)));

    //determining secondary color
    QImage image2(60, 60, QImage::Format_ARGB32_Premultiplied);
    image2.fill(ColorManager::getSecondaryColor());
    ui->secondary_color_button->setIcon(QIcon(QPixmap::fromImage(image2)));

    updateGradientsOnlyColors(false);
    if(gv.setAverageColor){
        ui->average_color_checkbox->setChecked(true);
        on_average_color_checkbox_clicked(true);
    }
}

ColorsGradients::~ColorsGradients()
{
    delete ui;
}

void ColorsGradients::on_average_color_checkbox_clicked(bool checked)
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
        image.fill(QColor(ColorManager::getPrimaryColor()));
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
}

void ColorsGradients::on_saveButton_clicked()
{
    close();
}

void ColorsGradients::on_primary_color_button_clicked()
{
    QColor initial = QColor::fromRgb(ui->primary_color_button->icon().pixmap(QSize(5,5), QIcon::Normal, QIcon::On).toImage().pixel(1,1));
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor color = QColorDialog::getColor(initial, this, tr("Select Color"), options);

    if(!color.isValid())
        return;

    ColorManager::setPrimaryColor(color.name());
    Q_EMIT updateDesktopColor();
    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::on_secondary_color_button_clicked()
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

void ColorsGradients::on_change_order_clicked()
{
    //this turns the secondary color primary and vice versa...
    QString temp = ColorManager::getPrimaryColor();
    ColorManager::setPrimaryColor(ColorManager::getSecondaryColor());
    ColorManager::setSecondaryColor(temp);

    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen)
        Q_EMIT updateTv();
}

void ColorsGradients::actionForSecondaryButtons()
{
    bool action = currentShading != ColoringType::Solid && wallpaperManager_->getCurrentFit() == 0;

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

void ColorsGradients::on_solid_radioButton_clicked()
{
    if(currentShading == ColoringType::Solid)
        return;

#ifdef Q_OS_UNIX
    if(currentDE == DE::Gnome || currentDE == DE::UnityGnome || currentDE == DE::Mate){
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "solid");
    }
    else if(currentDE == DE::XFCE){
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color-style"))
                DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << "0");
        }
    }
#else
    settings->setValue("ShadingType", "solid");
    wallpaperManager_->setBackground("", false, false, 0);
#endif

    currentShading = ColoringType::Solid;
    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::on_horizontal_radioButton_clicked()
{
    if(currentShading == ColoringType::Horizontal)
        return;

#ifdef Q_OS_UNIX
    if(currentDE == DE::Gnome || currentDE == DE::UnityGnome || currentDE == DE::Mate){
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "horizontal");
    }
    else if(currentDE == DE::XFCE){
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color-style"))
                DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << "1");
        }
    }
#else
    ColorManager::createVerticalHorizontalImage(gv.screenWidth, gv.screenHeight).save(gv.wallchHomePath+COLOR_IMAGE, 0, 80);
    wallpaperManager_->setBackground(gv.wallchHomePath+COLOR_IMAGE, false, false, 0);
    settings->setValue("ShadingType", "horizontal");
#endif

    currentShading = ColoringType::Horizontal;
    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::on_vertical_radioButton_clicked()
{
    if(currentShading == ColoringType::Vertical)
        return;

#ifdef Q_OS_UNIX
    if(currentDE == DE::Gnome || currentDE == DE::UnityGnome || currentDE == DE::Mate)
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "vertical");
    else if(currentDE == DE::XFCE){
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("color-style"))
                DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << "2");
        }
    }
#else
    ColorManager::createVerticalHorizontalImage(gv.screenWidth, gv.screenHeight).save(gv.wallchHomePath+COLOR_IMAGE, 0, 80);
    wallpaperManager_->setBackground(gv.wallchHomePath+COLOR_IMAGE, false, false, 0);
    settings->setValue("ShadingType", "vertical");
#endif

    currentShading = ColoringType::Vertical;
    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);
    Q_EMIT updateTv();
}

void ColorsGradients::on_colorModeButton_clicked()
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

void ColorsGradients::on_wallpaperModeButton_clicked()
{
    if(wallpaperManager_->getCurrentFit() != 0)
        return;

    ui->solid_radioButton->setEnabled(false);
    ui->vertical_radioButton->setEnabled(false);
    ui->horizontal_radioButton->setEnabled(false);

#ifdef Q_OS_UNIX
    wallpaperManager_->setCurrentFit(2);
#else
    wallpaperManager_->setBackground(settings->value("last_wallpaper", wallpaperManager_->getPreviousWallpaper()).toString(), false, false, 0);
#endif

    actionForSecondaryButtons();
    updateGradientsOnlyColors(false);

    Q_EMIT updateTv();
}
