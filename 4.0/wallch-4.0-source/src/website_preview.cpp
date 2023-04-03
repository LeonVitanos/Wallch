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

#include "website_preview.h"
#include "ui_website_preview.h"

#include <QDir>
#include <QShortcut>

WebsitePreview::WebsitePreview(WebsiteSnapshot *websiteSnapshotP, bool show_crop_dialog, bool crop, QRect cropArea, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::website_preview)
{
    //removing the close button (X) to make it clear that you can only cancel the process...
    this->setWindowFlags(((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint));

    ui->setupUi(this);
    ui->secondary_label->hide();

    forCropDialog_=show_crop_dialog;
    curCropArea_=cropArea;

    (void) new QShortcut(Qt::Key_Escape, this, SLOT(on_cancel_or_close_clicked()));

    websiteSnapshot_=websiteSnapshotP;

    websiteSnapshot_->setCrop(crop, curCropArea_);

    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(imageReady(QImage*,short)));

    timeout_=WEBSITE_TIMEOUT;

    ui->preview_progress->show();
    ui->counter_label->show();
    ui->counter->show();
    ui->counter->setText(QString::number(WEBSITE_TIMEOUT));

    countdownTimer_ = new QTimer(this);
    connect(countdownTimer_, SIGNAL(timeout()), this, SLOT(reduceTimeoutByOne()));
    countdownTimer_->start(1000);

    websiteSnapshot_->start();
}

WebsitePreview::~WebsitePreview()
{
    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(imageReady(QImage*,short)));
    delete ui;
}

void WebsitePreview::reduceTimeoutByOne(){
    ui->counter->setText(QString::number(--timeout_));
    if(timeout_<=0){
        if(countdownTimer_->isActive()){
            countdownTimer_->stop();
        }
    }
}

void WebsitePreview::imageReady(QImage *image, short errorCode){
    if(countdownTimer_->isActive()){
        countdownTimer_->stop();
    }
    if(errorCode==0 && !image->isNull()){
        //no error!
        if(forCropDialog_){
            beginCropDialog(image);
        }
        else
        {
            //just set it as preview
            Q_EMIT previewImageReady(image);
            closeDialog();
        }
    }
    else
    {
        QString further_info;
        switch(errorCode){
            case 1:
                further_info=tr("Some of the requested pages\nfailed to load successfully.");
                break;
            case 2:
                further_info=tr("Simple authentication failed.\nPlease check your username and/or password.");
                break;
            case 3:
                further_info=tr("Username and/or password fields are not found.\nPlease check that you are pointing at the login page.");
                break;
            case 4:
                further_info=tr("The timeout has been reached and the\nimage has yet to be created!");
                break;
            default:
                further_info=tr("Unknown error! Please try with\na different web page.");
                break;
        }
        failWithMessage(further_info);
    }
}

void WebsitePreview::closeDialog(){
    ui->main_label->setText(tr("Done!"));
    QTimer::singleShot(500, this, SLOT(on_cancel_or_close_clicked()));
}

void WebsitePreview::failWithMessage(const QString &further_info){
    ui->preview_progress->hide();
    ui->counter_label->hide();
    ui->counter->hide();
    ui->main_label->setText(tr("Failed!"));
    ui->secondary_label->show();
    ui->secondary_label->setText(further_info);
    ui->cancel_or_close->setText(tr("Close"));
    this->adjustSize();
}

void WebsitePreview::on_cancel_or_close_clicked()
{
    if(websiteSnapshot_->isLoading()){
        websiteSnapshot_->stop();
    }
    this->close();
}

void WebsitePreview::beginCropDialog(QImage *image){
    cropImageDialog_ = new CropImage(image, curCropArea_, this);
    connect(cropImageDialog_, SIGNAL(coordinates(QRect)), this, SLOT(sendCoordinates(QRect)));
    this->close();
    cropImageDialog_->exec();
    cropImageDialog_->deleteLater();
}

void WebsitePreview::sendCoordinates(const QRect &coords){
    //sending the coordinates back to mainwindow.
    Q_EMIT sendExtraCoordinates(coords);
}
