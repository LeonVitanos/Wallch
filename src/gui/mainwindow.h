/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2025, The Wallch Team.
 Original Authors (2010-2015): Alexandros Solanos, Leon Vitanos
 Modernization & New Code (2025-): Leon Vitanos

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define IMAGE_TRANSITION_SPEED 700

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

#include "preferences.h"
#include "history.h"
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
#include "timermanager.h"
#include "filemanager.h"
#include "dialoghelper.h"
#include "wallpaperhelper.h"
#include "searchimages.h"
#include "featurecontroller.h"
#include "silenced_qfuturewatcher.h" // IWYU pragma: keep

#ifndef Q_OS_LINUX
    #include "notification.h"
# ifdef Q_OS_WIN
    #include <stdio.h>
    #include <windows.h>
# endif
#endif

#include <QMessageBox>
#include <QProgressBar>
#include <QMovie>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDragEnterEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QButtonGroup>
#include <QShortcut>
#include <QSharedMemory>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QSharedMemory *attachedMemory,
               Global *globalParser,
               ImageFetcher *imageFetcher,
               WebsiteSnapshot *websiteSnapshot,
               WallpaperManager *wallpaperManager,
               DialogHelper *dialogHelper,
               TimerManager *timerManager,
               FeatureController *featureController,
               QWidget *parent = 0);
    ~MainWindow();
    void click_shortcut_next();
    Ui::MainWindow *ui;

protected:
    void closeEvent(QCloseEvent * event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

#if defined(Q_OS_WIN)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif // QT_VERSION
#endif // Q_OS_WIN

private:
#ifdef Q_OS_LINUX
    QTimer *batteryStatusChecker_;
    QString getSecondaryColor();
    QProcess *dconf;
#else
    Notification *notification_;
#endif

    QSharedMemory *attachedMemory_;
    Global *globalParser_;
    ImageFetcher *imageFetcher_;
    WebsiteSnapshot *websiteSnapshot_;
    WallpaperManager *wallpaperManager_;
    DialogHelper *dialogHelper_;
    TimerManager *timerManager_;
    FeatureController *featureController_;

    FileManager *fileManager_;
    ColorManager *colorManager_;
    CacheManager *cacheManager_;
    QTimer *updateSecondsTimer_;
    QTimer *updateCheckTime_;
    QTimer *iconUpdater_;
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

    QFutureWatcher<QImage> *scaleWatcher_;

    QRect availableGeometry_;

    LEPoint *lepoint_;
    About *about_;
    Preferences *preferences_;
    ColorsGradients *colorsGradients_;
    PotdViewer *potdViewer_;
    History *history_;
    PicturesLocations *locations_;
    PotdPreview *potdPreview_;
    WebsitePreview *webPreview_;
    WallpaperHelper *wallpaperHelper_;
    SearchImages *searchImages_;

    QMovie *processingRequestGif_;
    QPropertyAnimation *rightWidgetAnimation_;
    QPropertyAnimation *widget3Animation_;
    QPropertyAnimation *openCloseAddLogin_;
    QGraphicsOpacityEffect* opacityEffect_;
    QGraphicsOpacityEffect* opacityEffect2_;
    QPropertyAnimation *increaseOpacityAnimation;
    QPropertyAnimation *decreaseOpacityAnimation;
    QIcon imageLoading_;
    bool appAboutToClose_ = false;
    bool stoppedBecauseOnBattery_ = false;
    bool currentlyUpdatingIcons_ = false;
    bool processingOnlineRequest_;
    int previouslyRunningFeature_ = 0;
    double imagePreviewResizeFactorX_;
    double imagePreviewResizeFactorY_;
    short timePassedForLiveWebsiteRequest_;
    short tempForDelayedPicturesLocationChange_;

    //seach box
    QString nameOfSelectionPriorFolderChange_;

    //dialogs
    bool addDialogShown_ = false;
    bool historyShown_ = false;
    bool aboutShown_ = false;
    bool lePointShown_ = false;
    bool websitePreviewShown_ = false;
    bool colorsGradientsShown_ = false;
    bool potdViewerShown_ = false;
    bool potdPreviewShown_ = false;
    bool locationsShown_ = false;

    bool actAsStart_ = true;
    bool startWasJustClicked_ = false;
    bool justUpdatedPotd_ = false;
    bool loadedPages_[6] = {false,false,false,false,false,false};
    bool firstRandomImageIsntRandom_ = false;
    bool shuffleWasChecked_ = false;
    bool changingPicturesLocations_ = false;
    bool manuallyStartedOnBattery_ = false;
    QString initialWebPage_;
    QString changedWebPage_;
    QShortcut *menubarShortcut_ = NULL;
    QShortcut *preferencesShortcut_ = NULL;
    QShortcut *quitShortcut_ = NULL;
    QShortcut *contentsShortcut_ = NULL;
    QShortcut *historyShortcut_ = NULL;

    void actionsOnClose();
    void changeImage();
    void startButtonsSetEnabled(bool enabled);
    void stopButtonsSetEnabled(bool enabled);
    void previousAndNextButtonsSetEnabled(bool enabled);
    void startPotd(bool launchNow);
    void setupKeyboardShortcuts();
    void setupTimers();
    QPoint calculateSettingsMenuPos();
    void continueAlreadyRunningFeature();
    void applySettings();
    void initializePrivateVariables();
    void setupMenu();
    void connectSignalSlots();
    void startUpdateSeconds();    
    void animateProgressbarOpacity(bool show);
    void startPauseWallpaperChangingProcess();
    void animateScreenLabel(bool onlyHide);
    void loadWallpapersPage();
    void handlePicturesLocationWhileLoading(bool, int);

    void loadLePage();
    void loadPotdPage();
    void loadWallpaperClocksPage();
    void loadLiveWebsitePage();
    void loadMelloriPage();
    void savePicturesLocations();
    void processRequestStart();
    void processRequestStop();
    void setProgressbarsValue(short value);
    void imageTransition(const QString &filename = QString());
    void changeTextOfScreenLabelTo(const QString &text);
    bool updateIconOf(int row);
    void stopEverythingThatsRunning(short excludingFeature);
    void pauseEverythingThatsRunning();
    bool websiteConfiguredCorrectly();
    void updatePotdProgress();
    QString fixBasenameSize(const QString &basename);
    void actionsOnWallpaperChange();
    void processRunningResetPictures();
    void forceUpdateIconOf(int index);
    void prepareWebsiteSnapshot();
    QString base64Encode(const QString &string);
    static void nextKeySignal(const char *, void *);
    void disableLiveWebsitePage();
    QImage scaleWallpapersPreview(QString filename);
    void iconsPathsChanged();
    bool addingImageStylesNow = false;
    void changeRunningFeature(FeatureController::Feature feature);

public Q_SLOTS:
    void addFolderForMonitor(const QString &folder);
    void justChangeWallpaper();
    void handleStartButtonClick();
    void handlePreviousButtonClick();
    void handleNextButtonClick();
    void handleActivateLiveEarthClick();
    void handleActivatePotdClick();
    void handleActivateWebsiteClick();
    void closeWhatsRunning();
    void handlePreferencesAction();
    void handleAboutAction();
    void doQuit();
    void showNormal();
    void hideOrShow();

private Q_SLOTS:
#ifdef Q_OS_LINUX
    void dconfChanges();
#endif
    void handleSearchShortcut();
    void timeSpinboxChanged();
    void intervalTypeChanged();
    void checkBatteryStatus();
    void updateTiming();
    void changePathsToIcons();
    void changeIconsToPaths();
    void launchTimerToUpdateIcons();
    void updateVisibleIcons();
    void readCoordinates(const QRect &cropArea);
    void findAvailableWallpaperStyles();
    void setWebsitePreviewImage(QImage *image);
    void setImage(bool addToPrevious, const QString &image, int index);
    void setButtonColor();
    void enterPressed();
    void openCloseAddLoginAnimationFinished();
    void updateSeconds();
    void liveWebsiteImageCreated(QImage *image, short errorCode);
    void changeCurrentTheme();
    void escapePressed();
    void previousPage();
    void nextPage();
    void hideTimeForNext();
    void hidePreview();
    void showPreview();
    void removeBottomwidgets();
    void bottomWidgetsAnimation();
    void setAverageColor(const QString &image);
    void updateScreenLabel();
    void picturesLocationsChanged();
    void preferencesDestroyed();
    void lePointDestroyed();
    void historyDestroyed();
    void aboutDestroyed();
    void locationsDestroyed();
    void websitePreviewDestroyed();
    void colorsGradientsDestroyed();
    void potdViewerDestroyed();
    void potdPreviewDestroyed();
    void delayed_pictures_location_change();
    void restartPotdIfRunningAfterSettingChange();
    void restartLeIfRunningAfterSettingChange();
    void setPreviewImage();
    void unhoverMenuButton();
    void beginFixCacheForFolders();
    void wait_preview_changed();
    void preview_changed();
    void deChanged();
    void update_website_settings();
    void onlineRequestFailed();
    void onlineImageRequestReady(QString image);
    void handleHistoryAction();
    void handleContentsAction();
    void handleTimerSliderChange(int value);
    void openWebsitePreview(bool);
    void handleWebsiteCropCheck(bool checked);
    void handleSetDesktopColorClick();
    void handleImageStyleChange(int index);
    void handleWebsiteTextEdit(const QString &arg1);
    void handleBrowseFoldersClick();
    void handleWebsiteSliderChange(int value);
    void handleLiveEarthTagCheck(bool checked);
    void handleLiveEarthTagClick();
    void handlePicturesLocationChange(int index);
    void handleDonateAction();
    void handleReportBugAction();
    void handleHelpOnlineAction();
    void handleScreenResolutionAction();
    void handlePotdViewerClick();
    void handleAddLoginDetailsCheck(bool checked);

    void handleFeatureStart();

    void handleStopButtonClick();
    void handleDeactivateLiveEarthClick();
    void handleDeactivatePotdClick();
    void handleDeactivateWebsiteClick();

    void handlePageButtonClick(int btn);
    void handleIncludeDescriptionCheck(bool checked);
    void handleEditPotdClick();
    void handleShuffleImagesCheck();
    void handlePageChange(int page);
    void handleEditLocationsClick();
    void updateImageStyleCombo();
    void getScreenResolution(QRect geometry);
    void getScreenAvailableResolution(QRect geometry);

    void handleDaysSpinBoxChange(int arg1);
    void handleHoursSpinBoxChange(int arg1);
    void handleMinutesSpinBoxChange(int arg1);
    void handleSecondsSpinBoxChange(int arg1);
    void clearWallpapersList();

    // File System Watcher
    void prepareToSearchFolders();
    void monitoredFoldersUpdated();
    void currentFolderDoesNotExist();

    // 'Wallpapers' ListWidget functions
    void handleWallpaperListContextMenu();
    void handleWallpaperListDoubleClick();
    void handleWallpaperListSelectionChange();
    void deletePressed();
    void addPicturesToWallpaperList(const QStringList &pictures);

    // 'Wallpapers' ListWidget right-click menu functions
    void startWithThisImage();
    void openImage();
    void removeImageFromDisk();
    void removeImagesFromDisk();
    void rotateRight();
    void rotateLeft();
    void rotationCompleted(QString &imagePath);
    void copyImage();
    void copyImagePath();
    void openImageFolder();
    void openImageFolderMassive();
    void showProperties();

Q_SIGNALS:
     void fixLivewebsiteButtons();
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
