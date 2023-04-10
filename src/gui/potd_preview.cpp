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

    ui->left_margin_spinbox->setMaximum(gv.screenAvailableWidth-100);
    ui->right_margin_spinbox->setMaximum(gv.screenAvailableWidth-100);
    ui->bottom_top_margin_spinbox->setMaximum(gv.screenAvailableHeight-100);

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
        fetchFinished(QImage(gv.wallchHomePath+POTD_PREVIEW_IMAGE));
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

        (void) new QShortcut(Qt::Key_Escape, this, SLOT(on_cancel_clicked()));

        tryFetch_ = new TryHard(this, potdPreviewImages_);

        connect(tryFetch_, SIGNAL(success(QByteArray)), this, SLOT(downloadedImage(QByteArray)));;
        connect(tryFetch_, SIGNAL(failed()), this, SLOT(imageFetchfailed()));

        tryFetch_->start();
    }
}

PotdPreview::~PotdPreview()
{
    delete ui;
}

void PotdPreview::resizeEvent(QResizeEvent *)
{
    if(loadingWindow || currentPixmap_.isNull() || ui->potdLabel->pixmap(Qt::ReturnByValue).isNull())
        return;

    QSize scaledSize = currentPixmap_.size();
    scaledSize.scale(ui->potdLabel->size(), Qt::KeepAspectRatio);
    if (scaledSize != ui->potdLabel->pixmap(Qt::ReturnByValue).size())
        updateLabel();
}

void PotdPreview::updateLabel()
{
    ui->potdLabel->setPixmap(currentPixmap_.scaled(ui->potdLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void PotdPreview::imageFetchfailed(){
    fetchFailed_=true;
    ui->infoLabel->setText(tr("The Picture Of The Day image used for preview failed to download. Please check your internet connection or try later."));
    ui->cancel->hide();
    ui->ok->show();
    ui->progressBar->hide();
    ui->line->hide();
    //TODO:Check if needed
    //this->resize(this->minimumWidth(), this->minimumHeight());
    //this->move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center());
}

void PotdPreview::downloadedImage(QByteArray array){
    QImage img;
    img.loadFromData(array);
    img.save(gv.wallchHomePath+POTD_PREVIEW_IMAGE, "JPG", 100);

    fetchFinished(img);
}

void PotdPreview::fetchFinished(QImage img){
    originalImage_=img;
    currentPixmap_ = QPixmap().fromImage(img);

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

    fetchFinished_ = true;

    loadingWindow=false;

    QTimer::singleShot(100, this, SLOT(updateLabel()));
    QTimer::singleShot(150, this, SLOT(writeDescription()));
}

void PotdPreview::writeDescription(){
    if (!fetchFinished_) {
        return;
    }

    QImage image = originalImage_;
    QPainter painter(&image);

    QFont font = ui->potdFontComboBox->currentFont();
    font.setPixelSize(image.width()/55);

    painter.setFont(font);

    float divImageWidth=(float)image.width()/(float)gv.screenAvailableWidth;
    float divImageHeight=(float)image.height()/(float)gv.screenAvailableHeight;

    QRect drawRect = painter.fontMetrics().boundingRect(QRect(0, 0, image.width()-(divImageWidth*ui->left_margin_spinbox->value()+divImageHeight*ui->right_margin_spinbox->value()), image.height()), (Qt::TextWordWrap | Qt::AlignCenter), potdDescription_);
    drawRect.setX(0);
    drawRect.setWidth(image.width());

    int newY;
    if(ui->potd_description_bottom_radioButton->isChecked()){
        newY=image.height()-drawRect.height()-divImageHeight*ui->bottom_top_margin_spinbox->value();
    }
    else
    {
        newY=divImageHeight*ui->bottom_top_margin_spinbox->value();
    }
    int oldH=drawRect.height();
    drawRect.setY(newY);
    drawRect.setHeight(oldH);

    QColor backgroundColor=QColor(backgroundColor_);
    backgroundColor.setAlpha(176);

    painter.fillRect(drawRect, QBrush(backgroundColor));

    drawRect.setX(divImageWidth*ui->left_margin_spinbox->value());
    drawRect.setWidth(image.width()-(divImageWidth*ui->left_margin_spinbox->value()+divImageWidth*ui->right_margin_spinbox->value()));

    painter.setPen(QColor(textColor_));
    painter.drawText(drawRect, (Qt::TextWordWrap | Qt::AlignCenter), potdDescription_, &drawRect);

    painter.end();

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
        backgroundColor_ = backgroundColor.name();
        writeDescription();
    }
}

void PotdPreview::on_potdFontComboBox_currentFontChanged()
{
    writeDescription();
}

void PotdPreview::on_potd_description_bottom_radioButton_clicked()
{
    ui->bottom_top_margin_label->setText(tr("Bottom:"));
    writeDescription();
}

void PotdPreview::on_potd_description_top_radioButton_clicked()
{
    ui->bottom_top_margin_label->setText(tr("Top:"));
    writeDescription();
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
    if(tryFetch_){
        tryFetch_->abort();
    }
    this->close();
}

void PotdPreview::on_left_margin_spinbox_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    ui->left_margin_spinbox->setMaximum(gv.screenAvailableWidth-300-ui->right_margin_spinbox->value());
    writeDescription();
}


void PotdPreview::on_right_margin_spinbox_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    ui->right_margin_spinbox->setMaximum(gv.screenAvailableWidth-300-ui->left_margin_spinbox->value());
    writeDescription();
}

void PotdPreview::on_bottom_top_margin_spinbox_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    writeDescription();
}
