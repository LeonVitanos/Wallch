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

#include "wallpapermanager.h"

#ifdef Q_OS_WIN
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>
#endif

ColorsGradients::ColorsGradients(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::colors_gradients)
{
    ui->setupUi(this);

#ifdef Q_OS_UNIX
    ColoringType::Value coloringType;
    coloringType = ColorManager::getColoringType();
    if(coloringType == ColoringType::SolidColor){
        ui->solid_radioButton->setChecked(true);
        actionForSecondaryButtons(false);
    }
    else if(coloringType == ColoringType::VerticalColor){
        ui->vertical_radioButton->setChecked(true);
        actionForSecondaryButtons(true);
    }
    else if(coloringType == ColoringType::HorizontalColor){
        ui->horizontal_radioButton->setChecked(true);
        actionForSecondaryButtons(true);
    }
#else
    QString res = settings->value("ShadingType", "solid").toString();

    if(res == "solid"){
        ui->solid_radioButton->setChecked(true);
        actionForSecondaryButtons(false);
    }
    else if(res == "vertical" || res=="vertical-gradient"){
        ui->vertical_radioButton->setChecked(true);
        actionForSecondaryButtons(true);
    }
    else if(res == "horizontal-gradient"){
        ui->horizontal_radioButton->setChecked(true);
        actionForSecondaryButtons(true);
    }
#endif

    //determining current primary color
    QString res1 = ColorManager::getPrimaryColor();
    QImage image(60, 60, QImage::Format_ARGB32_Premultiplied);
    primaryColor_ = res1;
    image.fill(primaryColor_);
    ui->primary_color_button->setIcon(QIcon(QPixmap::fromImage(image)));

    //determining secondary color
    QString res2;
#ifdef Q_OS_UNIX
    res2=ColorManager::getSecondaryColor();
#endif
    secondaryColor_=res2;
    QImage image2(60, 60, QImage::Format_ARGB32_Premultiplied);
    image2.fill(secondaryColor_);
    ui->secondary_color_button->setIcon(QIcon(QPixmap::fromImage(image2)));

    updateGradientsOnlyColors(false);
    if(gv.setAverageColor)
    {
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
    if(gv.setAverageColor)
    {
        QColor currentBgAverageColor = WallpaperManager::getAverageColorOf(WallpaperManager::currentBackgroundWallpaper());
        if(!currentBgAverageColor.isValid()){
            primaryColor_ = currentBgAverageColor.name();
            ColorManager::setPrimaryColor(primaryColor_);
            updateGradientsOnlyColors(true);
        }
    }
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
}

void ColorsGradients::updateGradientsOnlyColors(bool updateLeftRightSolid){
    /*
     * If updateLeftRightSolid is true then all the buttons are updated,
     * if it is false, then only the top buttons are updated
     */

    QImage image(70, 70, QImage::Format_RGB32);
    image.fill(Qt::white);

    QColor color(primaryColor_);

    image.fill(color);
    if(updateLeftRightSolid)
    {
        QIcon pri_icon=QIcon(QPixmap::fromImage(image.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::FastTransformation)));
        ui->primary_color_button->setIcon(pri_icon);
    }

    if(ui->solid_radioButton->isChecked()){
        Q_EMIT updateColorButtonSignal(image);
    }

    /*
     * We drastically reduce the alpha  after the middle
     * of the image such as the second color not to be
     * so visible then.
     */
    color.setNamedColor(secondaryColor_);
    short alpha=255;
    short softness=0;

    QPainter painter;

    painter.begin(&image);
    //every pixel will be drawn with a different color
    for(short i=70;i>0;i--){
        if(!softness){
            alpha-=1;
            if(alpha<=244){
                softness=1;
            }
        }
        else if(softness==1){
            alpha-=3;
            if(alpha<=205){
                softness=2;
            }
        }
        else if(softness==2){
            alpha-=4;
        }

        color.setAlpha(alpha);
        painter.fillRect(i-1, 0, 1, 70, QColor(color));
    }
    painter.end();

    if(ui->horizontal_radioButton->isChecked())
    {
        Q_EMIT updateColorButtonSignal(image);
        ui->result->setPixmap(QPixmap::fromImage(image));
    }

    if(ui->vertical_radioButton->isChecked())
    {
        QTransform rot;rot.rotate(+90);
        image = image.transformed(rot, Qt::SmoothTransformation);
        Q_EMIT updateColorButtonSignal(image);
        ui->result->setPixmap(QPixmap::fromImage(image));
    }

    if(updateLeftRightSolid){
        image.fill(color);
        ui->secondary_color_button->setIcon(QIcon(QPixmap::fromImage(image.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::FastTransformation))));
    }
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

    if(!color.isValid()){
        return;
    }

    primaryColor_=color.name();
    ColorManager::setPrimaryColor(primaryColor_);
    Q_EMIT updateDesktopColor(primaryColor_);
    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
}

void ColorsGradients::on_secondary_color_button_clicked()
{
    QColor initial = QColor::fromRgb(ui->secondary_color_button->icon().pixmap(QSize(5,5), QIcon::Normal, QIcon::On).toImage().pixel(1,1));
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor color = QColorDialog::getColor(initial, this, "Select Color", options);

    if(!color.isValid()){
        return;
    }

#ifdef Q_OS_UNIX
    secondaryColor_=color.name();
    ColorManager::setSecondaryColor(secondaryColor_)
#endif

    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
}

void ColorsGradients::on_change_order_clicked()
{
    //this turns the secondary color primary and vice versa...
    QString temp;
    temp=primaryColor_;
    primaryColor_=secondaryColor_;
    secondaryColor_=temp;

    ColorManager::setPrimaryColor(primaryColor_);
#ifdef Q_OS_UNIX
    ColorManager::setSecondaryColor(secondaryColor_)
#endif

    updateGradientsOnlyColors(true);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
}

void ColorsGradients::on_remove_background_clicked()
{
    /*
     * Setting a 1x1 transparent image as desktop background. On Ubuntu 12.04 and back it would work to set an empty
     * string as desktop background, but it doesn't anymore (it rolls back to the default desktop background). This
     * solves any issue :)
     */

    if(!QFile::exists(gv.wallchHomePath+NULL_IMAGE)){
        QImage empty(1, 1, QImage::Format_ARGB32);
        empty.setPixel(0,0, qRgba(0, 0, 0, 0));
        empty.save(gv.wallchHomePath+NULL_IMAGE, "PNG", 1);
    }
    WallpaperManager::setBackground(gv.wallchHomePath+NULL_IMAGE, false, false, 0);
}

void ColorsGradients::actionForSecondaryButtons(bool action)
{
    if(action)
    {
        ui->change_order->show();
        ui->secondary_color_button->show();
        ui->secondary_label->show();
        ui->result_label->show();
        ui->result->show();
    }
    else
    {
        ui->change_order->hide();
        ui->secondary_color_button->hide();
        ui->secondary_label->hide();
        ui->result_label->hide();
        ui->result->hide();
    }
}

void ColorsGradients::on_solid_radioButton_clicked()
{
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Mate){
        Global::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "solid");
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color-style")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << "0");
            }
        }
    }
#else
    settings->setValue("ShadingType" , "solid");
#endif //#ifdef Q_OS_UNIX
    ui->solid_radioButton->setChecked(true);
    actionForSecondaryButtons(false);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
    updateGradientsOnlyColors(false);
}

void ColorsGradients::on_horizontal_radioButton_clicked()
{
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Mate){
        Global::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "horizontal");
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color-style")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << "1");
            }
        }
    }
#else
    settings->setValue("ShadingType" , "horizontal");
#endif //#ifdef Q_OS_UNIX
    ui->horizontal_radioButton->setChecked(true);
    actionForSecondaryButtons(true);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
    updateGradientsOnlyColors(false);
    createVerticalHorizontalImage("horizontal");
}

void ColorsGradients::on_vertical_radioButton_clicked()
{
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Mate){
        Global::gsettingsSet("org.gnome.desktop.background", "color-shading-type", "vertical");
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color-style")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << "2");
            }
        }
    }
#else
    settings->setValue("ShadingType" , "vertical");
#endif //#ifdef Q_OS_UNIX
    ui->vertical_radioButton->setChecked(true);
    actionForSecondaryButtons(true);
    if(gv.previewImagesOnScreen){
        Q_EMIT updateTv();
    }
    updateGradientsOnlyColors(false);
    createVerticalHorizontalImage("vertical");
}

QImage ColorsGradients::createVerticalHorizontalImage(const QString &type)
{
    QImage image(gv.screenWidth, gv.screenHeight, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter;
    painter.begin(&image);

    int rec_x1, rec_y1, rec_x2, rec_y2;
    if(type == "horizontal"){
        rec_x1 = gv.screenWidth/256+1 , rec_y1 = 0, rec_x2 = gv.screenWidth, rec_y2 = 0;
    }
    else
    {
        rec_x1=0, rec_y1 = gv.screenHeight/256+1, rec_x2 = 0, rec_y2 = gv.screenHeight;
    }

    QLinearGradient rect_gradient(rec_x1, rec_y1, rec_x2, rec_y2);
    rect_gradient.setColorAt(0, QColor(primaryColor_));
    rect_gradient.setColorAt(1, QColor(secondaryColor_));
    painter.setBrush(rect_gradient);
    painter.drawRect(0, 0, gv.screenWidth, gv.screenHeight);

    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);

    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRect(0, 0, gv.screenWidth, gv.screenHeight), image);

    return image;
}
