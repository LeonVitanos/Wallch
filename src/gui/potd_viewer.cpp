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

#include "potd_viewer.h"
#include "ui_potd_viewer.h"
#include "glob.h"

#include <QtWidgets>
#include <QtNetwork>

PotdViewer::PotdViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::potd_viewer)
{
    ui->setupUi(this);

    selectedDate_=QDate::currentDate();
    skipStepsAfterThree_=false;
    forceStop_=false;

    ui->previousButton->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/images/media-seek-backward.png")));
    ui->nextButton->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/images/media-seek-forward.png")));

    ui->save_progressBar->hide();
    ui->dateEdit->setDate(selectedDate_);

    (void) new QShortcut(Qt::Key_Right, this, SLOT(on_nextButton_clicked()));
    (void) new QShortcut(Qt::Key_Left, this, SLOT(on_previousButton_clicked()));
}

PotdViewer::~PotdViewer()
{
    if(reply_){
        reply_->deleteLater();
    }
    if(originalMovie_){
        originalMovie_->deleteLater();
    }
    delete ui;
}

void PotdViewer::resizeEvent(QResizeEvent *)
{
    if(ui->label->pixmap(Qt::ReturnByValue).isNull())
        return;

    QSize scaledSize = originalImage_.size();
    scaledSize.scale(ui->label->size(), Qt::KeepAspectRatio);
    if (scaledSize != ui->label->pixmap(Qt::ReturnByValue).size())
        updateLabel();
}

void PotdViewer::updateLabel()
{
    if(directLinkOfFullImage_.endsWith("gif")){
        ui->label->setMovie(originalMovie_);
        originalMovie_->start();
    }
    else
        ui->label->setPixmap(Global::roundedCorners(originalImage_.scaled(ui->label->size(),
                                                                          Qt::KeepAspectRatio, Qt::SmoothTransformation), 5));
}

void PotdViewer::urlQDate()
{
    /*
     * Step 1
     * Find the html link, and download its source
     */

    ui->saveimageButton->setEnabled(false);
    ui->save_progressBar->hide();

    htmlSource_.clear();

    if((selectedDate_.year()==2004 && (selectedDate_.month()==11 || selectedDate_.month()==12)) || selectedDate_.year()==2005 || selectedDate_.year()==2006)
        url_ = "https://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day/"+  Global::monthInEnglish(selectedDate_.month()) + selectedDate_.toString("_d,_yyyy");
    else
        url_ = "https://en.wikipedia.org/wiki/Template:POTD/"+selectedDate_.toString("yyyy-MM-dd");

    // schedule the request
    startRequest(url_);
}

void PotdViewer::startRequest(QUrl url)
{
    if(reply_){
        reply_->deleteLater();
        reply_=NULL;
    }

    reply_ = qnam_.get(QNetworkRequest(url));
    connect(reply_, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
}

void PotdViewer::startRequestAlternative(QUrl url)
{
    if(replyAlt_){
        replyAlt_->deleteLater();
        replyAlt_=NULL;
    }

    replyAlt_ = new QProcess(this);
    replyAlt_->start("wget", QStringList() << "-O" << QDir(QDir::currentPath()).filePath("potd") << url.toString());
    connect(replyAlt_ , SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(httpFinished()));
    connect(replyAlt_ , SIGNAL(error(QProcess::ProcessError)), this, SLOT(httpErrorOccured()));
}

void PotdViewer::movieDestroyed(){
    originalMovie_=NULL;
}

void PotdViewer::httpErrorOccured(){
    enableWidgets(true);
    ui->label->setText(tr("Check your internet connection or try another date. Maybe Wikipedia\nhasn't selected a picture of the day for")+" "+ui->dateEdit->date().toString());
    ui->potdDescription->clear();
    QMessageBox::information(this, "HTTPS",tr("Download failed")+": "+reply_->errorString()+".");
}

void PotdViewer::httpFinished()
{
    /*
     * Step 2
     * When finished check for errors or redirection page (tough wikipedia has not redirection you never know)
     */

    if(!reply_ && !replyAlt_)
        return;

    if (reply_ && reply_->error()){
        QUrl url = reply_->url();
        reply_->deleteLater();
        reply_ = NULL;
        startRequestAlternative(url);
        return;
    }

    if(reply_){
        QVariant redirectionTarget = reply_->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (!redirectionTarget.isNull()) {
            QUrl newUrl = url_.resolved(redirectionTarget.toUrl());
            url_ = newUrl;
            reply_->deleteLater();
            startRequest(url_);
            return;
        }
        reply_->deleteLater();
        reply_ = NULL;
    }
    else{
        QFile f("potd");
        if (!f.open(QFile::ReadOnly | QFile::Text)) return;
        QTextStream in(&f);
        htmlSource_ = in.readAll();
        replyAlt_->deleteLater();
        replyAlt_ = NULL;
    }

    if(!skipStepsAfterThree_)
    {
        /*
         * Step 3
         * Get the image's link of the html source downloaded.
         * The link collected here will still not be a direct link to the image
         */

        QString imageSummix;
        int imageSummixCount=0;
        if(htmlSource_.contains("/wiki/File:"))
        {
            QRegularExpression filter("/wiki/File:(.+)");
            QRegularExpressionMatch match = filter.match(htmlSource_);
            if(match.hasMatch())
                htmlSource_=match.captured(1);

            while (!imageSummix.contains("\""))
            {
                imageSummixCount++;
                imageSummix=htmlSource_.left(imageSummixCount);
            }
        }
        else
        {
            enableWidgets(true);
            ui->label->clear();
            QMessageBox::information(this, tr("Error!"),tr("Couldn't find image url. Try another date!"));
            return;
        }

        /*
         * Step 4
         * Find the desctription of the current potd
         */

        if(htmlSource_.contains("<p>") && htmlSource_.contains("</small>"))
        {
            QRegularExpression filter2("<p>(.+)</small>");
            QRegularExpressionMatch match = filter2.match(htmlSource_);
            if(match.hasMatch()){
                htmlSource_=match.captured(1);
            }
            htmlSource_.replace("/wiki", "https://en.wikipedia.org/wiki");
            ui->potdDescription->setText(htmlSource_);
        }
        else if(htmlSource_.contains("<p>") && htmlSource_.contains("</p>")){
            QRegularExpression filter2("<p>(.+)</p>");
            QRegularExpressionMatch match = filter2.match(htmlSource_);
            if(match.hasMatch()){
                htmlSource_=match.captured(1);
            }
            htmlSource_.replace("/wiki", "https://en.wikipedia.org/wiki");
            ui->potdDescription->setText(htmlSource_);
        }
        else
        {
            ui->potdDescription->setText(tr("We couldn't find the description for this date")+"...");
        }

        /*
         * Step 5
         * We will download the html source of the undirect link to the image
         * therefore we create a bool to skip steps three-four-five
        */

        skipStepsAfterThree_=true;
        url_="https://en.wikipedia.org/wiki/File:"+imageSummix.left(imageSummixCount-1);

        if(reply_)
            startRequest(url_);
        else
            startRequestAlternative(url_);
    }
    else
    {
        skipStepsAfterThree_=false;

        /*
         * Step 6
         * Find the direct link to the full size image and to the preview image from the second html source we downloaded
        */

        if(htmlSource_.contains("Deleted_photo.png"))
            directLinkOfFullImage_=directLinkOfPreviewImage_="https://upload.wikimedia.org/wikipedia/commons/e/e0/Deleted_photo.png";
        else
        {
            if(htmlSource_.contains("fullImageLink"))
            {
                QRegularExpression filter("fullImageLink\" id=\"file\"><a href=\"(.+)\"", QRegularExpression::InvertedGreedinessOption);
                QRegularExpressionMatch match = filter.match(htmlSource_);
                if(match.hasMatch())
                    directLinkOfFullImage_=match.captured(1);
                directLinkOfFullImage_="https:"+directLinkOfFullImage_;
            }
            else
            {
                enableWidgets(true);
                ui->label->clear();
                QMessageBox::information(this, tr("Error!"),tr("Couldn't find image url. Contact us, and include the specific date that shows this error!"));
                return;
            }

            if(htmlSource_.contains("Size of this preview")){
                QRegularExpression filter("Size of this preview: <a href=\"(.+)\"", QRegularExpression::InvertedGreedinessOption);
                QRegularExpressionMatch match = filter.match(htmlSource_);
                if(match.hasMatch())
                    directLinkOfPreviewImage_=match.captured(1);
                directLinkOfPreviewImage_="https:"+directLinkOfPreviewImage_;
            }
            else
                directLinkOfPreviewImage_=directLinkOfFullImage_;
        }

        /*
         * Step 7
         * Download the preview size image
         */

        ui->save_progressBar->setValue(0);
        ui->save_progressBar->show();
        ui->label->clear();

        if(reply_){
            reply_ = qnam_.get(QNetworkRequest(directLinkOfPreviewImage_));
            connect(reply_, SIGNAL(finished()),this, SLOT(imageDownloaded()));
            connect(reply_, SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(updateDataReadProgress(qint64,qint64)));
        }
        else{
            replyAlt_ = new QProcess(this);
            replyAlt_->start("wget", QStringList() << "-O" << QDir(QDir::currentPath()).filePath("potd") << directLinkOfPreviewImage_);

            connect(replyAlt_ , SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(imageDownloaded()));
        }
    }
}

void PotdViewer::imageDownloaded()
{
    /*
     * Step 8
     * Set the image to the label
     */

    if(!reply_ && !replyAlt_)
        return;

    if(directLinkOfFullImage_.endsWith("gif")){
        originalMovie_ = NULL;
        if(reply_){
            QBuffer *buffer = new QBuffer(this);
            buffer->open(QIODevice::WriteOnly);
            buffer->write(reply_->readAll());
            buffer->close();
            originalMovie_ = new QMovie(buffer, "GIF", this);
        }
        else{
            originalMovie_ = new QMovie(QDir(QDir::currentPath()).filePath("potd"), "GIF", this);
        }
        connect(originalMovie_, SIGNAL(destroyed()), this, SLOT(movieDestroyed()));
    }
    else
    {
        QImage image;
        if(reply_)
            image.loadFromData(reply_->readAll());
        else
            image = QImage(QDir(QDir::currentPath()).filePath("potd"));

        if(image.isNull())
            return;

        originalImage_ = image;
    }

    ui->save_progressBar->hide();
    ui->saveimageButton->setEnabled(true);
    enableWidgets(true);
    QTimer::singleShot(100, this, SLOT(updateLabel()));
}

void PotdViewer::httpReadyRead()
{
    /*
     * This slot gets called every time the QNetworkReply has new data.
     * All of the new data is being read and written into the file.
     * That way less RAM is used than if the reading was done at the
     * finished() signal of QNetworkReply
     */
    if(reply_){
        htmlSource_+=reply_->readAll();
    }
}

void PotdViewer::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
    ui->save_progressBar->setMaximum(totalBytes);
    ui->save_progressBar->setValue(bytesRead);
}

void PotdViewer::on_quitButton_clicked()
{
    close();
}

void PotdViewer::on_saveimageButton_clicked()
{
    //acts as save or cancel.
    if(!downloadingImage_ && QDate::fromString(settings->value("last_day_potd_was_set").toString(),"dd.MM.yyyy") == ui->dateEdit->date()){
        QString format = ".jpg";
        QString savePath = gv.homePath+"/potd_"+settings->value("last_day_potd_was_set", "unkownDate").toString()+format;
        QString filename = QFileDialog::getSaveFileName(this, tr("Save As"), savePath, format.toUpper()+" "+tr("Files")+" (*."+format+");"+tr("All Files")+" (*)");

        if(!filename.isEmpty()){
            if(QFile::exists(filename)){
                QFile::remove(filename);
            }
            if(!QFile::copy(Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*"), filename)){
                QMessageBox::warning(this, tr("Error"), tr("Something went wrong while saving the file!"));
            }
        }
    }
    else
    {
        if(downloadingImage_){
            //cancel has been pressed
            downloadingImage_ = false;
            ui->saveimageButton->setText(tr("Save"));
            httpRequestAborted_ = true;
            reply_->abort();
            ui->save_progressBar->hide();
            enableWidgets(true);
        }
        else
        {
            //save has been pressed
            downloadingImage_ = true;
            ui->saveimageButton->setText(tr("Cancel"));
            httpRequestAborted_ = false;
            QFileInfo link(directLinkOfFullImage_);
            formatOfImage_="."+link.suffix();
            ui->save_progressBar->setValue(0);
            ui->save_progressBar->show();
            enableWidgets(false);
            reply_ = qnam_.get(QNetworkRequest(directLinkOfFullImage_));
            connect(reply_, SIGNAL(finished()),this, SLOT(saveImage()));
            connect(reply_, SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(updateDataReadProgress_save(qint64,qint64)));
        }
    }
}

void PotdViewer::saveImage()
{
    if(reply_->error()){
        //aborted, through Cancel button or otherwise
        return;
    }
    downloadingImage_ = false;
    ui->saveimageButton->setText(tr("Save"));
    ui->save_progressBar->hide();
    enableWidgets(true);
    ui->saveimageButton->setEnabled(true);

    QString filename = QFileDialog::getSaveFileName(this, tr("Save As"),gv.homePath+"/potd_"+selectedDate_.toString("dd.MM.yyyy")+formatOfImage_,formatOfImage_.toUpper()+" "+tr("Files")+" (*."+formatOfImage_+");"+tr("All Files")+" (*)");
    if(!filename.isEmpty()){
        QFile file(filename);
        if(!file.open(QIODevice::WriteOnly)){
            return;
        }
        file.write(reply_->readAll());
        file.close();
    }
}

void PotdViewer::updateDataReadProgress_save(qint64 bytesRead, qint64 totalBytes)
{
    if(httpRequestAborted_){
        return;
    }

    ui->save_progressBar->setMaximum(totalBytes);
    ui->save_progressBar->setValue(bytesRead);
}

void PotdViewer::enableWidgets(bool state)
{
    ui->dateEdit->setEnabled(state);
    ui->nextButton->setEnabled(state);
    ui->previousButton->setEnabled(state);
}


void PotdViewer::on_dateEdit_dateChanged(const QDate &date_calendar)
{
    if(originalMovie_!=NULL){
        originalMovie_->deleteLater();
    }
    enableWidgets(false);
    selectedDate_=date_calendar;
    ui->label->setText(tr("Downloading")+"...");
    urlQDate();
}

void PotdViewer::on_previousButton_clicked()
{
    ui->dateEdit->setDate(selectedDate_.addDays(-1));
}

void PotdViewer::on_nextButton_clicked()
{
    ui->dateEdit->setDate(selectedDate_.addDays(+1));
}
