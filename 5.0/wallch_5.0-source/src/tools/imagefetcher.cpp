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

#include "imagefetcher.h"
#include "wallpapermanager.h"
#include "markitem.h"
#include "glob.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QTextDocument>
#include <QImage>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QRegularExpression>

ImageFetcher::ImageFetcher(QObject *parent) :
    QObject(parent)
{
    fileDownloader_ = new QNetworkAccessManager(this);
}

void ImageFetcher::fetch(){
    switch(fetchType_){
    case FetchType::POTD:
        potdDownload();
        break;
    default:
    case FetchType::LE:
        leDownload();
        break;
    case FetchType::General:
        if(!buffered_ && (imageFilename_.isEmpty() || imageUrl_.isEmpty())){
            Global::error("No image filename and/or url has been set. Set a filename and a url or");
            return;
        }
        generalDownload();
        break;
    }
}

void ImageFetcher::setFetchType(FetchType::Value fetchType){
    fetchType_ = fetchType;
}

void ImageFetcher::setBuffered(bool buffered){
    //set to true if you wish to download to memory instead of disk
    if(currentNetworkRequest_){
        if(currentNetworkRequest_->isRunning()){
            Global::error("Cannot set the buffered variable while a request is running!");
            return;
        }
    }
    buffered_ = buffered;
    if(imageBuffer_ == NULL){
        imageBuffer_ = new QByteArray();
    }
}

void ImageFetcher::releaseBuffer(){
    //releases the buffer of the last downloaded image
    if(imageBuffer_ != NULL){
        imageBuffer_->clear();
        delete imageBuffer_;
        imageBuffer_ = NULL;
    }
}

FetchType::Value ImageFetcher::fetchType(){
    return fetchType_;
}

void ImageFetcher::setFilename(const QString &imageFilename){
    //used when the fetch type is general
    imageFilename_ = imageFilename;
}

void ImageFetcher::potdDownload(){
    //searching whether the link for current day is saved at settings
    settings->beginGroup("potd_links");
    potdDescriptionDate_ = QDate::currentDate();
    QString savedOnlineLink = settings->value(potdDescriptionDate_.toString("dd.MM.yyyy"), "").toString();
    settings->endGroup();

    if(savedOnlineLink.isEmpty()){
        alreadyTriedAlternativeLinkToDropbox_ = false;
        downloadTextFileContainingImage(gv.potdOnlineUrl);
    }
    else
    {
        downloadOnlineImage(savedOnlineLink);
    }
}

void ImageFetcher::leDownload(){
    alreadyTriedAlternativeLinkToDropbox_ = false;
    downloadTextFileContainingImage(gv.liveEarthOnlineUrl);
}

void ImageFetcher::generalDownload(){
    downloadOnlineImage(imageUrl_);
}

void ImageFetcher::downloadOnlineImage(const QString &onlineLink){
    disconnectFileDownloader();

    currentNetworkRequest_ = fileDownloader_->get(QNetworkRequest(QUrl(onlineLink)));

    if(!buffered_){
        if(fetchType_ == FetchType::LE){
            Global::remove(gv.wallchHomePath+LE_IMAGE+"*");
            imageFile_.setFileName(gv.wallchHomePath+LE_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+'.'+Global::suffixOf(currentNetworkRequest_->url().toString()));
        }
        else
        {
            Global::remove(gv.wallchHomePath+POTD_IMAGE+"*");
            imageFile_.setFileName(gv.wallchHomePath+POTD_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+'.'+Global::suffixOf(currentNetworkRequest_->url().toString()));
        }

        if(!imageFile_.open(QIODevice::WriteOnly)){
            Global::error("There was a problem while trying to write to "+imageFile_.fileName());
            currentNetworkRequest_->abort();
            downloadFailed();
        }
    }

    connect(currentNetworkRequest_, SIGNAL(readyRead()), this, SLOT(writeImage()));
    connect(currentNetworkRequest_, SIGNAL(finished()), this, SLOT(imageDownloadFinished()));
    connect(currentNetworkRequest_, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(reportDownloadProgress(qint64,qint64)));
    connect(currentNetworkRequest_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadFailed(QNetworkReply::NetworkError)));
}

void ImageFetcher::writeImage(){
    if(buffered_){
        imageBuffer_->append(currentNetworkRequest_->readAll());
    }
    else
    {
        imageFile_.write(currentNetworkRequest_->readAll());
    }
}

void ImageFetcher::reportDownloadProgress(qint64 bytesReceived, qint64 bytesTotal){
    unsigned int progress = qRound( ((double) bytesReceived / (double) bytesTotal) * 100 );
    if(progress > 100){
        progress = 100;
    }
    Q_EMIT downloadProgress(progress);
}

void ImageFetcher::downloadFailed(QNetworkReply::NetworkError error /* = QNetworkReply::NoError */){
    Q_EMIT fail();
    if(error != QNetworkReply::NoError){
        Global::error("Could not process your request: "+currentNetworkRequest_->errorString());
    }
    if(currentNetworkRequest_){
        currentNetworkRequest_->deleteLater();
    }
}

void ImageFetcher::disconnectFileDownloader(){
    //we use the same object for different operations so we disconnect from all of them before reusing it
    disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(getPotdDescription(QNetworkReply*)));
    disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(readFileContainingImage(QNetworkReply*)));
}

void ImageFetcher::downloadTextFileContainingImage(const QString &url){
    //downloads the plain text file that contains the direct link to the image
    disconnectFileDownloader();
    QObject::connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(readFileContainingImage(QNetworkReply*)));
    currentNetworkRequest_ = fileDownloader_->get(QNetworkRequest(QUrl(url)));
}

void ImageFetcher::imageDownloadFinished()
{
    if(fetchType_ == FetchType::POTD){
        settings->setValue("previous_img_url", currentNetworkRequest_->url().toString());
        settings->setValue("last_day_potd_was_set", QDateTime::currentDateTime().toString("dd.MM.yyyy"));
        settings->setValue("potd_preferences_have_changed", false);
        settings->sync();
    }

    currentNetworkRequest_->deleteLater();

    if(buffered_){
        Q_EMIT successBuf(imageBuffer_);
    }
    else
    {
        QString filename = imageFile_.fileName();
        imageFile_.close();

        currentNetworkRequest_->deleteLater();

        if(fetchType_ == FetchType::POTD && gv.potdIncludeDescription){
            potdDescriptionFilename_ = filename;
            downloadPotdDescription();
        }
        else
        {
            if (fetchType_ == FetchType::LE && gv.leEnableTag) {
                applyLeTags(filename);
            }
            Q_EMIT success(filename);
        }
    }

}

void ImageFetcher::applyLeTags(const QString &filename)
{
    QImage leImage(filename);

    // a scaled down image (800px wide) is used on lepoint, so we need the ratio
    qreal ratio = leImage.width() / 800.0;

    QGraphicsScene scene;
    QPixmap background = QPixmap(filename);

    QGraphicsPixmapItem *backgroundItem = new QGraphicsPixmapItem();
    backgroundItem->setPixmap(background);

    scene.setSceneRect(background.rect());
    scene.addItem(backgroundItem);

    QSettings settings("wallch", "Settings");

    unsigned int imagesCount = settings.beginReadArray("le_mark_points");

    if (imagesCount == 0) {
        // load default settings
        MarkItem *pointItem = new MarkItem();
        pointItem->setPointType(PointItemType::Point1);
        pointItem->setScale(0.5 * ratio);
        pointItem->setPos(100 * ratio, 100 * ratio);
        scene.addItem(pointItem);
    } else {
        for (unsigned int i = 0; i < imagesCount; i++) {
            settings.setArrayIndex(i);

            MarkItem *pointItem = new MarkItem();
            pointItem->setPointType(PointItemType::CustomPoint, settings.value("icon", ":/images/point1.png").toString());
            pointItem->setScale(settings.value("scale", 1).toReal() * ratio);
            pointItem->setRotation(settings.value("rotation", 0).toReal());

            QPointF pos = settings.value("pos", QPoint(100, 100)).toPoint() * ratio;

            pointItem->setPos(pos.x(), pos.y());

            scene.addItem(pointItem);
        }
    }

    settings.endArray();

    QImage image(scene.sceneRect().size().toSize(), QImage::Format_ARGB32);  // Create the image with the exact size of the shrunk scene
    image.fill(Qt::transparent);                                              // Start all pixels transparent

    QPainter painter(&image);
    scene.render(&painter);
    image.save(filename, "JPEG", 100);
}

void ImageFetcher::downloadPotdDescription(){
    disconnectFileDownloader();
    connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(getPotdDescription(QNetworkReply*)));
    QString urlToGetDescription;
    if((potdDescriptionDate_.year() == 2004 && (potdDescriptionDate_.month() == 11 || potdDescriptionDate_.month() == 12)) || potdDescriptionDate_.year() == 2005 || potdDescriptionDate_.year() == 2006){
        urlToGetDescription = "https://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day/"+  Global::monthInEnglish(potdDescriptionDate_.month()) + potdDescriptionDate_.toString("_d,_yyyy");
    }
    else
    {
        urlToGetDescription = "https://en.wikipedia.org/wiki/Template:POTD/"+potdDescriptionDate_.toString("yyyy-MM-dd");
    }

    currentNetworkRequest_ = fileDownloader_->get(QNetworkRequest(QUrl(urlToGetDescription)));
}

void ImageFetcher::getPotdDescription(QNetworkReply *reply){
    currentNetworkRequest_->deleteLater();
    if(reply->error()){
        //set potd without the description
        Global::error("Could not get picture of the day description. Setting image as is.");
        Q_EMIT success(potdDescriptionFilename_);
        return;
    }
    QString potdDescription(reply->readAll());
    reply->deleteLater();

    if(potdDescription.contains("/wiki/File:"))
    {
        QRegularExpression filter("<a href=\"/wiki/File:(.+)</a></small>");
        QRegularExpressionMatch match = filter.match(potdDescription);
        if(match.hasMatch())
            potdDescription=match.captured(1);
    }
    else
    {
        Global::error("Could not get picture of the day description. Setting image as is.");
        Q_EMIT success(potdDescriptionFilename_);
        return;
    }

    if(potdDescription.contains("<p>") && potdDescription.contains("</small>"))
    {
        QRegularExpression filter("<p>(.+)</small>");
        QRegularExpressionMatch match = filter.match(potdDescription);
        if(match.hasMatch())
            potdDescription=match.captured(1);
    }
    else
    {
        Global::error("Could not get picture of the day description. Setting image as is.");
        Q_EMIT success(potdDescriptionFilename_);
        return;
    }
    potdDescription.replace("/wiki", "https://en.wikipedia.org/wiki").remove(QRegularExpression("<[^>]*>")).replace("\n", " ");
    QtConcurrent::run(this, &ImageFetcher::writePotdDescription, replaceSpecialHtml(potdDescription));
}

void ImageFetcher::writePotdDescription(const QString &description){
    QImage image(potdDescriptionFilename_);

    if(image.format() == QImage::Format_Indexed8){
        image = WallpaperManager::indexed8ToARGB32(image);
    }

    QPainter painter(&image);

    float divImageWidth = (float)image.width()/(float)gv.screenAvailableWidth;
    float divImageHeight = (float)image.height()/(float)gv.screenAvailableHeight;

    QFont font;
    font.setFamily(gv.potdDescriptionFont);
    font.setPixelSize(image.width()/55);

    painter.setFont(font);

    QRect drawRect = painter.fontMetrics().boundingRect(QRect(0, 0, image.width()-(divImageWidth*gv.potdDescriptionLeftMargin+divImageWidth*gv.potdDescriptionRightMargin), image.height()), (Qt::TextWordWrap | Qt::AlignCenter), description);
    drawRect.setX(0);
    drawRect.setWidth(image.width());

    int newY;
    if(gv.potdDescriptionBottom){
        newY = image.height()-drawRect.height()-divImageHeight*gv.potdDescriptionBottomTopMargin;
    }
    else
    {
        newY = divImageHeight*gv.potdDescriptionBottomTopMargin;
    }
    int oldH = drawRect.height();
    drawRect.setY(newY);
    drawRect.setHeight(oldH);

    QColor backgroundColor = QColor(gv.potdDescriptionBackgroundColor);
    backgroundColor.setAlpha(176);

    painter.fillRect(drawRect, QBrush(backgroundColor));

    drawRect.setX(divImageWidth*gv.potdDescriptionLeftMargin);
    drawRect.setWidth(image.width()-(divImageWidth*gv.potdDescriptionLeftMargin+divImageWidth*gv.potdDescriptionRightMargin));

    painter.setPen(QColor(gv.potdDescriptionColor));
    painter.drawText(drawRect, (Qt::TextWordWrap | Qt::AlignCenter), description, &drawRect);

    painter.end();
    image.save(potdDescriptionFilename_, "JPEG", 100);
    Q_EMIT success(potdDescriptionFilename_);
}

void ImageFetcher::tryDownloadingImagesFromAlternativeLink(){
    alreadyTriedAlternativeLinkToDropbox_ = true;
    if(fetchType_ == FetchType::POTD){
        downloadTextFileContainingImage(gv.potdOnlineUrlB);
    }
    else
    {
        downloadTextFileContainingImage(gv.liveEarthOnlineUrlB);
    }
}

void ImageFetcher::readFileContainingImage(QNetworkReply *reply){
    currentNetworkRequest_->deleteLater();
    if (reply->error()){
        if(!alreadyTriedAlternativeLinkToDropbox_){
            tryDownloadingImagesFromAlternativeLink();
        }
        else
        {
            Q_EMIT fail();
            Global::error("Could not process your request: "+reply->errorString());
            reply->deleteLater();
        }
        return;
    }
    QString onlineLink(reply->readAll());

    reply->deleteLater();

    if(fetchType_ == FetchType::POTD){
        //potd has multiple links in it
        QStringList linksDates = onlineLink.split(QRegularExpression("[ \n]"),QString::SkipEmptyParts);
        if(linksDates.count() != 6){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                Global().desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT fail();
            }
            return;
        }

        if(!linksDates.at(1).startsWith("http")){
            //something's wrong!
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                Global().desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT fail();
            }
            return;
        }
        settings->remove("potd_links");
        settings->beginGroup("potd_links");
        bool linkFound = false;
        QString curDate = QDate::currentDate().toString("dd.MM.yyyy");
        for(short i = 0; i < 6; i += 2){
            if(!linkFound){
                if(curDate == linksDates.at(i)){
                    linkFound = true;
                    onlineLink = linksDates.at(i+1);
                }
            }
            settings->setValue(linksDates.at(i), linksDates.at(i+1));
        }
        settings->endGroup();
        settings->sync();
        if(!linkFound){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                Global().desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT fail();
            }
            return;
        }
    }
    else if(fetchType_ == FetchType::LE)
    {
        if(!onlineLink.startsWith("http:")){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                Global::error("Live Earth image download failed!");
                Q_EMIT fail();
            }
            return;
        }
    }

    onlineLink.replace('\n', "");

    gv.onlineLinkForHistory = onlineLink;

    if(fetchType_ == FetchType::LE){
        Global::remove(gv.wallchHomePath+LE_IMAGE+"*");
    }
    else if(fetchType_ == FetchType::POTD){
        QString previousImageURL = settings->value("previous_img_url", "").toString();
        if(previousImageURL == onlineLink){
            //image has not yet changed, just skip it for the next day :D
            Global::debug("Wikipedia Picture of the day picture has yet to be updated. Setting the same image...");

            QString filename = Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
            if(!filename.isEmpty()){
                Q_EMIT success(filename);
                return;
            }
            else
            {
                Global::debug("Redownloading the image...");
            }
        }
    }

    downloadOnlineImage(onlineLink);
}

QString ImageFetcher::replaceSpecialHtml(const QString &html){
    QTextDocument textDocument;
    textDocument.setHtml(html);
    return textDocument.toPlainText();
}

void ImageFetcher::abort(){
    if(currentNetworkRequest_ && !currentNetworkRequest_->isFinished()){
        currentNetworkRequest_->abort();
    }
}
