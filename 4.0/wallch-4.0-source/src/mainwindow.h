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

#define QT_NO_KEYWORDS

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define IMAGE_TRANSITION_SPEED 250
#define GENERAL_ANIMATION_SPEED 150

#define SCREEN_LABEL_SIZE_X 285
#define SCREEN_LABEL_SIZE_Y 172

#define WALLPAPERS_HELP "/usr/share/gnome/help/wallch/C/howto_wallpapers.page";
#define LIVEARTH_HELP "/usr/share/gnome/help/wallch/C/howto_livearth.page";
#define POTD_HELP "/usr/share/gnome/help/wallch/C/howto_potd.page";
#define WALLCLOCKS_HELP "/usr/share/gnome/help/wallch/C/howto_wallclocks.page";
#define LIVEWEBSITE_HELP "/usr/share/gnome/help/wallch/C/howto_livewebsite.page";
#define COLSANDGRADS_HELP "/usr/share/gnome/help/wallch/C/howto_colsandgrads.page";

#define CHECK_CACHE_SIZE_TIMEOUT 5000

#define WALLPAPERS_LIST_ICON_SIZE QSize(60, 60)
#define WALLPAPERS_LIST_ITEMS_OFFSET QSize(4, 3)
#define WALLPAPER_CLOCKS_LIST_ICON_SIZE  QSize(100,75)
#define WALLPAPER_CLOCKS_ITEM_ICON_SIZE QSize(82, 82)

#include "about.h"
#include "ui_mainwindow.h"
#include "website_preview.h"
#include "properties.h"
#include "preferences.h"
#include "history.h"
#include "statistics.h"
#include "glob.h"
#include "colors_gradients.h"
#include "potd_viewer.h"
#include "websitesnapshot.h"
#include "notification.h"
#include "potd_preview.h"
#include "websitesnapshot.h"

#ifdef ON_LINUX

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <keybinder.h>
#include <unity/unity/unity.h>

#else

#include <QSystemTrayIcon>
#include <stdio.h>
#include <windows.h>

#endif //#ifdef ON_LINUX

#include <QMessageBox>
#include <QClipboard>
#include <QProgressBar>
#include <QMovie>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDragEnterEvent>
#include <QPainter>
#include <QFileSystemWatcher>
#include <QTranslator>
#include <QStyledItemDelegate>
#include <QButtonGroup>
#include <QFutureWatcher>

typedef enum {
    NoneStyle, Center, Tile, Stretch, Scale, Zoom
} DesktopStyle;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Global *globalParser, WebsiteSnapshot *websiteSnapshotP, int timeout_count, QStringList previous_pictures_from_main, int last_random_delay, int last_normal_picture, QWidget *parent = 0);
    ~MainWindow();
    void click_shortcut_next();
    Ui::MainWindow *ui;

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent * event);
    void resizeEvent(QResizeEvent *event);

private:
    void actionsOnClose();
    QTimer *updateSecondsTimer_;
    QTimer *wallpaperClockWait_;
    QTimer *updateCheckTime_;
    QTimer *iconUpdater_;
    QTimer *researchFoldersTimer_;
    QTimer *hideProgress_;
#ifdef ON_LINUX
    QTimer *indicatorChangeNormalTimer_;
#endif //#ifdef ON_LINUX
    QTimer *cacheSizeChecker_;

    QFutureWatcher<QImage*> futureWatcherWallClocksPreview_;
    QFutureWatcher<QImage*> futureWatcherWallpapersPreview_;

    QMenu *listwidgetMenu_;
    QMenu *clockwidgetMenu_;

    QFileSystemWatcher *watchFolders_;

    QButtonGroup *clocksRadioButtonsGroup;

    Statistics *statistics_;
    About *about_;
    Preferences *preferences_;
    Properties *properties_;
    ColorsGradients *colorsGradients_;
    PotdViewer *potdViewer_;
    History *history_;
    Notification *notification_;
    PotdPreview *potdPreview_;
    QTranslator *translator_;
    WebsitePreview *webPreview_;
    QRegExp *match_;
    QProgressBar *timeForNext_;
    QMovie *processingRequestGif_;
    QPropertyAnimation *openCloseSearch_;
    QPropertyAnimation *showTvMinimumHeight_;
    QPropertyAnimation *openCloseAddLogin_;
    QGraphicsOpacityEffect* opacityEffect_;
    QGraphicsOpacityEffect* opacityEffect2_;
#ifdef ON_LINUX
    QProcess *unzipArchive_;
#endif
    WebsiteSnapshot *websiteSnapshot_;
    QIcon imageLoading_;
    bool appAboutToClose_=false;
    bool currentlyUpdatingIcons_=false;
    bool processingOnlineRequest_;
    Global *globalParser_;
    qint64 secondsLeft_=0;
    int initialRandomSeconds_;
    int totalSeconds_=0;
    int secondsInWallpapersSlider_;
    double imagePreviewResizeFactorX_;
    double imagePreviewResizeFactorY_;
    int indexOfCurrentImage_;
    int currentSearchItemIndex;
    bool currentlyUninstallingWallpaperClock_=false;
    short timePassedForLiveWebsiteRequest_;
    short tempForDelayedPicturesLocationChange_;
    bool addDialogShown_=false;
    bool historyShown_=false;
    bool aboutShown_=false;
    bool propertiesShown_=false;
    bool statisticsShown_=false;
    bool websitePreviewShown_=false;
    bool colorsGradientsShown_=false;
    bool potdViewerShown_=false;
    bool potdPreviewShown_=false;
    bool actAsStart_;
    bool startWasJustClicked_=false;
    bool justUpdatedPotd_=false;
    bool searchIsOn_=false;
    bool wallpapersPageLoaded_=false;
    bool potdPageLoaded_=false;
    bool wallpaperClocksPageLoaded_=false;
    bool liveWebsitePageLoaded_=false;
    bool firstRandomImageIsntRandom_=false;
    bool shuffleWasChecked_=false;
    QString initialWebPage_;
    QString changedWebPage_;
    QString currentWallpaperClockPath_;
    QString nameOfSelectionPriorFolderChange_;
    QString currentFolder_;
    QStringList previousBackgrounds_;
    QStringList searchList_;
    QStringList customIntervals_;
    QStringList monitoredFoldersList_;

#ifdef ON_WIN32
    void unzip(QString input, const QString &outputDirectory);
#else
    DesktopStyle getDesktopStyle();
    ColoringType getColoringType();
    QString getPrimaryColor();
    QString getSecondaryColor();
#endif
    QString getPathOfListItem(int index=-1);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void changeImage();
    void startButtonsSetEnabled(bool enabled);
    void stopButtonsSetEnabled(bool enabled);
    void previousAndNextButtonsSetEnabled(bool enabled);
    void startLivearth();
    void stopLivearth();
    void startPotd(bool launch_now);
    void monitor(const QString &path, int &itemCount);
    void monitor(const QStringList &paths);
    void setupKeyboardShortcuts();
    void setupTimers();
    void setupThemeFallbacks();
    void connectSignalSlots();
    void startUpdateSeconds();
    void addFilesToWallpapers(const QString &path, int &itemCount);
    void searchFor(const QString &term);
    void continueToNextMatch();
    void continueToPreviousMatch();
    void animateProgressbarOpacity(bool show);
    void findSeconds(bool typeCountSeconds);
    void startPauseWallpaperChangingProcess();
    void loadPotdPage();
    void loadWallpaperClocksPage();
    void loadLiveWebsitePage();
    void setPreviewImage(QImage *image);
    void updatePicturesLocations();
    void processRequestStart();
    void processRequestStop();
    void setThemeToAmbiance();
    void setThemeToRadiance();
    void doesMatch();
    void doesntMatch();
    void setProgressbarsValue(short value);
    void imageTransition(const QString &filename);
    void hideScreenLabel();
    void changeTextOfScreenLabelTo(const QString &text);
    bool updateIconOf(int row);
    void closeEverythingThatsRunning(short excludingFeature);
    bool websiteConfiguredCorrectly();
    void updatePotdProgress();
    QImage *wallpaperClocksPreview(const QString &wallClockPath, bool minuteCheck, bool hourCheck, bool amPmCheck, bool dayWeekCheck, bool dayMonthCheck, bool monthCheck);
    QString secondsToMh(int seconds);
    QString secondsToHms(int seconds);
    QString secondsToHm(int seconds);
    QString fixBasenameSize(const QString &basename);
    void actionsOnWallpaperChange();
    void resetWatchFolders();
    void strongMinimize();
    void processRunningResetPictures(bool fromRandomImagesToNormal);
    void forceUpdateIconOf(int index);
    void fixCacheSizeGlobalCallback();
    void cacheSizeChanged(const QString &newCacheImage);
    void prepareWebsiteSnapshot();
    QString base64Encode(const QString &string);
    static void nextKeySignal(const char *, void *);
    void loadWallpapersPage();
    void disableLiveWebsitePage();
    bool currentFolderExists();
    void setupAnimationsAndChangeImage(QImage *image);
    QImage *scaleWallpapersPreview(QString filename);

private Q_SLOTS:
    void closeWhatsRunning();
    void addFolderForMonitor(const QString &folder);
    void installWallpaperClock(const QString &currentPath);
    void random_time_changed(short index);
    void folderChanged();
    void researchFolders();
    void updateTiming();
    void sendPropertiesNext(int current);
    void sendPropertiesPrevious(int current);
    void changePathsToIcons();
    void changeIconsToPaths();
    void launchTimerToUpdateIcons();
    void updateVisibleIcons();
    void removeImageFromDisk();
    void removeImagesFromDisk();
    void showProperties();
    void openImageFolder();
    void openImageFolderMassive();
    void rotateRight();
    void rotateLeft();
    void copyImage();
    void copyImagePath();
    void clockCheckboxClickedWhileRunning();
    void readCoordinates(const QRect &cropArea);
    void setStyle();
    void setWebsitePreviewImage(QImage *image);
    void setButtonColor();
    void setButtonColor(const QString &color_name);
    void enterPressed();
    void openCloseAddLoginAnimationFinished();
    void updateSeconds();
    void liveWebsiteImageCreated(QImage *image, short errorCode);
    void addClockToList();
    void changeCurrentThemeTo(const QString &theme);
    void showHideSearchBox();
    void showHideSearchBoxMenu();
    void escapePressed();
    void deletePressed();
    void previousPage();
    void nextPage();
    void refreshIntervals();
    void openCloseSearchAnimationFinished();
    void hideTimeForNext();
    void tvPreview(bool show);
    void hideTv();
    void wrongSizeOfWindow();
    void startWithThisImage();
    void setAverageColor(const QString &image);
    void updateScreenLabel();
    void strongShowApp();
#ifdef ON_LINUX
    void bindKey(const QString &key);
    void unbindKey(const QString &key);
    void unityProgressbarSetEnabled(bool enabled);
    void indicatorSetNormal();
#endif //#ifdef ON_LINUX
    void preferencesDestroyed();
    void statisticsDestroyed();
    void historyDestroyed();
    void aboutDestroyed();
    void propertiesDestroyed();
    void websitePreviewDestroyed();
    void colorsGradientsDestroyed();
    void potdViewerDestroyed();
    void potdPreviewDestroyed();
    void updateColorButton(QImage image);
    void fixCacheSizeThreaded();
    void delayed_pictures_location_change();
    void clockRadioButton_check_state_changed();
    void restartPotdIfRunningAfterSettingChange();
    void wallpaperClocksPreviewImageGenerationFinished();
    void wallpapersPreviewImageGenerationFinished();
    void justChangeWallpaper();
    void onlineRequestFailed();
    void onlineImageRequestReady(QString image);
    void on_actionHistory_triggered();
    void on_actionStatistics_triggered();
    void on_actionContents_triggered();
    void on_previous_Button_clicked();
    void on_next_Button_clicked();
    void on_listWidget_customContextMenuRequested();
    void on_listWidget_itemDoubleClicked();
    void on_listWidget_itemSelectionChanged();
    void on_timerSlider_valueChanged(int value);
    void on_install_clock_clicked();
    void on_label_9_linkActivated();
    void on_label_3_linkActivated();
    void on_website_preview_clicked();
    void on_edit_crop_clicked();
    void on_website_crop_checkbox_clicked(bool checked);
    void on_set_desktop_color_clicked();
    void on_image_style_combo_currentIndexChanged(int index);
    void on_website_textEdited(const QString &arg1);
    void on_page_5_other_clicked();
    void on_browse_folders_clicked();
    void on_website_slider_valueChanged(int value);
    void on_pictures_location_comboBox_currentIndexChanged(int index);
    void on_search_box_textChanged(const QString &arg1);
    void on_search_close_clicked();
    void on_search_up_clicked();
    void on_search_down_clicked();
    void on_actionDonate_triggered();
    void on_actionDownload_triggered();
    void on_actionReport_A_Bug_triggered();
    void on_actionGet_Help_Online_triggered();
    void on_actionWhat_is_my_screen_resolution_triggered();
    void on_actionCopy_Name_triggered();
    void on_actionCopy_Path_triggered();
    void on_actionCopy_Image_triggered();
    void on_actionDelete_triggered();
    void on_actionProperties_triggered();
    void on_actionOpen_Folder_triggered();
    void on_actionQuit_Ctrl_Q_triggered();
    void on_potd_viewer_Button_clicked();
    void on_add_login_details_clicked(bool checked);
    void on_action_About_triggered();
    void on_action_Preferences_triggered();
    void on_stopButton_clicked();
    void on_startButton_clicked();
    void on_activate_livearth_clicked();
    void on_deactivate_livearth_clicked();
    void on_activate_potd_clicked();
    void on_deactivate_potd_clicked();
    void on_activate_clock_clicked();
    void on_deactivate_clock_clicked();
    void on_activate_website_clicked();
    void on_deactivate_website_clicked();
    void on_page_0_wallpapers_clicked();
    void on_page_1_earth_clicked();
    void on_page_2_potd_clicked();
    void on_page_3_clock_clicked();
    void on_page_4_web_clicked();
    void on_clocksTableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_include_description_checkBox_clicked(bool checked);
    void on_edit_potd_clicked();
    void clockCheckboxClicked();
    void on_clocksTableWidget_customContextMenuRequested();
    void uninstall_clock();
    void on_shuffle_images_checkbox_clicked();
    void on_stackedWidget_currentChanged(int page);
    void update_website_settings();
    void on_donateButton_clicked();
    void on_melloristudio_link_label_linkActivated(const QString &link);

Q_SIGNALS:
     void fixLivewebsiteButtons();
     void noMatch();
     void givePropertiesRequest(QString img, int current_index);
     void monitorCheck();
#ifdef ON_WIN32
     void signalUncheckRunningFeatureOnTray();
     void signalRecreateTray();
#endif

};

class HideGrayLinesDelegate : public QStyledItemDelegate
{
public:
        HideGrayLinesDelegate(QObject *parent = NULL) :  QStyledItemDelegate(parent){}
        void paint(QPainter *painter, const QStyleOptionViewItem &option,  const QModelIndex &index) const
        {
                QStyleOptionViewItem opt = option;
                opt.state &= ~QStyle::State_HasFocus;
                QStyledItemDelegate::paint(painter, opt, index);
        }
};

#endif // MAINWINDOW_H
