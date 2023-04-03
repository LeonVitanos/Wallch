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

#ifndef POTD_PREVIEW_H
#define POTD_PREVIEW_H

#include "tryhard.h"

#include <QDialog>
#include <QNetworkAccessManager>
#include <QPointer>

namespace Ui {
class PotdPreview;
}

class PotdPreview : public QDialog
{
    Q_OBJECT
    
protected:
    void resizeEvent(QResizeEvent *event);

public:
    explicit PotdPreview(QWidget *parent = 0);
    ~PotdPreview();

private:
    Ui::PotdPreview *ui;

    QPixmap currentPixmap_;
    QTimer *updatePreviewTimer_;

    QString originalTextColor_;
    QString originalBackgroundColor_;
    QString originalTextFont_;
    bool originalTop_;
    int originalLeftMargin_;
    int originalRightMargin_;
    int originalBottomTopMargin_;

    QString textColor_;
    QString backgroundColor_;
    const QString potdDescription_="The Laughing Kookaburra (Dacelo novaeguineae) is a carnivorous bird in the kingfisher family. Native to eastern Australia, it has also been introduced to parts of New Zealand, Tasmania and Western Australia. Male and female adults are similar in plumage, which is predominantly brown and white. A common and familiar bird, this species of kookaburra is well known for its laughing call. Photo: JJ Harrison";
    const QStringList potdPreviewImages_ = QStringList() << "http://i.imgur.com/6rRVaA1.jpg" << "http://i.imgur.com/VLD6HRQ.jpg" << "http://melloristudio.com/wallch/potd_preview.jpg";

    TryHard *tryFetch_ = NULL;

    QImage originalImage_;
    bool fetchFailed_=false;
    bool fetchFinished_=false;
    void fetchFinished(QImage img);
    void getPotdPreviewImage();
    bool loadingWindow=true;
    
private Q_SLOTS:
    void imageFetchfailed();
    void downloadedImage(QByteArray array);
    void on_textColorPotd_clicked();
    void on_backgroundColorPotd_clicked();
    void on_potdFontComboBox_currentFontChanged();
    void on_potd_description_bottom_radioButton_clicked();
    void on_potd_description_top_radioButton_clicked();
    void on_ok_clicked();
    void on_cancel_clicked();
    void on_left_margin_spinbox_valueChanged(int arg1);
    void on_right_margin_spinbox_valueChanged(int arg1);
    void on_bottom_top_margin_spinbox_valueChanged(int arg1);
    void writeDescription();
    void updateLabel();

Q_SIGNALS:
    void potdPreferencesChanged();
};

#endif // POTD_PREVIEW_H
