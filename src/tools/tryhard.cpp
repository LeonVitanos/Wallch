/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#include "tryhard.h"

// todo remove
#include <QImage>

const bool DEBUG_ENABLED = false;

TryHard::TryHard(QObject *parent, const QStringList &fetchLinks) :
    QObject(parent)
{
    fetchLinks_ = fetchLinks;
    fetchLinksCount_ = fetchLinks.count();
}

void TryHard::start()
{
    fileDownloader_ = new QNetworkAccessManager(this);

    connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadedFile(QNetworkReply*)));

    attemptDownload();
}

void TryHard::abort()
{
    if (currentNetworkRequest_) {
        currentNetworkRequest_->abort();
    }
}

void TryHard::actionsOnFail()
{
    Q_EMIT failed();
}

void TryHard::actionsOnSuccess(const QByteArray &array)
{
    Q_EMIT success(array);
    currentBackupLinkIndex_ = 0;
}

void TryHard::attemptDownload()
{
    if (currentBackupLinkIndex_ >= fetchLinksCount_) {
        actionsOnFail();
    } else {
        if (DEBUG_ENABLED) {
            qDebug() << "Downloading" << fetchLinks_.at(currentBackupLinkIndex_);
        }
        currentNetworkRequest_ = fileDownloader_->get(QNetworkRequest(QUrl(fetchLinks_.at(currentBackupLinkIndex_++))));
    }
}

void TryHard::downloadedFile(QNetworkReply *reply){

    if(reply == NULL || reply->error()){
        if (reply) {
            reply->deleteLater();
        }
        // proceed to the next backup link
        attemptDownload();
        return;
    }

    actionsOnSuccess(reply->readAll());

    reply->deleteLater();
}
