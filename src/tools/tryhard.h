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

#ifndef TRYHARD_H
#define TRYHARD_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>

class TryHard : public QObject
{
    Q_OBJECT
public:
    explicit TryHard(QObject *parent = 0, const QStringList &fetchLinks = QStringList());
    void start();
    void abort();

private:
    QNetworkAccessManager *fileDownloader_;
    QPointer<QNetworkReply> currentNetworkRequest_;
    QStringList fetchLinks_;
    unsigned short fetchLinksCount_ = 0;
    unsigned short currentBackupLinkIndex_ = 0;
    bool fetchFailed_ = false;
    bool fetchFinished_ = false;
    void fetchFinished(QImage *img);
    void getFile();
    void fileFetchFailed();
    void attemptDownload();
    void actionsOnFail();
    void actionsOnSuccess(const QByteArray &array);

Q_SIGNALS:
    void success(const QByteArray &array);
    void failed();

public Q_SLOTS:
    void downloadedFile(QNetworkReply *reply);
};

#endif // TRYHARD_H
