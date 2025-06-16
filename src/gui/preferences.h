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
    QString getCommandOfDesktopFile(const QString &file);
    QString dataToNiceString(qint64 data);
    short oldTheme;

private Q_SLOTS:
    void previousPage();
    void nextPage();
    void handlePageNavigation(short);
    void handleResetClick();
    void handleSaveClick();
    void handleCloseClick();
    void handleGeneralPageClick();
    void handleWallpapersPageClick();
    void handleLiveWebsitePageClick();
    void handleAdvancedPageClick();
    void handleThemeChange(int index);
    void handleRotateCheck(bool checked);
    void handleStartupCheck(bool checked);
    void handleHelpClick();
    void handleMaxCacheSliderChange(int value);
    void handleClearThumbnailsClick();

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
