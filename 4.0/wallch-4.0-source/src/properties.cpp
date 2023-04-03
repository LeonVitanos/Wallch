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

#include <QDir>
#include <QShortcut>
#include <QDesktopWidget>
#include <QImageReader>

Properties::Properties(const QString &img, bool showNextPrevious, int currentIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::properties)
{
    ui->setupUi(this);

    //Moving the window to the center of screen!
    this->move(QApplication::desktop()->availableGeometry().center() - this->rect().center());

    nextPreviousTimer_ = new QTimer(this);
    nextPreviousTimer_->setSingleShot(true);
    connect(nextPreviousTimer_, SIGNAL(timeout()), this, SLOT(uncheckButtons()));

    updateEntries(img, currentIndex);

    if(!showNextPrevious){
        ui->previous->hide();
        ui->next->hide();
    }
    else
    {
        (void) new QShortcut(Qt::Key_Right, this, SLOT(simulateNext()));
        (void) new QShortcut(Qt::Key_Left, this, SLOT(simulatePrevious()));
    }

    ui->previous->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/icons/Pictures/media-seek-backward.svg")));
    ui->next->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/icons/Pictures/media-seek-forward.svg")));
}

Properties::~Properties()
{
    if(nextPreviousTimer_->isActive()){
        nextPreviousTimer_->stop();
    }
    delete ui;
}

void Properties::on_close_clicked()
{
    close();
}

void Properties::resizeEvent(QResizeEvent *)
{
    QSize scaledSize = currentPixmap_.size();
    scaledSize.scale(ui->label->size(), Qt::KeepAspectRatio);
    if (!ui->label->pixmap() || scaledSize != ui->label->pixmap()->size()){
        updateScreenshotLabel();
    }
}

void Properties::updateScreenshotLabel()
{
    ui->label->setPixmap(currentPixmap_.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void Properties::updateEntries(const QString &filename, int currentIndex){
    currentFilename_=filename;
    currentIndex_=currentIndex;
    QImageReader reader(filename);
    if(reader.canRead()){
        QString originalWidth = QString::number(reader.size().width());
        QString originalHeight = QString::number(reader.size().height());

        bool resize=false;
        double scaleFactorX=0, scaleFactorY=0;
        if(reader.size().width()>gv.screenWidth){
            resize=true;
            scaleFactorX=gv.screenWidth*1.0/reader.size().width();
        }
        if(reader.size().height()>gv.screenHeight){
            resize=true;
            scaleFactorY=gv.screenHeight*1.0/reader.size().height();
        }
        if(resize)
        {
            if(scaleFactorX==0 || (scaleFactorY<scaleFactorX)){
                reader.setScaledSize(reader.size()*scaleFactorY);
            }
            else if(scaleFactorY==0 || (scaleFactorX<scaleFactorY)){
                reader.setScaledSize(reader.size()*scaleFactorX);
            }
        }

        QFileInfo imageInfo(filename);

        ui->nameLineEdit->setText(Global::basenameOf(filename));
        ui->type_label->setText(imageInfo.suffix().toUpper());
        ui->dimensionsLabel->setText(originalWidth + " x " + originalHeight);
        ui->size_label->setText(sizeToNiceString(QFile(filename).size()));
        ui->location_label->setText(Global::dirnameOf(filename));
        ui->created_label->setText(imageInfo.created().toString("ddd, MMM d yyyy hh:mm:ss"));
        ui->modified_label->setText(imageInfo.lastModified().toString("ddd, MMM d yyyy hh:mm:ss"));

        currentPixmap_ = QPixmap().fromImage(reader.read());
        updateScreenshotLabel();
    }
    else
    {
        currentPixmap_ = QPixmap(QIcon::fromTheme("dialog-error").pixmap(100));
        updateScreenshotLabel();
    }
}

void Properties::on_next_clicked()
{
    Q_EMIT requestNext(currentIndex_);
}

void Properties::on_previous_clicked()
{
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
    Global::setBackground(currentFilename_, true, true, 1);
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
    Global::openFolderOf(currentFilename_);
}
