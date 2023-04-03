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

#ifndef WEBSITE_PREVIEW_H
#define WEBSITE_PREVIEW_H

#define QT_NO_KEYWORDS

#include <QDialog>
#include <QTimer>

#include "crop_image.h"
#include "websitesnapshot.h"
#include "glob.h"

namespace Ui {
class website_preview;
}

class WebsitePreview : public QDialog
{
    Q_OBJECT
    
public:
    explicit WebsitePreview(WebsiteSnapshot *websiteSnapshotP, bool showCropDialog, bool crop, QRect cropArea, QWidget *parent = 0);
    ~WebsitePreview();
    
private:
    Ui::website_preview *ui;
    CropImage *cropImageDialog_;
    WebsiteSnapshot *websiteSnapshot_;
    QTimer *countdownTimer_;
    bool forCropDialog_;
    QRect curCropArea_;
    short timeout_;
    void closeDialog();
    void failWithMessage(const QString &further_info);
    void beginCropDialog(QImage *image);

private Q_SLOTS:
    void on_cancel_or_close_clicked();
    void sendCoordinates(const QRect &coords);
    void imageReady(QImage *image, short errorCode);
    void reduceTimeoutByOne();

Q_SIGNALS:
    void sendExtraCoordinates(QRect coords);
    void previewImageReady(QImage *image);
};

#endif // WEBSITE_PREVIEW_H
