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

#define QT_NO_KEYWORDS

#include "potd_preview.h"
#include "ui_potd_preview.h"

#include "glob.h"

#include <QColorDialog>
#include <QShortcut>
#include <QFontMetrics>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>

PotdPreview::PotdPreview(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PotdPreview)
{
    ui->setupUi(this);

    ui->potdFontComboBox->setCurrentFont(QFont(gv.potdDescriptionFont));
    ui->potd_description_bottom_radioButton->setChecked(gv.potdDescriptionBottom);
    if(gv.potdDescriptionBottom){
        ui->bottom_top_margin_label->setText(tr("Bottom:"));
    }
    else
    {
        ui->bottom_top_margin_label->setText(tr("Top:"));
    }
    ui->potd_description_top_radioButton->setChecked(!gv.potdDescriptionBottom);
    ui->left_margin_spinbox->setValue(gv.potdDescriptionLeftMargin);
    ui->right_margin_spinbox->setValue(gv.potdDescriptionRightMargin);
    ui->bottom_top_margin_spinbox->setValue(gv.potdDescriptionBottomTopMargin);
    textColor_=gv.potdDescriptionColor;
    backgroundColor_=gv.potdDescriptionBackgroundColor;

    originalTextColor_=textColor_;
    originalBackgroundColor_=backgroundColor_;
    originalTextFont_=gv.potdDescriptionFont;
    originalTop_=ui->potd_description_top_radioButton->isChecked();
    originalLeftMargin_=gv.potdDescriptionLeftMargin;
    originalRightMargin_=gv.potdDescriptionRightMargin;
    originalBottomTopMargin_=gv.potdDescriptionBottomTopMargin;

    if(QFile::exists(gv.wallchHomePath+POTD_PREVIEW_IMAGE)){
        QImage *image = new QImage(gv.wallchHomePath+POTD_PREVIEW_IMAGE);
        fetchFinished(image);
    }
    else
    {
        ui->potdLabel->hide();
        ui->potdFontComboBox->hide();
        ui->potd_description_bottom_radioButton->hide();
        ui->potd_description_top_radioButton->hide();
        ui->descrPos_label->hide();
        ui->font_label->hide();
        ui->textColorPotd->hide();
        ui->backgroundColorPotd->hide();
        ui->ok->hide();
        ui->margins_label->hide();
        ui->left_margin_label->hide();
        ui->left_margin_spinbox->hide();
        ui->right_margin_label->hide();
        ui->right_margin_spinbox->hide();
        ui->bottom_top_margin_label->hide();
        ui->bottom_top_margin_spinbox->hide();

        this->adjustSize();

        (void) new QShortcut(Qt::Key_Escape, this, SLOT(on_cancel_clicked()));

        fileDownloader_=new QNetworkAccessManager(this);

        QObject::connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadedImage(QNetworkReply*)));

        getPotdPreviewImage();
    }
}

PotdPreview::~PotdPreview()
{
    if(currentNetworkRequest_){
        currentNetworkRequest_->deleteLater();
    }
    if(originalImage_){
        delete originalImage_;
    }
    delete ui;
}

void PotdPreview::resizeEvent(QResizeEvent *)
{
    QSize scaledSize = currentPixmap_.size();
    scaledSize.scale(ui->potdLabel->size(), Qt::KeepAspectRatio);
    if (!ui->potdLabel->pixmap() || scaledSize != ui->potdLabel->pixmap()->size()){
        updateLabel();
    }
}

void PotdPreview::updateLabel()
{
    ui->potdLabel->setPixmap(currentPixmap_.scaled(ui->potdLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void PotdPreview::getPotdPreviewImage(){
    if(currentBackupLinkIndex_>=potdPreviewImages_.count()){
        imageFetchfailed();
    }
    else
    {
        currentNetworkRequest_=fileDownloader_->get(QNetworkRequest(QUrl(potdPreviewImages_.at(currentBackupLinkIndex_))));
    }
}

void PotdPreview::imageFetchfailed(){
    fetchFailed_=true;
    ui->infoLabel->setText(tr("The Picture Of The Day Image used for preview failed to download. Please check your internet connection."));
    ui->cancel->hide();
    ui->ok->show();
}

void PotdPreview::downloadedImage(QNetworkReply *reply){
    currentNetworkRequest_=NULL;
    if(reply==NULL){
        currentBackupLinkIndex_++;
        getPotdPreviewImage();
        return;
    }
    else if(reply->error()){
        reply->deleteLater();
        currentBackupLinkIndex_++;
        getPotdPreviewImage();
        return;
    }

    QImage *img = new QImage();

    img->loadFromData(reply->readAll());

    reply->deleteLater();

    if(img->isNull()){
        currentBackupLinkIndex_++;
        getPotdPreviewImage();
        return;
    }

    img->save(gv.wallchHomePath+POTD_PREVIEW_IMAGE, "JPG", 100);

    fetchFinished(img);
}

void PotdPreview::fetchFinished(QImage *img){
    originalImage_=img;
    currentPixmap_ = QPixmap().fromImage(*img);
    updateLabel();

    ui->potdLabel->show();
    ui->potdFontComboBox->show();
    ui->potd_description_bottom_radioButton->show();
    ui->potd_description_top_radioButton->show();
    ui->descrPos_label->show();
    ui->font_label->show();
    ui->textColorPotd->show();
    ui->backgroundColorPotd->show();
    ui->ok->show();
    ui->margins_label->show();
    ui->left_margin_label->show();
    ui->left_margin_spinbox->show();
    ui->right_margin_label->show();
    ui->right_margin_spinbox->show();
    ui->bottom_top_margin_label->show();
    ui->bottom_top_margin_spinbox->show();

    ui->infoLabel->hide();
    ui->progressBar->hide();

    fetchFinished_=true;

    writeDescription();
}

void PotdPreview::writeDescription(){
    if(originalImage_==NULL){
        return;
    }
    QImage image = QImage(*originalImage_);
    QPainter drawer(&image);

    QFont font = ui->potdFontComboBox->currentFont();
    font.setPixelSize(image.width()/55);

    drawer.setFont(font);

    QRect drawRect;
    drawRect=drawer.fontMetrics().boundingRect(QRect(0, 0, image.width()-(ui->left_margin_spinbox->value()+ui->right_margin_spinbox->value()), image.height()), (Qt::TextWordWrap | Qt::AlignCenter), potdDescription_);
    drawRect.setX(0);
    drawRect.setWidth(image.width());

    int newY;
    if(ui->potd_description_bottom_radioButton->isChecked()){
        newY=image.height()-drawRect.height()-ui->bottom_top_margin_spinbox->value();
    }
    else
    {
        newY=ui->bottom_top_margin_spinbox->value();
    }
    int oldH=drawRect.height();
    drawRect.setY(newY);
    drawRect.setHeight(oldH);

    QColor backgroundColor=QColor(backgroundColor_);
    backgroundColor.setAlpha(176);

    drawer.fillRect(drawRect, QBrush(backgroundColor));

    drawRect.setX(ui->left_margin_spinbox->value());
    drawRect.setWidth(image.width()-(ui->left_margin_spinbox->value()+ui->right_margin_spinbox->value()));

    drawer.setPen(QColor(textColor_));
    drawer.drawText(drawRect, (Qt::TextWordWrap | Qt::AlignCenter), potdDescription_, &drawRect);

    drawer.end();

    currentPixmap_ = QPixmap().fromImage(image);
    updateLabel();
}

void PotdPreview::on_textColorPotd_clicked()
{
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor textColor = QColorDialog::getColor(QColor(textColor_), this, tr("Select Color"), options);
    if(textColor.isValid()){
        textColor_=textColor.name();
        writeDescription();
    }
}

void PotdPreview::on_backgroundColorPotd_clicked()
{
    QColorDialog::ColorDialogOptions options = QFlag(0);
    QColor backgroundColor = QColorDialog::getColor(QColor(backgroundColor_), this, tr("Select Color"), options);
    if(backgroundColor.isValid()){
        backgroundColor_=backgroundColor.name();
        writeDescription();
    }
}

void PotdPreview::on_potdFontComboBox_currentFontChanged()
{
    if(fetchFinished_){
        writeDescription();
    }
}

void PotdPreview::on_potd_description_bottom_radioButton_clicked()
{
    ui->bottom_top_margin_label->setText(tr("Bottom:"));
    if(fetchFinished_){
        writeDescription();
    }
}

void PotdPreview::on_potd_description_top_radioButton_clicked()
{
    ui->bottom_top_margin_label->setText(tr("Top:"));
    if(fetchFinished_){
        writeDescription();
    }
}

void PotdPreview::on_ok_clicked()
{
    if(!fetchFailed_){
        gv.potdDescriptionBottom=ui->potd_description_bottom_radioButton->isChecked();
        gv.potdDescriptionFont=ui->potdFontComboBox->currentFont().family();
        gv.potdDescriptionColor=textColor_;
        gv.potdDescriptionBackgroundColor=backgroundColor_;
        gv.potdDescriptionLeftMargin=ui->left_margin_spinbox->value();
        gv.potdDescriptionRightMargin=ui->right_margin_spinbox->value();
        gv.potdDescriptionBottomTopMargin=ui->bottom_top_margin_spinbox->value();

        settings->setValue("potd_description_bottom", gv.potdDescriptionBottom);
        settings->setValue("potd_description_font", gv.potdDescriptionFont);
        settings->setValue("potd_text_color", gv.potdDescriptionColor);
        settings->setValue("potd_background_color", gv.potdDescriptionBackgroundColor);
        settings->setValue("potd_description_left_margin", gv.potdDescriptionLeftMargin);
        settings->setValue("potd_description_right_margin", gv.potdDescriptionRightMargin);
        settings->setValue("potd_description_bottom_top_margin", gv.potdDescriptionBottomTopMargin);
        settings->sync();

        if(originalTextColor_!=textColor_ || originalBackgroundColor_!=backgroundColor_ || originalTextFont_!=gv.potdDescriptionFont || originalTop_!=ui->potd_description_top_radioButton->isChecked() || originalLeftMargin_!=gv.potdDescriptionLeftMargin || originalRightMargin_!=gv.potdDescriptionRightMargin || originalBottomTopMargin_!=gv.potdDescriptionBottomTopMargin){
            Q_EMIT potdPreferencesChanged();
        }
    }

    this->close();
}

void PotdPreview::on_cancel_clicked()
{
    if(currentNetworkRequest_){
        currentNetworkRequest_->abort();
    }
    this->close();
}

void PotdPreview::on_left_margin_spinbox_valueChanged()
{
    writeDescription();
}

void PotdPreview::on_right_margin_spinbox_valueChanged()
{
    on_left_margin_spinbox_valueChanged();
}

void PotdPreview::on_bottom_top_margin_spinbox_valueChanged()
{
    on_left_margin_spinbox_valueChanged();
}
