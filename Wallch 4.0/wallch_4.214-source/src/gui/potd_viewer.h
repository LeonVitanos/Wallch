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

#ifndef POTD_VIEWER_H
#define POTD_VIEWER_H

#define QT_NO_KEYWORDS

#include "glob.h"
#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QFile>
#include <QMovie>

namespace Ui {
    class potd_viewer;
}

class PotdViewer : public QDialog
{
    Q_OBJECT    

protected:
    void resizeEvent(QResizeEvent *);

public:
    explicit PotdViewer(QWidget *parent = 0);
    ~PotdViewer();
    
private:
    Ui::potd_viewer *ui;
    QUrl url_;
    bool downloadingImage_=false;
    QNetworkAccessManager qnam_;
    QNetworkReply *reply_=NULL;
    QImage originalImage_;
    QMovie *originalMovie_=NULL;
    int httpGetId_;
    bool httpRequestAborted_;
    QDate selectedDate_;
    QString directLinkOfFullImage_;
    QString directLinkOfPreviewImage_;
    bool skipStepsAfterThree_;
    bool forceStop_;
    QString htmlSource_;
    QString formatOfImage_;
    void startRequest(QUrl url);
    void urlQDate();
    void enableWidgets(bool state);

private Q_SLOTS:
    void movieDestroyed();
    void httpFinished();
    void httpReadyRead();
    void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);
    void updateDataReadProgress_save(qint64 bytesRead, qint64 totalBytes);
    void imageDownloaded();
    void saveImage();
    void on_quitButton_clicked();
    void on_saveimageButton_clicked();
    void on_dateEdit_dateChanged(const QDate &date_calendar);
    void on_previousButton_clicked();
    void on_nextButton_clicked();
    void updateLabel();

};

#endif // POTD_VIEWER_H
