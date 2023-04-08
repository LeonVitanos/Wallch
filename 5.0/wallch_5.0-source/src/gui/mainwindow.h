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

#define IMAGE_TRANSITION_SPEED 700
#define GENERAL_ANIMATION_SPEED 150

#define SCREEN_LABEL_SIZE_X 285
#define SCREEN_LABEL_SIZE_Y 172

#define WALLPAPERS_HELP "/usr/share/gnome/help/wallch/C/howto_wallpapers.page";
#define LIVEARTH_HELP "/usr/share/gnome/help/wallch/C/howto_livearth.page";
#define POTD_HELP "/usr/share/gnome/help/wallch/C/howto_potd.page";
#define LIVEWEBSITE_HELP "/usr/share/gnome/help/wallch/C/howto_livewebsite.page";
#define COLSANDGRADS_HELP "/usr/share/gnome/help/wallch/C/howto_colsandgrads.page";

#define WALLPAPERS_LIST_ICON_SIZE QSize(60, 60)
#define WALLPAPERS_LIST_ITEMS_OFFSET QSize(4, 3)

#include "about.h"
#include "ui_mainwindow.h"
#include "website_preview.h"
#include "properties.h"
#include "preferences.h"
#include "history.h"
#include "statistics.h"
#include "lepoint.h"
#include "glob.h"
#include "colors_gradients.h"
#include "potd_viewer.h"
#include "websitesnapshot.h"
#include "potd_preview.h"
#include "websitesnapshot.h"
#include "wallpapermanager.h"
#include "colormanager.h"
#include "cachemanager.h"
#include "imagefetcher.h"
#include "pictures_locations.h"

#ifdef Q_OS_UNIX

#include <glib-object.h>
#include <gtk/gtk.h>
#include <unity/unity/unity.h>

#else

#include <QSystemTrayIcon>

#include "notification.h"

#include <stdio.h>
#include <windows.h>

#endif //#ifdef Q_OS_UNIX

#include <QMessageBox>
#include <QClipboard>
#include <QProgressBar>
#include <QMovie>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QFutureWatcher>
#include <QDragEnterEvent>
#include <QPainter>
#include <QFileSystemWatcher>
#include <QTranslator>
#include <QStyledItemDelegate>
#include <QButtonGroup>
#include <QShortcut>
#include <QSharedMemory>

typedef enum {
    NoneStyle, Center, Tile, Stretch, Scale, Zoom, Span
} DesktopStyle;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QSharedMemory *attachedMemory, Global *globalParser, ImageFetcher *imageFetcher_,
               WebsiteSnapshot *websiteSnapshot, WallpaperManager *wallpaperManager,
               int timeoutCount, int lastRandomDelay, QWidget *parent = 0);
    ~MainWindow();
    void click_shortcut_next();
    Ui::MainWindow *ui;

protected:
    void closeEvent(QCloseEvent * event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
#ifdef Q_OS_UNIX
    QTimer *batteryStatusChecker_;
    QString getSecondaryColor();
    QProcess *dconf;
#else
    Notification *notification_;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
#endif

    QSharedMemory *attachedMemory_;
    WallpaperManager *wallpaperManager_;
    ColorManager *colorManager_;
    CacheManager *cacheManager_;
    QTimer *updateSecondsTimer_;
    QTimer *updateCheckTime_;
    QTimer *iconUpdater_;
    QTimer *researchFoldersTimer_;
    QTimer *hideProgress_;

    QMenu *listwidgetMenu_;

    QList<QLabel*> menuSeparators_;

    QButtonGroup *btn_group;

    //building menubar's menu
    QMenu *settingsMenu_;
    QMenu *helpMenu_;
    QMenu *currentBgMenu_;
    QAction *preferencesAction_;
    QAction *QuitAction_;

    QFileSystemWatcher *watchFolders_;

    QFutureWatcher<QImage> *scaleWatcher_;

    QRect availableGeometry_;

    LEPoint *lepoint_;
    Statistics *statistics_;
    About *about_;
    Preferences *preferences_;
    Properties *properties_;
    ColorsGradients *colorsGradients_;
    PotdViewer *potdViewer_;
    History *history_;
    PicturesLocations *locations_;
    PotdPreview *potdPreview_;
    QTranslator *translator_;
    WebsitePreview *webPreview_;

    QRegularExpression *match_;
    QMovie *processingRequestGif_;
    QPropertyAnimation *openCloseSearch_;
    QPropertyAnimation *rightWidgetAnimation_;
    QPropertyAnimation *widget3Animation_;
    QPropertyAnimation *openCloseAddLogin_;
    QGraphicsOpacityEffect* opacityEffect_;
    QGraphicsOpacityEffect* opacityEffect2_;
    QPropertyAnimation *increaseOpacityAnimation;
    QPropertyAnimation *decreaseOpacityAnimation;
    WebsiteSnapshot *websiteSnapshot_;
    QIcon imageLoading_;
    bool appAboutToClose_ = false;
    bool stoppedBecauseOnBattery_ = false;
    bool currentlyUpdatingIcons_ = false;
    bool processingOnlineRequest_;
    Global *globalParser_ = NULL;
    ImageFetcher *imageFetcher_ = NULL;
    qint64 secondsLeft_ = 0;
    int initialRandomSeconds_;
    int totalSeconds_ = 0;
    int previouslyRunningFeature_ = 0;
    int secondsInWallpapersSlider_;
    double imagePreviewResizeFactorX_;
    double imagePreviewResizeFactorY_;
    int currentSearchItemIndex;
    short timePassedForLiveWebsiteRequest_;
    short tempForDelayedPicturesLocationChange_;

    //dialogs
    bool addDialogShown_ = false;
    bool historyShown_ = false;
    bool aboutShown_ = false;
    bool propertiesShown_ = false;
    bool statisticsShown_ = false;
    bool lePointShown_ = false;
    bool websitePreviewShown_ = false;
    bool colorsGradientsShown_ = false;
    bool potdViewerShown_ = false;
    bool potdPreviewShown_ = false;
    bool locationsShown_ = false;

    bool actAsStart_ = true;
    bool startWasJustClicked_ = false;
    bool justUpdatedPotd_ = false;
    bool searchIsOn_ = false;
    bool loadedPages_[6] = {false,false,false,false,false,false};
    bool firstRandomImageIsntRandom_ = false;
    bool shuffleWasChecked_ = false;
    bool changingPicturesLocations_ = false;
    bool currentFolderIsAList_ = false;
    bool manuallyStartedOnBattery_ = false;
    QString initialWebPage_;
    QString changedWebPage_;
    QString nameOfSelectionPriorFolderChange_;
    QString currentFolder_;
    QStringList searchList_;
    QStringList monitoredFoldersList_;
    QStringList currentFolderList_;
    QShortcut *menubarShortcut_ = NULL;
    QShortcut *preferencesShortcut_ = NULL;
    QShortcut *quitShortcut_ = NULL;
    QShortcut *contentsShortcut_ = NULL;
    QShortcut *historyShortcut_ = NULL;

    QString getPathOfListItem(int index = -1);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void actionsOnClose();
    void changeImage();
    void startButtonsSetEnabled(bool enabled);
    void stopButtonsSetEnabled(bool enabled);
    void previousAndNextButtonsSetEnabled(bool enabled);
    void startPotd(bool launchNow);
    void monitor(const QString &path, int &itemCount);
    void monitor(const QStringList &finalListOfPaths);
    void setupKeyboardShortcuts();
    void setupTimers();
    QPoint calculateSettingsMenuPos();
    void continueAlreadyRunningFeature(int timeoutCount, int lastRandomDelay);
    void applySettings();
    void retrieveSettings();
    void initializePrivateVariables(Global *globalParser, ImageFetcher *imageFetcher);
    void setupMenu();
    void connectSignalSlots();
    void startUpdateSeconds();
    void addFilesToWallpapers(const QString &path, int &itemCount);
    void searchFor(const QString &term);
    void continueToNextMatch();
    void continueToPreviousMatch();
    void animateProgressbarOpacity(bool show);
    void findSeconds(bool typeCountSeconds);
    void startPauseWallpaperChangingProcess();
    void animateScreenLabel(bool onlyHide);
    void loadWallpapersPage();
    void loadLePage();
    void loadPotdPage();
    void loadWallpaperClocksPage();
    void loadLiveWebsitePage();
    void loadMelloriPage();
    void savePicturesLocations();
    void processRequestStart();
    void processRequestStop();
    void changeAppStyleSheetTo(const QString &styleSheetFile);
    void doesMatch();
    void doesntMatch();
    void setProgressbarsValue(short value);
    void imageTransition(const QString &filename = QString());
    void changeTextOfScreenLabelTo(const QString &text);
    bool updateIconOf(int row);
    bool atLeastOneFolderFromTheSetExists();
    void stopEverythingThatsRunning(short excludingFeature);
    void pauseEverythingThatsRunning();
    bool websiteConfiguredCorrectly();
    void updatePotdProgress();
    void currentFolderDoesNotExist();
    QString secondsToMh(int seconds);
    QString secondsToHms(int seconds);
    QString secondsToHm(int seconds);
    QString fixBasenameSize(const QString &basename);
    void actionsOnWallpaperChange();
    void resetWatchFolders();
    void strongMinimize();
    void processRunningResetPictures();
    void forceUpdateIconOf(int index);
    void prepareWebsiteSnapshot();
    QString base64Encode(const QString &string);
    static void nextKeySignal(const char *, void *);
    void disableLiveWebsitePage();
    bool currentFolderExists();
    QImage scaleWallpapersPreview(QString filename);
    QStringList getCurrentWallpaperFolders();
    void iconsPathsChanged();

private Q_SLOTS:
#ifdef Q_OS_UNIX
    void unityProgressbarSetEnabled(bool enabled);
    void dconfChanges();
#endif
    void timeSpinboxChanged();
    void intervalTypeChanged();
    void closeWhatsRunning();
    void checkBatteryStatus();
    void addFolderForMonitor(const QString &folder);
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
    void readCoordinates(const QRect &cropArea);
    void findAvailableWallpaperStyles();
    void setWebsitePreviewImage(QImage *image);
    void setButtonColor();
    void setButtonColor(const QString &colorName);
    void enterPressed();
    void openCloseAddLoginAnimationFinished();
    void updateSeconds();
    void liveWebsiteImageCreated(QImage *image, short errorCode);
    void changeCurrentTheme();
    void showHideSearchBox();
    void showHideSearchBoxMenu();
    void escapePressed();
    void deletePressed();
    void previousPage();
    void nextPage();
    void openCloseSearchAnimationFinished();
    void hideTimeForNext();
    void hidePreview();
    void showPreview();
    void removeBottomwidgets();
    void bottomWidgetsAnimation();
    void startWithThisImage();
    void setAverageColor(const QString &image);
    void updateScreenLabel();
    void strongShowApp();
    void hideOrShow();
    void picturesLocationsChanged();
    void preferencesDestroyed();
    void statisticsDestroyed();
    void lePointDestroyed();
    void historyDestroyed();
    void aboutDestroyed();
    void locationsDestroyed();
    void propertiesDestroyed();
    void websitePreviewDestroyed();
    void colorsGradientsDestroyed();
    void potdViewerDestroyed();
    void potdPreviewDestroyed();
    void updateColorButton(QImage image);
    void delayed_pictures_location_change();
    void restartPotdIfRunningAfterSettingChange();
    void restartLeIfRunningAfterSettingChange();
    void justChangeWallpaper();
    void setPreviewImage();
    void unhoverMenuButton();
    void beginFixCacheForFolders();
    void wait_preview_changed();
    void preview_changed();
    void update_website_settings();
    void doQuit();
    void onlineRequestFailed();
    void onlineImageRequestReady(QString image);
    void on_actionHistory_triggered();
    void on_actionStatistics_triggered();
    void on_actionContents_triggered();
    void on_previous_Button_clicked();
    void on_next_Button_clicked();
    void on_wallpapersList_customContextMenuRequested();
    void on_wallpapersList_itemDoubleClicked();
    void on_wallpapersList_itemSelectionChanged();
    void on_timerSlider_valueChanged(int value);
    void on_website_preview_clicked();
    void on_edit_crop_clicked();
    void on_website_crop_checkbox_clicked(bool checked);
    void on_set_desktop_color_clicked();
    void on_image_style_combo_currentIndexChanged(int index);
    void on_website_textEdited(const QString &arg1);
    void on_browse_folders_clicked();
    void on_website_slider_valueChanged(int value);
    void on_le_tag_checkbox_clicked(bool checked);
    void on_le_tag_button_clicked();
    void on_pictures_location_comboBox_currentIndexChanged(int index);
    void on_search_box_textChanged(const QString &arg1);
    void on_search_close_clicked();
    void on_search_up_clicked();
    void on_search_down_clicked();
    void on_actionDonate_triggered();
    void on_actionReport_A_Bug_triggered();
    void on_actionGet_Help_Online_triggered();
    void on_actionWhat_is_my_screen_resolution_triggered();
    void on_actionCopy_Path_triggered();
    void on_actionCopy_Image_triggered();
    void on_actionDelete_triggered();
    void on_actionProperties_triggered();
    void on_actionOpen_Image_triggered();
    void on_actionOpen_Folder_triggered();
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
    void on_activate_website_clicked();
    void on_deactivate_website_clicked();
    void page_button_clicked(int btn);
    void on_include_description_checkBox_clicked(bool checked);
    void on_edit_potd_clicked();
    void on_shuffle_images_checkbox_clicked();
    void on_stackedWidget_currentChanged(int page);
    void on_random_from_valueChanged(int value);
    void on_random_to_valueChanged(int value2);
    void on_random_time_from_combobox_currentIndexChanged(int index);
    void on_random_time_to_combobox_currentIndexChanged(int index);
    void on_edit_pushButton_clicked();
    void updateImageStyleCombo();

Q_SIGNALS:
     void fixLivewebsiteButtons();
     void noMatch();
     void givePropertiesRequest(QString img, int current_index);
     void monitorCheck();
     void signalUncheckRunningFeatureOnTray();
     void signalRecreateTray();

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
