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

#include "properties.h"
#include "ui_properties.h"
#include "glob.h"
#include "wallpapermanager.h"

#include <QDir>
#include <QShortcut>
#include <QScreen>
#include <QImageReader>
#include <QImage>
#include <QtConcurrent/QtConcurrentRun>

#define BYTES_PER_KiB 1024.0
#define BYTES_PER_MiB 1048576.0

Properties::Properties(const QString &image, bool showNextPrevious, int currentIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::properties)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    propertiesReadyWatcher_ = new QFutureWatcher<QImage>(this);
    connect(propertiesReadyWatcher_, SIGNAL(finished()), this, SLOT(propertiesReady()));

    nextPreviousTimer_ = new QTimer(this);
    nextPreviousTimer_->setSingleShot(true);
    connect(nextPreviousTimer_, SIGNAL(timeout()), this, SLOT(uncheckButtons()));

    if(!showNextPrevious){
        ui->previous->hide();
        ui->next->hide();
    }
    else
    {
        (void) new QShortcut(Qt::Key_Right, this, SLOT(simulateNext()));
        (void) new QShortcut(Qt::Key_Left, this, SLOT(simulatePrevious()));
    }

    ui->previous->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/images/media-seek-backward.png")));
    ui->next->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/images/media-seek-forward.png")));

    updateEntries(image, currentIndex);
}

Properties::~Properties()
{
    if(nextPreviousTimer_->isActive()){
        nextPreviousTimer_->stop();
    }
    delete ui;
}

void Properties::propertiesReady(){
    if(currentImageWidth_ > 0 && currentImageHeight_ > 0){
        ui->dimensionsLabel->setText(QString::number(currentImageWidth_) + " x " + QString::number(currentImageHeight_));
    }

    updatePropertiesImageLabel(propertiesReadyWatcher_->future().result());
}

QImage Properties::resizePreview(){
    QImageReader reader(currentFilename_);
    reader.setDecideFormatFromContent(true);
    if(reader.canRead()){
        currentImageWidth_ = reader.size().width();
        currentImageHeight_ = reader.size().height();

        if(currentImageWidth_>(gv.screenWidth) || currentImageHeight_>(gv.screenHeight)){
            float scaleFactor;
            if((currentImageWidth_-(gv.screenWidth)) > (currentImageHeight_-(gv.screenHeight))){
                scaleFactor = (gv.screenWidth*1.0)/currentImageWidth_;
            }
            else
            {
                scaleFactor = (gv.screenHeight*1.0)/currentImageHeight_;
            }
            reader.setScaledSize(QSize(reader.size())*scaleFactor);
        }

        QImage image = reader.read();

        if(image.isNull()){
            image = QImage(currentFilename_);
            if(image.isNull()){
                return QIcon::fromTheme("dialog-error").pixmap(100).toImage();
            }
        }

        return image;
    }
    else
    {
        currentImageWidth_ = -1;
        currentImageHeight_ = -1;
        return QIcon::fromTheme("dialog-error").pixmap(100).toImage();
    }

}

void Properties::on_close_clicked()
{
    close();
}

void Properties::resizeEvent(QResizeEvent *)
{
    if(justOpened_){
        justOpened_ = false;
        //Moving the window to the center of screen, now that the labels have been set
        //(large labels, like path, may resize the window, thus now is the correct time to center it)
        this->move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center());
    }

    if(ui->propertiesImage->pixmap(Qt::ReturnByValue).isNull())
        return;

    QSize scaledSize = currentImage_.size();
    scaledSize.scale(ui->propertiesImage->size(), Qt::KeepAspectRatio);
    if (scaledSize != ui->propertiesImage->pixmap(Qt::ReturnByValue).size())
        updatePropertiesImageLabel();
}

void Properties::updatePropertiesImageLabel(){

    ui->propertiesImage->setPixmap(Global::roundedCorners(currentImage_.scaled(ui->propertiesImage->size(),
                                                                               Qt::KeepAspectRatio, Qt::SmoothTransformation), 6));
}

void Properties::updatePropertiesImageLabel(QImage image)
{
    currentImage_ = image;
    ui->propertiesImage->setPixmap(Global::roundedCorners(image.scaled(ui->propertiesImage->size(),
                                                                       Qt::KeepAspectRatio, Qt::SmoothTransformation), 6));
}

void Properties::updateEntries(const QString &filename, int currentIndex){
    currentFilename_ = filename;
    currentIndex_ = currentIndex;

    ui->propertiesImage->clear();
    ui->propertiesImage->setText(tr("Loading..."));

    QFileInfo imageInfo(currentFilename_);
    ui->type_label->setText(imageInfo.suffix().toUpper());
    ui->created_label->setText(imageInfo.created().toString("ddd, MMM d yyyy hh:mm:ss"));
    ui->modified_label->setText(imageInfo.lastModified().toString("ddd, MMM d yyyy hh:mm:ss"));

    ui->nameLineEdit->setText(Global::basenameOf(currentFilename_));
    ui->size_label->setText(sizeToNiceString(QFile(currentFilename_).size()));
    ui->location_label->setText(Global::dirnameOf(currentFilename_));
    ui->dimensionsLabel->setText("-");

    propertiesReadyWatcher_->setFuture(QtConcurrent::run(this, &Properties::resizePreview));
}

void Properties::on_next_clicked()
{
    if(propertiesReadyWatcher_->isRunning()){
        propertiesReadyWatcher_->cancel();
    }
    Q_EMIT requestNext(currentIndex_);
}

void Properties::on_previous_clicked()
{
    if(propertiesReadyWatcher_->isRunning()){
        propertiesReadyWatcher_->cancel();
    }
    Q_EMIT requestPrevious(currentIndex_);
}

void Properties::simulateNext(){
    //simulates the next button to be pressed down for a bit!
    ui->previous->setDown(false);
    ui->next->setDown(false);
    ui->next->setDown(true);
    on_next_clicked();
    if(nextPreviousTimer_->isActive()){
        nextPreviousTimer_->stop();
    }
    nextPreviousTimer_->start(200);
}

void Properties::simulatePrevious(){
    ui->previous->setDown(false);
    ui->next->setDown(false);
    ui->previous->setDown(true);
    on_previous_clicked();
    if(nextPreviousTimer_->isActive()){
        nextPreviousTimer_->stop();
    }
    nextPreviousTimer_->start(200);
}

void Properties::uncheckButtons(){
    ui->next->setDown(false);
    ui->previous->setDown(false);
}

void Properties::on_set_as_background_clicked()
{
    WallpaperManager::setBackground(currentFilename_, true, true, 1);
    if(gv.setAverageColor){
        Q_EMIT averageColorChanged();
    }
}

QString Properties::sizeToNiceString(qint64 fsize){
    if(fsize>=BYTES_PER_MiB){
        return QString::number(fsize/BYTES_PER_MiB, 'f', 1) + " MiB";
    }
    else{
        return QString::number(fsize/BYTES_PER_KiB, 'f', 1) + " KiB";
    }
}

void Properties::on_open_location_button_clicked()
{
    WallpaperManager::openFolderOf(currentFilename_);
}
