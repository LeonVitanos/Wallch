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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QGroupBox>
#include <QGridLayout>
#include <QProcess>
#include <QTimer>

namespace Ui {
    class properties;
}

class Properties : public QDialog
{
    Q_OBJECT

public:
    explicit Properties(const QString &img, bool showNextPrevious, int currentIndex, QWidget *parent = 0);
    ~Properties();

protected:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::properties *ui;
    QGroupBox *optionsGroupBox_;
    QPixmap currentPixmap_;
    QGridLayout *optionsGroupBoxLayout_;
    QVBoxLayout *mainLayout_;
    QHBoxLayout *buttonsLayout_;
    QTimer *nextPreviousTimer_;
    QString currentFilename_;
    int  currentIndex_;
    void updateScreenshotLabel();
    QString sizeToNiceString(qint64 fsize);

private Q_SLOTS:
    void on_close_clicked();
    void on_previous_clicked();
    void on_next_clicked();
    void on_set_as_background_clicked();
    void on_open_location_button_clicked();
    void simulateNext();
    void simulatePrevious();
    void uncheckButtons();
    void updateEntries(const QString &filename, int currentIndex);

Q_SIGNALS:
    void requestNext(int currentIndex_);
    void requestPrevious(int currentIndex_);
    void newAverageColor(QString currentFilename);
};

#endif // PROPERTIES_H
