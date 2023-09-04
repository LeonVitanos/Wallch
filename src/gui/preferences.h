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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QPropertyAnimation>

namespace Ui {
    class preferences;
}

class Preferences : public QDialog {
    Q_OBJECT
public:
    Preferences(QWidget *parent = 0);
    ~Preferences();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::preferences *ui;
    bool listIsAlreadyIcons_;
    bool addNewIntervalIsShown_;
    bool picturesLocationsChanged_ = false;
    bool maxCacheChanged_ = false;
    void setupShortcuts();
    bool createDesktopFile(const QString &path, const QString &command, const QString &comment);
    QString getCommandOfDesktopFile(const QString &file);
    QString dataToNiceString(qint64 data);
    short oldTheme;

private Q_SLOTS:
    void previousPage();
    void nextPage();
    void on_reset_clicked();
    void on_saveButton_clicked();
    void on_closeButton_clicked();
    void on_page_0_general_clicked();
    void on_page_1_wallpapers_page_clicked();
    void on_page_2_live_website_clicked();
    void on_page_3_advanced_clicked();
    void on_theme_combo_currentIndexChanged(int index);
    void on_rotate_checkBox_clicked(bool checked);
#ifdef Q_OS_LINUX
    void on_de_combo_currentIndexChanged(int index);
#endif
    void on_startupCheckBox_clicked(bool checked);
    void on_help_clicked();
    void on_max_cache_slider_valueChanged(int value);
    void on_clear_thumbnails_button_clicked();

Q_SIGNALS:
    void intervalTypeChanged();
    void changePathsToIcons();
    void changeIconsToPaths();
    void changeTheme();
    void researchFolders();
    void previewChanged();
    void deManuallyChanged();
    void maxCacheChanged(qint64 maxCache);
};

#endif // PREFERENCES_H
