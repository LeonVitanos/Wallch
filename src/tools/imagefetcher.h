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

#ifndef IMAGEFETCHER_H
#define IMAGEFETCHER_H
#define QT_NO_KEYWORDS

#include <QObject>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QPointer>

struct FetchType {
  enum Value {
    POTD, LE, General
  };
};

class ImageFetcher : public QObject
{
    Q_OBJECT
public:
    explicit ImageFetcher(QObject *parent = 0);
    void fetch();
    void setFilename(const QString& imageFilename);
    void setFetchType(FetchType::Value fetchType);
    void setBuffered(bool buffered);
    void releaseBuffer();
    void abort();
    FetchType::Value fetchType();

private:
    FetchType::Value fetchType_ = FetchType::LE;
    bool alreadyTriedAlternativeLinkToDropbox_ = false;
    QDate potdDescriptionDate_;
    QString potdDescriptionFilename_;
    QNetworkAccessManager *fileDownloader_;
    QPointer<QNetworkReply> currentNetworkRequest_;
    QFile imageFile_;
    QString imageFilename_;
    QString imageUrl_;
    QByteArray *imageBuffer_ = NULL;
    bool buffered_ = false;

    void potdDownload();
    void leDownload();
    void generalDownload();
    void downloadTextFileContainingImage(const QString &url);
    void downloadOnlineImage(const QString &onlineLink);
    void disconnectFileDownloader();
    void downloadPotdDescription();
    void writePotdDescription(const QString &description);
    void tryDownloadingImagesFromAlternativeLink();
    void applyLeTags(const QString &filename);
    QString replaceSpecialHtml(const QString &html);

Q_SIGNALS:
    void fail();
    void success(QString image);
    void successBuf(QByteArray* imageData);
    void downloadProgress(unsigned int);

private Q_SLOTS:
    void writeImage();
    void imageDownloadFinished();
    void getPotdDescription(QNetworkReply* reply);
    void readFileContainingImage(QNetworkReply *reply);
    void downloadFailed(QNetworkReply::NetworkError error = QNetworkReply::NoError);
    void reportDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // IMAGEFETCHER_H
