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

#include <QtConcurrent/QtConcurrentRun>
#include <QMimeData>
#include <QDesktopServices>
#include <QRadioButton>
#include <QFileDialog>
#include <QNetworkReply>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QStandardItem>
#include <QColorDialog>
#include <QShortcut>

#ifdef Q_OS_WIN
#include <QtGui/private/qzipreader_p.h>
#endif

#include "mainwindow.h"

#include "math.h"

MainWindow *mainWindowInstance;

MainWindow::MainWindow(QSharedMemory *attachedMemory, Global *globalParser, ImageFetcher *imageFetcher, WebsiteSnapshot *websiteSnapshot, WallpaperManager *wallpaperManager, int timeoutCount, int lastRandomDelay, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWindowInstance = this;

    attachedMemory_ = attachedMemory;

    initializePrivateVariables(globalParser, imageFetcher);

    wallpaperManager_ = wallpaperManager;
    websiteSnapshot_ = websiteSnapshot;

    QDesktopWidget desktopWidget;

    availableGeometry_ = desktopWidget.availableGeometry(desktopWidget.screen());
    imagePreviewResizeFactorX_ = SCREEN_LABEL_SIZE_X*2.0/(gv.screenWidth*1.0);
    imagePreviewResizeFactorY_ = SCREEN_LABEL_SIZE_Y*2.0/(gv.screenHeight*1.0);

    setupMenu();
    connectSignalSlots();
    retrieveSettings();
    resetWatchFolders();
    setupTimers();
    setupKeyboardShortcuts();
    applySettings();
    continueAlreadyRunningFeature(timeoutCount, lastRandomDelay);
    setAcceptDrops(true);


    //Moving the window to the center of screen!
    this->move(desktopWidget.availableGeometry().center() - this->rect().center());

    processingOnlineRequest_ = false;

    //processing request (loading gif)
    ui->process_request_label->hide();
    processingRequestGif_ = new QMovie(this);
    processingRequestGif_->setFileName(":/images/process_request.gif");
    ui->process_request_label->setMovie(processingRequestGif_);

    //for manually searching for files or re-selecting a picture after a folder contents have changed
    match_ = new QRegExp();
    match_->setCaseSensitivity(Qt::CaseInsensitive);
    match_->setPatternSyntax(QRegExp::Wildcard);

    QTimer::singleShot(10, this, SLOT(setButtonColor()));
    QTimer::singleShot(20, this, SLOT(setStyle()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    strongMinimize();
    this->hide();
    event->ignore();
}

void MainWindow::actionsOnClose()
{
    if(loadedPages_[0]){
        savePicturesLocations();
    }
    appAboutToClose_ = true;
    this->hide();
}

void MainWindow::resizeEvent(QResizeEvent *e){
    if(!gv.mainwindowLoaded){
        return;
    }
    if(ui->stackedWidget->currentIndex() == 0){
        launchTimerToUpdateIcons();
    }

    //screen preview is not getting updated when we resize the window, so we do it manually
    if(ui->screen_label->pixmap())
    {
        if(gv.previewImagesOnScreen && gv.mainwindowLoaded)
        {
            QPixmap *pixmap = new QPixmap(*ui->screen_label->pixmap());
            if(pixmap){
                ui->screen_label_transition->setPixmap(*pixmap);
            }
            else
            {
                ui->screen_label_transition->clear();
            }

            ui->screen_label->setPixmap(*pixmap);
            delete pixmap;
        }
    }
    else
    {
        QString temp = ui->screen_label_text->text();
        ui->screen_label_text->clear();
        ui->screen_label_text->setText(temp);
    }

    QMainWindow::resizeEvent(e);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
    if (object == settingsMenu_ && event->type() == QEvent::Show)
    {
        settingsMenu_->move(calculateSettingsMenuPos());
        return true;
    }
    return false;
}

QPoint MainWindow::calculateSettingsMenuPos(){
    int menuWidth = settingsMenu_->width();
    int menuHeight = settingsMenu_->height();
    int buttonWidth = ui->menubarMenu->width();
    int buttonHeight = ui->menubarMenu->height();
    QPoint globalCoords = ui->menubarMenu->mapToGlobal(QPoint(0, buttonHeight));

    //default value, the menu fits just fine
    QPoint showPoint = globalCoords - QPoint((menuWidth-buttonWidth), 0);

    QRect settingsMenuRect(showPoint.x(), showPoint.y(), menuWidth, menuHeight);

    if(availableGeometry_.contains(settingsMenuRect, true)){
        //menu fits
        return showPoint;
    }
    else
    {
        //menu doesn't fit
        QRect intersection = availableGeometry_.intersected(settingsMenuRect);

        if(intersection.width() < menuWidth){
            //width doesn't fit
            if(intersection.x() == 0){
                //it doesn't fit to the left, move to the right
                showPoint.setX(0);
            }
            else
            {
                //it doesn't fit to the right, move to the left
                showPoint.setX(availableGeometry_.width()-menuWidth);
            }
        }

        if(intersection.height() < menuHeight){
            //height doesn't fit

            if((intersection.y()-buttonHeight-availableGeometry_.y()) > menuHeight){
                //it fits to the top just fine
                showPoint.setY(globalCoords.y()-buttonHeight-menuHeight);
            }
            else
            {
                //it doesn't fit all above, fit it to the left/right
                showPoint.setY(availableGeometry_.y()+availableGeometry_.height()-menuHeight);

                bool fitsLeft = true; //does it fit to move to the left?

                if(intersection.x()-buttonWidth < 0){
                    fitsLeft = false;
                }

                if(fitsLeft){
                    //fit to left
                    showPoint.setX(intersection.x()-buttonWidth);
                }
                else
                {
                    //fit to right
                    showPoint.setX(globalCoords.x()+buttonWidth);
                }
            }
        }
    }

    return showPoint;
}

void MainWindow::strongShowApp(){
    if(this->isActiveWindow()){
        return;
    }

    this->show();
    this->activateWindow();
}

void MainWindow::hideOrShow()
{
    if (this->isHidden()) {
        strongShowApp();
    } else {
        strongMinimize();
        this->hide();
    }
}

void MainWindow::strongMinimize(){
    this->setWindowState(Qt::WindowMaximized); //dirty trick
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::connectSignalSlots(){
    connect(imageFetcher_, SIGNAL(fail()), this, SLOT(onlineRequestFailed()));
    connect(imageFetcher_, SIGNAL(success(QString)), this, SLOT(onlineImageRequestReady(QString)));
    connect(cacheManager_, SIGNAL(requestCurrentFolders()), this, SLOT(beginFixCacheForFolders()));
    connect(ui->month_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->day_month_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->day_week_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->am_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->hour_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->minutes_checkBox, SIGNAL(clicked()), this, SLOT(clockCheckboxClicked()));
    connect(ui->website, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->website_slider, SIGNAL(valueChanged(int)), this, SLOT(update_website_settings()));
    connect(ui->website_crop_checkbox, SIGNAL(clicked()), this, SLOT(update_website_settings()));
    connect(ui->add_login_details, SIGNAL(clicked()), this, SLOT(update_website_settings()));
    connect(ui->username, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->password, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->final_webpage, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->wallpapersList->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(launchTimerToUpdateIcons()));
    connect(scaleWatcher_, SIGNAL(finished()), this, SLOT(setupAnimationsAndChangeImage()));
    connect(clocksPreviewWatcher_, SIGNAL(finished()), this, SLOT(clocksPreviewGenerationFinished()));
    connect(settingsMenu_, SIGNAL(aboutToHide()), this, SLOT(unhoverMenuButton()));
    connect(ui->days_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->hours_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->minutes_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->seconds_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
}

void MainWindow::retrieveSettings()
{
    gv.potdOnlineUrl = settings->value("potd_online_url", POTD_ONLINE_URL).toString();
    gv.potdOnlineUrlB = settings->value("potd_online_urlB", POTD_ONLINE_URL_B).toString();
    gv.liveEarthOnlineUrl = settings->value("line_earth_online_url", LIVEARTH_ONLINE_URL).toString();
    gv.liveEarthOnlineUrlB = settings->value("live_earth_online_urlB", LIVEARTH_ONLINE_URL_B).toString();
#ifdef Q_OS_UNIX
    gv.unityProgressbarEnabled = settings->value("unity_progressbar_enabled", false).toBool();
#endif

    gv.independentIntervalEnabled = settings->value("independent_interval_enabled", true).toBool();
    gv.rotateImages = settings->value("rotation", false).toBool();
    gv.firstTimeout = settings->value("first_timeout", false).toBool();
    gv.symlinks = settings->value("symlinks", false).toBool();
    gv.showNotification = settings->value("notification", false).toBool();
    gv.saveHistory = settings->value("history", true).toBool();
    gv.pauseOnBattery = settings->value("pause_on_battery", false).toBool();
    gv.setAverageColor = settings->value("average_color", false).toBool();
    gv.typeOfInterval = settings->value("typeOfIntervals", 0).toInt();
    gv.randomTimeFrom = settings->value("random_time_from", 300).toInt();
    gv.randomTimeTo = settings->value("random_time_to", 1200).toInt();
    gv.iconMode = settings->value("icon_style", true).toBool();
    gv.defaultWallpaperClock = settings->value("default_wallpaper_clock", "").toString();
    gv.randomImagesEnabled = settings->value("random_images_enabled", true).toBool();
    gv.previewImagesOnScreen = settings->value("preview_images_on_screen", true).toBool();
}

void MainWindow::resetWatchFolders(){
    /*
     * Unfortunately, this is the only way to completely reset QFileSystemWatcher, because
     * the removePaths(s) function fails miserably most of the times.
     */
    if(gv.mainwindowLoaded){
        delete watchFolders_;
    }
    watchFolders_ = new QFileSystemWatcher(this);
    connect(watchFolders_, SIGNAL(directoryChanged(QString)), this, SLOT(folderChanged()));
}

void MainWindow::setupTimers(){
    //Timer to updates progressbar and launch the changing process
    updateSecondsTimer_ = new QTimer(this);
    updateSecondsTimer_->setTimerType(Qt::VeryCoarseTimer);
    connect(updateSecondsTimer_, SIGNAL(timeout()), this, SLOT(updateSeconds()));

    //Timer for folder monitoring
    researchFoldersTimer_ = new QTimer(this);
    connect(researchFoldersTimer_, SIGNAL(timeout()), this, SLOT(researchFolders()));

    //Timer that updates the configuration for timing once it is changed
    updateCheckTime_ = new QTimer(this);
    connect(updateCheckTime_, SIGNAL(timeout()), this, SLOT(updateTiming()));

    //Timer if you click one of the checkboxes of wallpaper clock, if wallpaper clock is running
    wallpaperClockWait_ = new QTimer(this);
    wallpaperClockWait_->setSingleShot(true);
    connect(wallpaperClockWait_, SIGNAL(timeout()), this, SLOT(clockCheckboxClickedWhileRunning()));

    //Timer that update the icons of the visible items
    iconUpdater_ = new QTimer(this);
    connect(iconUpdater_, SIGNAL(timeout()), this, SLOT(updateVisibleIcons()));
    iconUpdater_->setSingleShot(true);

    //Timer to hide progressbar
    hideProgress_ = new QTimer(this);
    connect(hideProgress_, SIGNAL(timeout()), this, SLOT(hideTimeForNext()));
    hideProgress_->setSingleShot(true);
#ifdef Q_OS_UNIX
    //Timer to check battery status after laptop is unplugged
    batteryStatusChecker_ = new QTimer(this);
    connect(batteryStatusChecker_, SIGNAL(timeout()), this, SLOT(checkBatteryStatus()));
#endif
}

void MainWindow::setupKeyboardShortcuts(){
    (void) new QShortcut(Qt::Key_Escape, this, SLOT(escapePressed()));
    (void) new QShortcut(Qt::Key_Delete, this, SLOT(deletePressed()));
    (void) new QShortcut(Qt::ALT + Qt::Key_Return, this, SLOT(showProperties()));
    (void) new QShortcut(Qt::ALT + Qt::Key_1, this, SLOT(on_page_0_wallpapers_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_2, this, SLOT(on_page_1_earth_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_3, this, SLOT(on_page_2_potd_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_4, this, SLOT(on_page_3_clock_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_5, this, SLOT(on_page_4_web_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_6, this, SLOT(on_page_5_other_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_1, this, SLOT(on_page_0_wallpapers_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_2, this, SLOT(on_page_1_earth_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_3, this, SLOT(on_page_2_potd_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_4, this, SLOT(on_page_3_clock_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_5, this, SLOT(on_page_4_web_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_6, this, SLOT(on_page_5_other_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(showHideSearchBox()));
    (void) new QShortcut(Qt::Key_Return, this, SLOT(enterPressed()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousPage()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextPage()));
    (void) new QShortcut(Qt::ALT + Qt::Key_F4, this, SLOT(escapePressed()));
}

void MainWindow::changeCurrentThemeTo(const QString &theme)
{
    gv.currentTheme=theme;
    if(theme=="ambiance")
    {
        setThemeToAmbiance();
    }
    else if(theme=="radiance")
    {
        setThemeToRadiance();
    }
    else
    {
        switch(globalParser_->autodetectTheme()){
        default:
        case 0:
            setThemeToAmbiance();
            break;
        case 1:
            setThemeToRadiance();
            break;
        }
    }
}

void MainWindow::initializePrivateVariables(Global *globalParser, ImageFetcher *imageFetcher){
    globalParser_ = (globalParser == NULL) ? new Global() : globalParser;
    imageFetcher_ = (imageFetcher == NULL) ? new ImageFetcher() : imageFetcher;
    cacheManager_ = new CacheManager();
    opacityEffect_ = new QGraphicsOpacityEffect(this);
    opacityEffect2_ = new QGraphicsOpacityEffect(this);
    clocksRadioButtonsGroup = new QButtonGroup(this);
    scaleWatcher_ = new QFutureWatcher<QImage>(this);
    clocksPreviewWatcher_ = new QFutureWatcher<QImage>(this);
}

void MainWindow::setupMenu()
{
    //We decided this is better than having a menubar
    settingsMenu_ = new QMenu(this);
    currentBgMenu_ = new QMenu(this);
    settingsMenu_->addAction(tr("Preferences"), this, SLOT(on_action_Preferences_triggered()), QKeySequence(tr("Ctrl+P")));
    settingsMenu_->addSeparator();
    currentBgMenu_->setTitle(tr("Current Background"));
    currentBgMenu_->addAction(tr("Open Image"), this, SLOT(on_actionOpen_Image_triggered()));
    currentBgMenu_->addAction(tr("Open Folder"), this, SLOT(on_actionOpen_Folder_triggered()));
    currentBgMenu_->addAction(tr("Copy Image"), this, SLOT(on_actionCopy_Image_triggered()));
    currentBgMenu_->addAction(tr("Copy Path"), this, SLOT(on_actionCopy_Path_triggered()));
    currentBgMenu_->addAction(tr("Delete"), this, SLOT(on_actionDelete_triggered()));
    currentBgMenu_->addAction(tr("Properties"), this, SLOT(on_actionProperties_triggered()));
    settingsMenu_->addMenu(currentBgMenu_);
    settingsMenu_->addSeparator();
    settingsMenu_->addAction(tr("History"), this, SLOT(on_actionHistory_triggered()), QKeySequence(tr("Ctrl+H")));
    settingsMenu_->addAction(tr("Statistics"), this, SLOT(on_actionStatistics_triggered()));
    settingsMenu_->addAction(tr("Download 1000 HD Wallpapers"), this, SLOT(on_actionDownload_triggered()));
    settingsMenu_->addAction(tr("What is my screen resolution?"), this, SLOT(on_actionWhat_is_my_screen_resolution_triggered()));
    settingsMenu_->addSeparator();
    settingsMenu_->addAction(tr("About Wallch"), this, SLOT(on_action_About_triggered()));
    helpMenu_ = new QMenu(this);
    helpMenu_->setTitle(tr("Help"));
    helpMenu_->addAction(tr("How to use Wallch?"), this, SLOT(on_actionContents_triggered()), QKeySequence(tr("F1")));
    helpMenu_->addAction(tr("Ask a question"), this, SLOT(on_actionGet_Help_Online_triggered()));
    helpMenu_->addAction(tr("Report a bug"), this, SLOT(on_actionReport_A_Bug_triggered()));
    settingsMenu_->addMenu(helpMenu_);
    settingsMenu_->addAction(tr("Donate"), this, SLOT(on_actionDonate_triggered()));
    settingsMenu_->addSeparator();
    settingsMenu_->addAction(tr("Quit"), this, SLOT(doQuit()), QKeySequence(tr("Ctrl+Q")));
    settingsMenu_->installEventFilter(this);

    ui->menubarMenu->setMenu(settingsMenu_);

    //storing the menu separators for mass operations
    menuSeparators_.append(ui->sep1);
    menuSeparators_.append(ui->sep2);
    menuSeparators_.append(ui->sep3);
    menuSeparators_.append(ui->sep4);
    menuSeparators_.append(ui->sep5);
    menuSeparators_.append(ui->sep6);
}

void MainWindow::applySettings()
{
    changeCurrentThemeTo(settings->value("theme", "autodetect").toString());

    int maxCacheIndex = settings->value("max_cache_size", 2).toInt();
    if(maxCacheIndex < 0 || maxCacheIndex >= cacheManager_->maxCacheIndexes.size()){
        maxCacheIndex = 2;
    }

    cacheManager_->setMaxCache(cacheManager_->maxCacheIndexes.value(maxCacheIndex).second);

    if(gv.iconMode){
        ui->wallpapersList->setIconSize(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->wallpapersList->setUniformItemSizes(true);
    }
    else
    {
        ui->wallpapersList->setViewMode(QListView::ListMode);
    }

    //setting the slider's value...
    secondsInWallpapersSlider_ = settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();

    if(!gv.previewImagesOnScreen)
    {
        ui->horizontalLayout_preview_off->addWidget(ui->set_desktop_color);
        ui->horizontalLayout_preview_off->addWidget(ui->image_style_combo);
        ui->horizontalLayout_preview_off->addWidget(ui->timeForNext);
        ui->right_widget->hide();
        ui->right_widget->setMaximumWidth(0);
        ui->timeForNext->setMinimumWidth(300);
    }
    else
    {
        ui->widget_3->hide();
        ui->widget_3->setMaximumHeight(0);
    }

    if(gv.randomTimeFrom>gv.randomTimeTo-3){
        //The randomness is misconfigured, apply default values
        ui->random_from->setValue(10);
        ui->random_to->setValue(20);
        ui->random_time_from_combobox->setCurrentIndex(1);
        ui->random_time_to_combobox->setCurrentIndex(1);
        gv.randomTimeFrom=600;
        gv.randomTimeTo=1200;
    }

    if(gv.randomImagesEnabled){
        srand(time(0));
    }

    ui->shuffle_images_checkbox->setChecked(gv.randomImagesEnabled);

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::LXDE){
        ui->set_desktop_color->hide();
    }
#endif
}

void MainWindow::continueAlreadyRunningFeature(int timeoutCount, int lastRandomDelay)
{
    if(gv.wallpapersRunning)
    {
        loadWallpapersPage();

        if(ui->wallpapersList->count() < LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else{
            startButtonsSetEnabled(true);
        }

        if(gv.processPaused){
            actAsStart_=true;
            ui->startButton->setText(tr("&Start"));
            ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
            animateProgressbarOpacity(1);
            findSeconds(true);
            secondsLeft_=(timeoutCount+1);
            initialRandomSeconds_=lastRandomDelay;
            globalParser_->resetSleepProtection(secondsLeft_);
            updateSeconds();
            stopButtonsSetEnabled(true);
#ifdef Q_OS_UNIX
            if(gv.currentDE == DesktopEnvironment::UnityGnome){
                dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
            }
#endif
            previousAndNextButtonsSetEnabled(false);
            ui->timeForNext->setFormat(ui->timeForNext->format()+" - Paused.");
        }
        else
        {
            //starting the process...
            actAsStart_=false;
            ui->startButton->setText(tr("Pau&se"));
            ui->startButton->setIcon(QIcon::fromTheme("media-playback-pause", QIcon(":/images/media-playback-pause.png")));
            animateProgressbarOpacity(1);
            findSeconds(true);
            startWasJustClicked_=true;
            secondsLeft_=timeoutCount;
            initialRandomSeconds_=lastRandomDelay;
            startUpdateSeconds();
            stopButtonsSetEnabled(true);
            previousAndNextButtonsSetEnabled(true);
        }
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if(gv.liveEarthRunning)
    {
        secondsLeft_=timeoutCount;
        ui->activate_livearth->setEnabled(false);
        ui->deactivate_livearth->setEnabled(true);
        startUpdateSeconds();
        on_page_1_earth_clicked();
        animateProgressbarOpacity(1);
    }
    else if(gv.potdRunning)
    {
        ui->deactivate_potd->setEnabled(true);
        ui->activate_potd->setEnabled(false);
        on_page_2_potd_clicked();
        startPotd(false);
    }
    else if(gv.wallpaperClocksRunning)
    {
        ui->deactivate_clock->setEnabled(true);
        ui->activate_clock->setEnabled(false);
        totalSeconds_=secondsLeft_=timeoutCount;
        startUpdateSeconds();
        setProgressbarsValue(100);
        on_page_3_clock_clicked();
        animateProgressbarOpacity(1);
    }
    else if(gv.liveWebsiteRunning)
    {
        secondsLeft_=timeoutCount;
        ui->deactivate_website->setEnabled(true);
        ui->activate_website->setEnabled(false);
        startUpdateSeconds();
        on_page_4_web_clicked();
        animateProgressbarOpacity(1);
    }
    else
    {   //nothing's running, so just open the application..

        stopButtonsSetEnabled(false);
        previousAndNextButtonsSetEnabled(false);

        hideTimeForNext();

        switch(settings->value("current_page", 0).toInt()){
        default:
        case 0:
            on_page_0_wallpapers_clicked();
            break;
        case 1:
            on_page_1_earth_clicked();
            break;
        case 2:
            on_page_2_potd_clicked();
            break;
        case 3:
            on_page_3_clock_clicked();
            break;
        case 4:
            on_page_4_web_clicked();
            break;
        case 5:
            on_page_5_other_clicked();
            break;
        }

        //the app has opened normally, so there is no point in keeping a previous independent interval
        if(gv.independentIntervalEnabled){
            globalParser_->saveSecondsLeftNow(-1, 0);
        }
    }
}

void MainWindow::closeWhatsRunning(){
    if(gv.wallpapersRunning){
        on_stopButton_clicked();
    }
    else if(gv.liveEarthRunning){
        on_deactivate_livearth_clicked();
    }
    else if(gv.potdRunning){
        on_deactivate_potd_clicked();
    }
    else if(gv.wallpaperClocksRunning){
        on_deactivate_clock_clicked();
    }
    else if(gv.liveWebsiteRunning){
        on_deactivate_website_clicked();
    }
}

QString MainWindow::base64Encode(const QString &string){
    return QByteArray().append(string).toBase64();
}

void MainWindow::onlineRequestFailed(){
    globalParser_->error("Something went wrong with your online request... Skipping this time.");
    processRequestStop();
}

void MainWindow::onlineImageRequestReady(QString image){
    WallpaperManager::setBackground(image, true, gv.potdRunning||gv.liveEarthRunning, (gv.potdRunning ? 3 : 2));
    if(gv.setAverageColor){
        setButtonColor();
    }
    imageTransition(false, image);
    processRequestStop();
}

void MainWindow::escapePressed(){
    if(ui->stackedWidget->currentIndex() == 0 && ui->search_box->hasFocus() && searchIsOn_){
        on_search_close_clicked();
    }
    else
    {
        strongMinimize();
    }
}

void MainWindow::deletePressed(){
    if(ui->stackedWidget->currentIndex() == 0 && ui->wallpapersList->selectedItems().count() > 0){
        if(ui->wallpapersList->selectedItems().count() > 1){
            removeImagesFromDisk();
        }
        else
        {
            removeImageFromDisk();
        }
    }
    else if(ui->stackedWidget->currentIndex() == 3 && ui->clocksTableWidget->selectedItems().count() > 0){
        uninstall_clock();
    }
}

void MainWindow::startUpdateSeconds(){
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }

    updateSeconds();
    updateSecondsTimer_->start(1000);
    if(ui->timeForNext->isHidden()){
        ui->timeForNext->show();
#ifdef Q_OS_UNIX
        if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityProgressbarEnabled){
            globalParser_->setUnityProgressBarEnabled(true);
        }
#endif
    }
}

void MainWindow::updatePotdProgress(){
    int secsToPotd = globalParser_->getSecondsTillHour("00:00");

    if(secsToPotd<=3600){
        //less than one hour left
        ui->timeForNext->setFormat(secondsToHms(secsToPotd));
    }
    else
    {
        ui->timeForNext->setFormat(secondsToHm(secsToPotd));
    }

    setProgressbarsValue(short(((float) (secsToPotd/86400.0)) * 100));
}

void MainWindow::setProgressbarsValue(short value){
    ui->timeForNext->setValue(value);
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityProgressbarEnabled){
        globalParser_->setUnityProgressbarValue((float)(value/100.0));
    }
#endif
}

void MainWindow::imageTransition(bool wallpaperClock, const QString &filename /*=QString()*/)
{
    if(!gv.previewImagesOnScreen || !gv.mainwindowLoaded){
        return;
    }

    if(scaleWatcher_->isRunning()){
        scaleWatcher_->cancel();
    }

    if(clocksPreviewWatcher_->isRunning()){
        clocksPreviewWatcher_->cancel();
    }

    if(wallpaperClock){
        //generate the wallpaper clock preview
        clocksPreviewWatcher_->setFuture(QtConcurrent::run(this, &MainWindow::wallpaperClocksPreview));
    }
    else
    {
        //scale the image
        scaleWatcher_->setFuture(QtConcurrent::run(this, &MainWindow::scaleWallpapersPreview, filename));
    }
}

void MainWindow::clocksPreviewGenerationFinished(){
    if(clocksPreviewWatcher_->isCanceled()){
        //last moment cancel, if we don't return, we crash
        return;
    }
    //the wallpaper clock preview has been generated. Now scale it
    scaleWatcher_->setFuture(QtConcurrent::run(this, &MainWindow::scaleWallpapersPreview));
}

QImage MainWindow::scaleWallpapersPreview(){
    if(clocksPreviewWatcher_->isCanceled() || scaleWatcher_->isCanceled()){
        return QImage();
    }
    QImage image = clocksPreviewWatcher_->future().result();
    return QImage(image.scaled(QSize(image.width()*imagePreviewResizeFactorX_, image.height()*imagePreviewResizeFactorY_), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QImage MainWindow::scaleWallpapersPreview(QString filename){
    QImageReader reader(filename);
    reader.setDecideFormatFromContent(true);
    reader.setScaledSize(QSize(reader.size().width()*imagePreviewResizeFactorX_, reader.size().height()*imagePreviewResizeFactorY_));
    return reader.read();
}

void MainWindow::setupAnimationsAndChangeImage(){
    if(ui->screen_label->pixmap())
    {
        ui->screen_label_transition->setPixmap(*ui->screen_label->pixmap());
    }
    else
    {
        ui->screen_label_transition->clear();
    }

    ui->screen_label_text->clear();

    opacityEffect_->setOpacity(false);
    ui->screen_label->setGraphicsEffect(opacityEffect_);
    QPropertyAnimation* increaseOpacityAnimation = new QPropertyAnimation(this);
    increaseOpacityAnimation->setTargetObject(opacityEffect_);
    increaseOpacityAnimation->setPropertyName("opacity");
    increaseOpacityAnimation->setDuration(IMAGE_TRANSITION_SPEED);
    increaseOpacityAnimation->setStartValue(opacityEffect_->opacity());
    increaseOpacityAnimation->setEndValue(1);
    increaseOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
    increaseOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    if(ui->screen_label->isHidden()){
        ui->screen_label->show();
    }

    opacityEffect2_->setOpacity(true);
    ui->screen_label_transition->setGraphicsEffect(opacityEffect2_);
    QPropertyAnimation* decreaseOpacityAnimation = new QPropertyAnimation(this);
    decreaseOpacityAnimation->setTargetObject(opacityEffect2_);
    decreaseOpacityAnimation->setPropertyName("opacity");
    decreaseOpacityAnimation->setDuration(IMAGE_TRANSITION_SPEED);
    decreaseOpacityAnimation->setStartValue(opacityEffect2_->opacity());
    decreaseOpacityAnimation->setEndValue(0);
    decreaseOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
    decreaseOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    if(!scaleWatcher_->isCanceled()){
        setPreviewImage();
    }
}

void MainWindow::unhoverMenuButton(){
    /*todo how to make this shiet work*/
    QMouseEvent *event1 = new QMouseEvent(QEvent::MouseMove, QCursor::pos()+QPoint(1, 1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::postEvent(ui->menubarMenu, event1, Qt::HighEventPriority);
    QApplication::sendEvent(ui->menubarMenu, event1);
    QHoverEvent *event2 = new QHoverEvent(QEvent::HoverLeave, QPoint(-1, -1), QPoint(1, 1));
    QApplication::postEvent(ui->menubarMenu, event2, Qt::HighEventPriority);
    QApplication::sendEvent(ui->menubarMenu, event2);
}

void MainWindow::hideScreenLabel()
{
    if(!gv.previewImagesOnScreen){
        return;
    }

    if(ui->screen_label->pixmap()){
        ui->screen_label_transition->setPixmap(QPixmap::fromImage(ui->screen_label->pixmap()->toImage()));
    }
    else
    {
        ui->screen_label_transition->clear();
        return;
    }

    ui->screen_label->clear();

    opacityEffect2_->setOpacity(true);
    ui->screen_label_transition->setGraphicsEffect(opacityEffect2_);
    QPropertyAnimation* anim = new QPropertyAnimation(this);
    anim->setTargetObject(opacityEffect2_);
    anim->setPropertyName("opacity");
    anim->setDuration(IMAGE_TRANSITION_SPEED);
    anim->setStartValue(opacityEffect2_->opacity());
    anim->setEndValue(false);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::setPreviewImage(){
    if(!gv.previewImagesOnScreen || scaleWatcher_->isCanceled()){
        return;
    }

    QImage image = scaleWatcher_->result();

    if(!image.isNull()){
#ifdef Q_OS_UNIX

        DesktopStyle desktopStyle = getDesktopStyle();

        if(image.format() == QImage::Format_Indexed8){
            //painting on QImage::Format_Indexed8 is not supported
            image = WallpaperManager::indexed8ToARGB32(image);
        }
        QImage colorsGradientsImage(75, 75, QImage::Format_RGB32);

        //bring me some food right meow!
        bool previewColorsAndGradientsMeow=true;

        if(desktopStyle == Tile || desktopStyle == Stretch){
            //the preview of the background color(s) is meaningless for the current styling
            previewColorsAndGradientsMeow = false;
        }
        ColoringType::Value coloringType = globalParser_->getColoringType();
        if(previewColorsAndGradientsMeow || desktopStyle == NoneStyle){
            if(desktopStyle == NoneStyle){
                image = QImage();
            }
            QString primaryColor;
            if(gv.setAverageColor)
            {
                primaryColor = WallpaperManager::getAverageColorOf(image).name();
            }
            else
            {
                primaryColor = globalParser_->getPrimaryColor();
            }

            colorsGradientsImage.fill(primaryColor);

            if(coloringType != ColoringType::SolidColor && coloringType != ColoringType::NoneColor)
            {
                QString secondaryColor=globalParser_->getSecondaryColor();

                short alpha=255;
                short softness=0;

                QPainter painter(&colorsGradientsImage);
                QColor color(secondaryColor);

                for(short i=75;i>0;i--){
                    if(!softness){
                        alpha-=1;
                        if(alpha<=244){
                            softness=1;
                        }
                    }
                    else if(softness==1){
                        alpha-=3;
                        if(alpha<=205){
                            softness=2;
                        }
                    }
                    else if(softness==2){
                        alpha-=4;
                    }

                    color.setAlpha(alpha);
                    painter.fillRect(i-1, 0, 1, 75, QColor(color));
                }
                painter.end();

                if(coloringType == ColoringType::VerticalColor){
                    //just rotate the image
                    colorsGradientsImage = colorsGradientsImage.transformed(QTransform().rotate(90), Qt::SmoothTransformation);
                }
            }
        }
        int vScreenWidth= int((float)imagePreviewResizeFactorX_*gv.screenWidth);
        int vScreenHeight=int((float)imagePreviewResizeFactorY_*gv.screenHeight);

        //making the preview exactly like it will be shown at the desktop
        switch(desktopStyle){
        //Many thanks to http://askubuntu.com/questions/226816/background-setting-cropping-options
        case Tile:
        {
            //Tile (or "wallpaper" picture-option)
            /*
             *  How it works (in Unity-Gnome):
             *  We have to repeat the image in case its width or height is less than the screen's
             *  If only one of the width or height are less then the screen's, then the one that
             *  is greater or equal is centered
             */
            if(image.width() < vScreenWidth || image.height() < vScreenHeight){
                QImage originalImage = image;
                //find how many times (rounded UPWARDS)
                short timesX = (vScreenWidth+originalImage.width()-1)/originalImage.width();
                short timesY = (vScreenHeight+originalImage.height()-1)/originalImage.height();

                image = QImage(image.copy(0, 0, vScreenWidth, vScreenHeight));
                image.fill(Qt::black);

                QPainter painter(&image);

                if(timesX>1 && timesY>1){
                    //both width and height of the image are less than the screen's
                    for(short i=0; i<timesX; i++){
                        for(short j=0; j<timesY; j++){
                            painter.drawPixmap(originalImage.width()*i, originalImage.height()*j, QPixmap::fromImage(originalImage));
                        }
                    }
                }
                else if(timesX>1){
                    //timesY equals 1, this means the image's height is greater or equal to gv.screenHeight. So, the image must be centered on the height.
                    short yPos = 0-(originalImage.height()-vScreenHeight)/2.0;
                    for(short i=0; i<timesX; i++){
                        painter.drawPixmap(originalImage.width()*i, yPos, QPixmap::fromImage(originalImage));
                    }
                }
                else
                {
                    //timesX equals 1, this means the image's width is greater or equal to gv.screenWidth. So, the image must be centered on the width.
                    short xPos = 0-(originalImage.width()-vScreenWidth)/2.0;
                    for(short j=0; j<timesY; j++){
                        painter.drawPixmap(xPos, originalImage.height()*j, QPixmap::fromImage(originalImage));
                    }
                }
                painter.end();
            }
            else
            {
                previewColorsAndGradientsMeow=false;
                if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome){
                    //both dimensions are bigger than the screen's. Center both dimensions.
                    QImage originalImage = image;

                    image = QImage(image.copy(0, 0, vScreenWidth, vScreenHeight));

                    image.fill(Qt::black);

                    QPainter painter(&image);
                    painter.drawPixmap((vScreenWidth-originalImage.width())/2, (vScreenHeight-originalImage.height())/2, QPixmap::fromImage(originalImage));
                    painter.end();
                }
                else
                {
                    //just start from the top left of the image and fill the rest.
                    image = QImage(image.copy(0, 0, vScreenWidth, vScreenHeight));
                }
            }
            break;
        }
        case Zoom:
        {
            //Zoom
            /*
             *  How it works:
             *  It calculates the max of vScreenWidth/imageWidth and vScreenHeight/imageHeight.
             *  It stretches the corresponding dimension to fit the screen.
             *  The other dimension is centered.
             */
            QImage originalImage = image;

            image = QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);

            image.fill(Qt::transparent);

            QPainter painter(&image);

            float widthFactor = (float) vScreenWidth  / originalImage.width();
            float heightFactor = (float) vScreenHeight / originalImage.height();

            int newWidth, newHeight;

            if(widthFactor>heightFactor){
                //it expands via the width, so center the height
                newWidth  = floor (originalImage.width() * widthFactor + 0.5);
                newHeight = floor (originalImage.height() * widthFactor + 0.5);
                painter.drawPixmap(0, (vScreenHeight-newHeight)/2, newWidth, newHeight, QPixmap::fromImage(originalImage));
            }
            else
            {
                //it expands via the height, so center the width
                newWidth  = floor (originalImage.width() * heightFactor + 0.5);
                newHeight = floor (originalImage.height() * heightFactor + 0.5);
                painter.drawPixmap((vScreenWidth-newWidth)/2, 0, newWidth, newHeight, QPixmap::fromImage(originalImage));
            }

            painter.end();
            break;

        }
        case Center:
        {
            //Center
            /*
             *  How it works:
             *  If the image is smaller than the screen's width then just center it. Everything else is black.
             *  If one or more dimensions of the image are larger than the screen's just crop the image to the
             *  screen's dimension and center it.
             */
            QImage originalImage = image;

            image = QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);

            image.fill(Qt::transparent);

            QPainter painter(&image);
            painter.drawPixmap((vScreenWidth-originalImage.width())/2, (vScreenHeight-originalImage.height())/2, QPixmap::fromImage(originalImage));
            painter.end();
            break;
        }
        case Scale:
        {
            //Scale - same as Span.
            /*
             *  How it works:
             *  It calculates the min of vScreenWidth/imageWidth and vScreenHeight/imageHeight.
             *  It stretches the corresponding dimension to fit the screen.
             *  The other dimension is centered.
             */
            QImage originalImage = image;

            image = QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);


            image.fill(Qt::transparent);

            QPainter painter(&image);

            float width_factor = (float) vScreenWidth  / originalImage.width();
            float height_factor = (float) vScreenHeight / originalImage.height();

            int new_width, new_height;


            if(width_factor<height_factor){
                //it expands via the width, so center the height
                new_width  = floor (originalImage.width() * width_factor + 0.5);
                new_height = floor (originalImage.height() * width_factor + 0.5);
                painter.drawPixmap(0, (vScreenHeight-new_height)/2, new_width, new_height, QPixmap::fromImage(originalImage));
            }
            else
            {
                //it expands via the height, so center the width
                new_width  = floor (originalImage.width() * height_factor + 0.5);
                new_height = floor (originalImage.height() * height_factor + 0.5);
                painter.drawPixmap((vScreenWidth-new_width)/2, 0, new_width, new_height, QPixmap::fromImage(originalImage));
            }

            painter.end();
            break;
        }
        case NoneStyle:
        case Stretch:
            //Fill (or "stretched"). This is the default for the QLabel.
        default:
            break;
        }
        if(desktopStyle!=NoneStyle){
            if(image.width() > 2*SCREEN_LABEL_SIZE_X || image.height() > 2*SCREEN_LABEL_SIZE_Y){
                image = QImage(image.scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation).scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            else
            {
                image = QImage(image.scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
        }

        if(previewColorsAndGradientsMeow || desktopStyle==NoneStyle){
            colorsGradientsImage=colorsGradientsImage.scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation);
            QPainter painter;
            painter.begin(&colorsGradientsImage);
            painter.drawPixmap(0, 0, QPixmap::fromImage(image));
            painter.end();
            image = QImage(colorsGradientsImage);
        }
#else
        image = QImage(image.scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation).scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
#endif //#ifdef Q_OS_UNIX
        ui->screen_label->setPixmap(QPixmap::fromImage(image));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")){
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    switch(ui->stackedWidget->currentIndex()){
    case 0:
    {
        //something was dropped inside the Wallpapers page!
        QList<QUrl> urlList = event->mimeData()->urls();
        for (QList<QUrl>::const_iterator i = urlList.begin(); i != urlList.end();i++)
        {
            //removing extra characters from the absolute path...
            QString dropped_temp=(*i).toString();
            QString droppedfile = dropped_temp.mid(7, dropped_temp.length()-2);
            addFolderForMonitor(droppedfile);
        }
        break;
    }
    case 3:
    {
        QList<QUrl> urlList = event->mimeData()->urls();
        for (QList<QUrl>::const_iterator i = urlList.begin(); i != urlList.end();i++)
        {
            QString dropped_temp=(*i).toString();
            QString droppedfile = dropped_temp.mid(7, dropped_temp.length()-2);
            if(droppedfile.endsWith(".wcz")){
                //wallpaper clock! Install it!
                installWallpaperClock(droppedfile);
            }
        }
        break;
    }
    default:
        event->acceptProposedAction();
        return;
    }

    event->acceptProposedAction();
}

void MainWindow::animateProgressbarOpacity(bool show){
    if(hideProgress_->isActive()){
        hideProgress_->stop();
    }

    opacityEffect_->setOpacity(!show);

    if(show)
    {
        ui->timeForNext->show();
    }
    else
    {
        hideProgress_->start(400);
    }

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityProgressbarEnabled){
        globalParser_->setUnityProgressBarEnabled(show);
    }
#endif //#ifdef Q_OS_UNIX

    ui->timeForNext->setGraphicsEffect(opacityEffect_);
    QPropertyAnimation* anim = new QPropertyAnimation(this);
    anim->setTargetObject(opacityEffect_);
    anim->setPropertyName("opacity");
    anim->setDuration(400);
    anim->setStartValue(opacityEffect_->opacity());
    anim->setEndValue(show);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::hideTimeForNext()
{
    ui->timeForNext->hide();
    ui->timeForNext->setFormat(tr("Calculating..."));
    setProgressbarsValue(100);
}

void MainWindow::findSeconds(bool typeCountSeconds)
{
    int count_seconds;
    if(gv.typeOfInterval==0)
    {
        count_seconds=gv.defaultIntervals.at(ui->timerSlider->value()-1);
    }
    else
    {
        count_seconds=ui->days_spinBox->value()*86400+ui->hours_spinBox->value()*3600+ui->minutes_spinBox->value()*60+ui->seconds_spinBox->value();
    }

    if(typeCountSeconds)
    {
        //true,its time to change wallpaper and we want
        //to change the seconds_left to the current value of slider and to temp save the current value of slider.
        secondsLeft_=totalSeconds_=count_seconds;
    }
    else
    {
        //false, we just changed the value of the slider and we want the value to be converted
        //in seconds BUT  we don't want seconds_left or tmp_slider to be changed.
        secondsInWallpapersSlider_=count_seconds;
    }
}

void MainWindow::processRequestStart(){
    if(processingOnlineRequest_){
        return;
    }

    processingOnlineRequest_=true;
    ui->process_request_label->show();
    processingRequestGif_->start();
    if(gv.previewImagesOnScreen){
        ui->screen_label_info->setText(tr("Processing Request")+"...");
    }
}

void MainWindow::processRequestStop(){
    if(!processingOnlineRequest_){
        return;
    }
    processingOnlineRequest_=false;
    processingRequestGif_->stop();
    ui->process_request_label->hide();
    if(gv.previewImagesOnScreen){
        ui->screen_label_info->clear();
    }
    if(gv.potdRunning){
        QString filename=globalParser_->getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
        if(!filename.isEmpty() && ui->stackedWidget->currentIndex()==2 && QFile::exists(filename)){
            imageTransition(false, filename);
        }
    }
}

void MainWindow::setStyle(){
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::Mate){
        ui->image_style_combo->addItem(tr("Tile"));
        ui->image_style_combo->addItem(tr("Zoom"));
        ui->image_style_combo->addItem(tr("Center"));
        ui->image_style_combo->addItem(tr("Scale"));
        ui->image_style_combo->addItem(tr("Fill"));
        ui->image_style_combo->addItem(tr("Span"));

        QString style=globalParser_->gsettingsGet("org.gnome.desktop.background", "picture-options");
        short index=0;
        if (style=="zoom")
        {
            index=1;
        }
        else if (style=="centered"){
            index=2;
        }
        else if (style=="scaled")
        {
            index=3;
        }
        else if (style=="stretched")
        {
            index=4;
        }
        else if (style=="spanned")
        {
            index=5;
        }
        ui->image_style_combo->setCurrentIndex(index);
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        ui->image_style_combo->addItem(tr("None"));
        ui->image_style_combo->addItem(tr("Centered"));
        ui->image_style_combo->addItem(tr("Tiled"));
        ui->image_style_combo->addItem(tr("Stretched"));
        ui->image_style_combo->addItem(tr("Scaled"));
        ui->image_style_combo->addItem(tr("Zoomed"));
        int index=0;
        Q_FOREACH(QString entry, globalParser_->getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-style")){
                QString imageStyle=globalParser_->getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry);
                index=imageStyle.toInt();
            }
        }
        if(index<ui->image_style_combo->count() && index>=0){
            ui->image_style_combo->setCurrentIndex(index);
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE){
        ui->image_style_combo->addItem(tr("Empty"));
        ui->image_style_combo->addItem(tr("Fill"));
        ui->image_style_combo->addItem(tr("Fit"));
        ui->image_style_combo->addItem(tr("Center"));
        ui->image_style_combo->addItem(tr("Tile"));
        int value = globalParser_->getPcManFmValue("wallpaper_mode").toInt();
        if(value<ui->image_style_combo->count() && value>=0){
            ui->image_style_combo->setCurrentIndex(value);
        }
    }
#else
    ui->image_style_combo->addItem(tr("Tile"));
    ui->image_style_combo->addItem(tr("Center"));
    ui->image_style_combo->addItem(tr("Stretch"));

    if(settings->value("windows_major_version").toInt()==6 && settings->value("windows_minor_version").toInt()>= 1)
    {
        ui->image_style_combo->addItem(tr("Fit"));
        ui->image_style_combo->addItem(tr("Fill"));
    }
    QSettings desktop_settings("HKEY_CURRENT_USER\\Control Panel\\Desktop", QSettings::NativeFormat);

    switch(desktop_settings.value("WallpaperStyle").toInt())
    {
    default:
    case 0:
        if(desktop_settings.value("TileWallpaper")!=0){
            ui->image_style_combo->setCurrentIndex(0);
        }
        else{
            ui->image_style_combo->setCurrentIndex(1);
        }
        break;
    case 2:
        ui->image_style_combo->setCurrentIndex(2);
        break;
    case 6:
        ui->image_style_combo->setCurrentIndex(3);
        break;
    case 10:
        ui->image_style_combo->setCurrentIndex(4);
        break;
    }

#endif //#ifdef Q_OS_UNIX

    gv.mainwindowLoaded=true;
    updateScreenLabel();
}

#ifdef Q_OS_UNIX
DesktopStyle MainWindow::getDesktopStyle(){
    DesktopStyle desktopStyle=Stretch;
    if(gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::Mate){
        switch(ui->image_style_combo->currentIndex()){
        default:
        case 0:
            desktopStyle=Tile;
            break;
        case 1:
            desktopStyle=Zoom;
            break;
        case 2:
            desktopStyle=Center;
            break;
        case 5:
        case 3:
            desktopStyle=Scale;
            break;
        case 4:
            desktopStyle=Stretch;
            break;
        }
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        switch(ui->image_style_combo->currentIndex()){
        case 0:
            desktopStyle=NoneStyle;
            break;
        case 1:
            desktopStyle=Center;
            break;
        case 2:
            desktopStyle=Tile;
            break;
        default:
        case 3:
            desktopStyle=Stretch;
            break;
        case 4:
            desktopStyle=Scale;
            break;
        case 5:
            desktopStyle=Zoom;
            break;
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE){
        switch(ui->image_style_combo->currentIndex()){
        case 0:
            desktopStyle=NoneStyle;
            break;
        default:
        case 1:
            desktopStyle=Stretch;
            break;
        case 2:
            desktopStyle=Scale;
            break;
        case 3:
            desktopStyle=Center;
            break;
        case 4:
            desktopStyle=Tile;
            break;
        }
    }
    return desktopStyle;
}
#endif

void MainWindow::setButtonColor()
{
    QString colorName = globalParser_->getPrimaryColor();
    setButtonColor(colorName);
}

void MainWindow::setButtonColor(const QString &colorName){
#ifdef Q_OS_WIN
    QImage image(40, 19, QImage::Format_ARGB32_Premultiplied);
    image.fill(colorName);
    ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image)));
#else
    ColoringType::Value coloringType=globalParser_->getColoringType();
    if(coloringType == ColoringType::SolidColor){
        QImage image(40, 19, QImage::Format_ARGB32_Premultiplied);
        image.fill(colorName);
        ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image)));
    }
    else
    {
        QString secondaryColor=globalParser_->getSecondaryColor();

        QImage image(75, 75, QImage::Format_RGB32);
        image.fill(Qt::white);
        QColor color(colorName);
        image.fill(color);
        color.setNamedColor(secondaryColor);
        short alpha=255;
        short softness=0;
        QPainter painter;
        painter.begin(&image);
        for(short i=75;i>0;i--){
            if(!softness){
                alpha-=1;
                if(alpha<=244){
                    softness=1;
                }
            }
            else if(softness==1){
                alpha-=3;
                if(alpha<=205){
                    softness=2;
                }
            }
            else if(softness==2){
                alpha-=4;
            }

            color.setAlpha(alpha);
            painter.fillRect(i-1, 0, 1, 75, QColor(color));
        }
        painter.end();
        if(coloringType == ColoringType::VerticalColor)
        {
            image = image.transformed(QTransform().rotate(+90), Qt::SmoothTransformation);
        }
        updateColorButton(image);
    }
#endif
}

void MainWindow::on_image_style_combo_currentIndexChanged(int index)
{
    if(!gv.mainwindowLoaded){
        return;
    }

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::Gnome || gv.currentDE == DesktopEnvironment::UnityGnome || gv.currentDE == DesktopEnvironment::Mate){
        QString type;
        switch(index){
        case 0:
            type="wallpaper";
            break;
        default:
        case 1:
            type="zoom";
            break;
        case 2:
            type="centered";
            break;
        case 3:
            type="scaled";
            break;
        case 4:
            type="stretched";
            break;
        case 5:
            type="spanned";
            break;
        }
        globalParser_->gsettingsSet("org.gnome.desktop.background", "picture-options", type);
    }
    else if(gv.currentDE == DesktopEnvironment::XFCE){
        Q_FOREACH(QString entry, globalParser_->getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-style")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << QString::number(index));
            }
        }
    }
    else if(gv.currentDE == DesktopEnvironment::LXDE){
        QString style;
        switch(index){
        default:
        case 0:
            style="color";
            break;
        case 1:
            style="stretch";
            break;
        case 2:
            style="fit";
            break;
        case 3:
            style="center";
            break;
        case 4:
            style="tile";
            break;
        }

        QProcess::startDetached("pcmanfm", QStringList() << "--wallpaper-mode="+style);
    }

#else
    /*
     * ----------Reference: http://msdn.microsoft.com/en-us/library/bb773190(VS.85).aspx#desktop------------
     * Two registry values are set in the Control Panel\Desktop key.
     * TileWallpaper
     * 0: The wallpaper picture should not be tiled
     * 1: The wallpaper picture should be tiled
     * WallpaperStyle
     * 0:  The image is centered if TileWallpaper=0 or tiled if TileWallpaper=1
     * 2:  The image is stretched to fill the screen
     * 6:  The image is resized to fit the screen while maintaining the aspect
     *     ratio. (Windows 7 and later)
     * 10: The image is resized and cropped to fill the screen while
     *     maintaining the aspect ratio. (Windows 7 and later)
     * -----------------------------------------------------------------------------------------------------
     */

    /*
     * Unfortunately QSettings can't manage to change a registry DWORD,
     * so we have to go with the Windows' way.
     */

    HRESULT hr = S_OK;

    HKEY hKey = NULL;
    hr = HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_CURRENT_USER,
                                         L"Control Panel\\Desktop", 0, KEY_READ | KEY_WRITE, &hKey));
    if (SUCCEEDED(hr))
    {
        PWSTR pszWallpaperStyle;
        PWSTR pszTileWallpaper;

        wchar_t zero[10] = L"0";
        wchar_t one[10] = L"1";
        wchar_t two[10] = L"2";
        wchar_t six[10] = L"6";
        wchar_t ten[10] = L"10";

        switch (index)
        {
        case 0: //tile
            pszWallpaperStyle = zero;
            pszTileWallpaper = one;
            break;

        case 1: //center
            pszWallpaperStyle = zero;
            pszTileWallpaper = zero;
            break;

        case 2: //stretch
            pszWallpaperStyle = two;
            pszTileWallpaper = zero;
            break;

        case 3: //fit (Windows 7 and later)
            pszWallpaperStyle = six;
            pszTileWallpaper = zero;
            break;

        case 4: //fill (Windows 7 and later)
            pszWallpaperStyle = ten;
            pszTileWallpaper = zero;
            break;
        }

        //set the WallpaperStyle and TileWallpaper registry values.
        DWORD cbData = lstrlen(pszWallpaperStyle) * sizeof(*pszWallpaperStyle);
        hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"WallpaperStyle", 0, REG_SZ,
                                              reinterpret_cast<const BYTE *>(pszWallpaperStyle), cbData));
        if (SUCCEEDED(hr))
        {
            cbData = lstrlen(pszTileWallpaper) * sizeof(*pszTileWallpaper);
            hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"TileWallpaper", 0, REG_SZ,
                                                  reinterpret_cast<const BYTE *>(pszTileWallpaper), cbData));
        }

        RegCloseKey(hKey);
    }

    //set the desktop background to the already current background image now with the style changed!
#ifdef UNICODE
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID) WallpaperManager::currentBackgroundWallpaper().toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#else
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID) WallpaperManager::currentBackgroundWallpaper().toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#endif

#endif //#ifdef Q_OS_UNIX

    if(ui->screen_label_text->text().isEmpty()){
        updateScreenLabel();
    }
}

void MainWindow::actionsOnWallpaperChange(){
    if(gv.wallpapersRunning)
    {
        if(!currentFolderExists()){
            return;
        }
        if(gv.typeOfInterval==2)
        {
            //srand(time(0));
            secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
            initialRandomSeconds_=secondsLeft_;
            totalSeconds_=secondsLeft_;
        }
        else
        {
            findSeconds(true);
        }
        if(startWasJustClicked_){
            startWasJustClicked_=false;
            if(!gv.firstTimeout){
                changeImage();
            }
        }
        else
        {
            if(gv.independentIntervalEnabled){
                globalParser_->saveSecondsLeftNow(-1, 0);
            }
            changeImage();
        }
        globalParser_->saveSecondsLeftNow(secondsLeft_, 0);
    }
    else if(gv.wallpaperClocksRunning){
        calculateSecondsLeftForWallClocks();
        totalSeconds_=secondsLeft_;
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                                ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }
    }
    else if(gv.liveWebsiteRunning){
        ui->timeout_text_label->show();
        ui->website_timeout_label->show();
        processRequestStart();
        websiteSnapshot_->start();
        secondsLeft_=globalParser_->websiteSliderValueToSeconds(ui->website_slider->value());
        globalParser_->saveSecondsLeftNow(secondsLeft_, 2);
    }
    else if(gv.liveEarthRunning){
        processRequestStart();
        imageFetcher_->setFetchType(FetchType::LE);
        imageFetcher_->fetch();
        secondsLeft_=1800;
        globalParser_->saveSecondsLeftNow(secondsLeft_, 1);
    }
    else if(gv.potdRunning){
        justUpdatedPotd_=true;
        //time has come
        QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
        QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
            //the previous time that photoofday changed was another day or the settings have changed
            processRequestStart();
            imageFetcher_->setFetchType(FetchType::POTD);
            imageFetcher_->fetch();
        }
        else
        {
            /*
             * The previous time was today, so the picture must be still there,
             * so re-set it as background, no need to download anything!
             */
            QString filename = globalParser_->getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
            globalParser_->debug("Picture of the day has already been downloaded.");
            if(filename.isEmpty()){
                globalParser_->error("Probably you set the picture of the day today but you removed the original\n"\
                              "downloaded photo. If it didn't happen like this, please report this as a bug!\n"\
                              "Wallch will now attemp to download again the Picture Of The Day.");
                processRequestStart();
                imageFetcher_->setFetchType(FetchType::POTD);
                imageFetcher_->fetch();
            }
            wallpaperManager_->setBackground(filename, true, true, 3);
            if(gv.setAverageColor){
                setButtonColor();
            }
        }
    }
}

void MainWindow::updateSeconds(){
    /*
     * This function updates the seconds of the progressbar of Wallch and launches
     * the corresponding action when the seconds have passed!
     */

#ifdef Q_OS_UNIX
    if(gv.pauseOnBattery){
        if(globalParser_->runsOnBattery()){
            if(!manuallyStartedOnBattery_){
                bool array[5] = {gv.wallpapersRunning, gv.liveEarthRunning, gv.potdRunning, gv.wallpaperClocksRunning, gv.liveWebsiteRunning};

                for(int i=0;i<5;i++){
                    if(array[i]){
                        previouslyRunningFeature_=i;
                        break;
                    }
                }

                pauseEverythingThatsRunning();
                batteryStatusChecker_->start(2000);
                return;
            }
            //else on battery but manually started
        }
        else
        {
            //not on battery, so if it was on battery and manually started, reset the corresponding variable
            if(manuallyStartedOnBattery_){
                manuallyStartedOnBattery_=false;
            }
        }
    }
#endif

    if(gv.potdRunning){
        if(globalParser_->timeNowToString()=="00:00"){
            if(!justUpdatedPotd_){
                actionsOnWallpaperChange();
            }
        }
        else
        {
            justUpdatedPotd_=false;
        }
        updatePotdProgress();
    }
    else
    {
        gv.runningTimeOfProcess = QDateTime::currentDateTime();
        if(secondsLeft_<=0){
            actionsOnWallpaperChange();
            gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsLeft_);
        }

        //in case the computer went to hibernate/suspend take actions
        int secsToFinishInterval=gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
        if(secondsLeft_<(secsToFinishInterval-1) || secondsLeft_>(secsToFinishInterval+1))
        {
            int secondsToChange = gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
            if(secondsToChange<=0)
            {
                //the wallpaper should've already changed!
                actionsOnWallpaperChange();
                gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsLeft_);
            }
            else if (!(gv.wallpaperClocksRunning&& (secondsLeft_-secondsToChange<-1 || secondsLeft_-secondsToChange>1))){
                //the time has yet to come, just update
                if(abs(secondsLeft_-secondsToChange)>1){
                    secondsLeft_=secondsToChange;
                }
            }
        }

        ui->timeForNext->setFormat(secondsToHms(secondsLeft_));

        if(gv.wallpapersRunning)
        {
            if(gv.typeOfInterval==2){
                if(initialRandomSeconds_==0){
                    setProgressbarsValue(100);
                }
                else
                {
                    setProgressbarsValue((secondsLeft_*100)/initialRandomSeconds_);
                }
            }
            else{
                if(totalSeconds_==0){
                    setProgressbarsValue(100);
                }
                else
                {
                    setProgressbarsValue((secondsLeft_*100)/(totalSeconds_));
                }
            }
        }
        else if(gv.wallpaperClocksRunning){
            if(totalSeconds_==0){
                setProgressbarsValue(100);
            }
            else
            {
                setProgressbarsValue((secondsLeft_*100)/(totalSeconds_));
            }
        }
        else if(gv.liveWebsiteRunning){
            int totalSecs=globalParser_->websiteSliderValueToSeconds(ui->website_slider->value());
            if(totalSecs==0){
                setProgressbarsValue(100);
            }
            else
            {
                setProgressbarsValue((secondsLeft_*100)/totalSecs);
            }
            if(processingOnlineRequest_ && timePassedForLiveWebsiteRequest_<=WEBSITE_TIMEOUT){
                ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT+1-(timePassedForLiveWebsiteRequest_++))+" "+tr("seconds")+"...");
            }
        }
        else if(gv.liveEarthRunning){
            setProgressbarsValue(secondsLeft_*100/1800);
        }

        secondsLeft_--;
    }
}

void MainWindow::nextKeySignal(const char *, void *){
    mainWindowInstance->click_shortcut_next();
}

void MainWindow::click_shortcut_next(){
    on_next_Button_clicked();
}

void MainWindow::on_website_slider_valueChanged(int value)
{
    ui->website_interval_slider_label->setText(secondsToMh(globalParser_->websiteSliderValueToSeconds(value)));
}

void MainWindow::changeAppStyleSheetTo(const QString &styleSheetFile){
    QFile file(styleSheetFile);
    if(!file.open(QIODevice::ReadOnly)){
        Global::error("Could not load the current theme file.");
        return;
    }

    qApp->setStyleSheet(QString::fromLatin1(file.readAll()));
    file.close();
}

void MainWindow::setThemeToAmbiance(){
    changeAppStyleSheetTo(":/themes/ambiance.qss");
    Q_FOREACH(QLabel *separator, menuSeparators_){
        separator->show();
        separator->setPixmap(AMBIANCE_SEPARATOR);
    }
}

void MainWindow::setThemeToRadiance(){

    changeAppStyleSheetTo(":/themes/radiance.qss");
    Q_FOREACH(QLabel *separator, menuSeparators_){
        separator->show();
        separator->setPixmap(RADIANCE_SEPARATOR);
    }

}

void MainWindow::stopEverythingThatsRunning(short excludingFeature)
{
    if(excludingFeature!=1 && gv.wallpapersRunning){
        on_stopButton_clicked();
    }
    else if(excludingFeature!=2 && gv.liveEarthRunning){
        on_deactivate_livearth_clicked();
    }
    else if(excludingFeature!=3 && gv.potdRunning){
        on_deactivate_potd_clicked();
    }
    else if(excludingFeature!=4 && gv.wallpaperClocksRunning){
        on_deactivate_clock_clicked();
    }
    else if(excludingFeature!=5 && gv.liveWebsiteRunning){
        on_deactivate_website_clicked();
    }
}

void MainWindow::pauseEverythingThatsRunning()
{
    if(gv.wallpapersRunning){
        if(!gv.processPaused)
        {
            on_startButton_clicked();
        }
    }
    else if(gv.liveEarthRunning){
        on_deactivate_livearth_clicked();
    }
    else if(gv.potdRunning){
        on_deactivate_potd_clicked();
    }
    else if(gv.wallpaperClocksRunning){
        on_deactivate_clock_clicked();
    }
    else if(gv.liveWebsiteRunning){
        on_deactivate_website_clicked();
    }
}

QString MainWindow::secondsToHms(int seconds){
    int minutes_left=0, hours_left=0, finalSeconds;
    if(seconds>=60){
        minutes_left=seconds/60;
        if(minutes_left>=60){
            hours_left=minutes_left/60;
        }
        minutes_left=minutes_left-hours_left*60;
        finalSeconds=seconds-(hours_left*3600+minutes_left*60);
    }
    else{
        finalSeconds=seconds;
    }
    if(!hours_left && !minutes_left){
        return QString::number(finalSeconds)+tr("s");
    }
    else if(!hours_left){
        if(finalSeconds){
            return QString::number(minutes_left)+tr("m")+" " + QString::number(finalSeconds)+tr("s");
        }
        else{
            return QString::number(minutes_left)+tr("m");
        }
    }
    else if(!minutes_left){
        if(finalSeconds){
            return QString::number(hours_left) + tr("h")+" " + QString::number(finalSeconds)+tr("s");
        }
        else{
            return QString::number(hours_left) + tr("h");
        }
    }
    else
    {
        if(finalSeconds){
            return QString::number(hours_left) + tr("h")+" " + QString::number(minutes_left)+tr("m")+" " + QString::number(finalSeconds)+tr("s");
        }
        else{
            return QString::number(hours_left) + tr("h")+" " + QString::number(minutes_left)+tr("m");
        }
    }
}

QString MainWindow::secondsToHm(int seconds){
    if(seconds<=60){
        return QString("1"+tr("m"));
    }
    else if(seconds<=3600){
        return QString::number(seconds/60)+QString(tr("m"));
    }
    else
    {
        int hours=seconds/3600;
        seconds-=hours*3600;
        int minutes=seconds/60;
        if(minutes){
            return QString::number(hours)+QString(tr("h")+" ")+QString::number(minutes)+QString(tr("m"));
        }
        else{
            return QString::number(hours)+QString(tr("h"));
        }
    }
}

QString MainWindow::secondsToMh(int seconds)
{
    if (seconds<60){
        return QString(QString::number(seconds) + " "+tr("seconds"));
    }
    else if(seconds==60){
        return QString("1 "+tr("minute"));
    }
    else if(seconds<3600){
        return QString(QString::number(seconds/60) + " "+tr("minutes"));
    }
    else if(seconds==3600){
        return QString("1 "+tr("hour"));
    }
    else if(seconds<86400){
        return QString(QString::number(seconds/3600) + " "+tr("hours"));
    }
    else if(seconds==86400){
        return QString("1 "+tr("day"));
    }
    else if(seconds==604800){
        return QString("1 "+tr("week"));
    }
    return QString("");
}

void MainWindow::setAverageColor(const QString &image){
    //sets the desktop background color, updates mainwindow's primary color box
    setButtonColor(globalParser_->setAverageColor(image));
}

QString MainWindow::fixBasenameSize(const QString &basename){
    if(basename.length()>35){
        return basename.left(32)+"...";
    }
    return basename;
}

void MainWindow::updateScreenLabel()
{
    ui->screen_label_info->clear();
    ui->screen_label_text->clear();

    switch(ui->stackedWidget->currentIndex()){
    case 0:

        if(ui->wallpapersList->count()==0 || ui->wallpapersList->selectedItems().count()==0)
        {
            changeTextOfScreenLabelTo(tr("Select an image to preview"));
        }
        else
        {
            on_wallpapersList_itemSelectionChanged();
        }

        break;

    case 1:
    {

        QString filename=globalParser_->getFilename(gv.wallchHomePath+"liveEarth*");
        if(!filename.isEmpty()){
            imageTransition(false, filename);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }

        break;

    }
    case 2:
    {

        QString filename=globalParser_->getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
        if(!filename.isEmpty()){
            imageTransition(false, filename);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }

        break;

    }
    case 3:

        if(ui->clocksTableWidget->rowCount()==0)
        {
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }
        else if(ui->clocksTableWidget->rowCount()!=0 && ui->clocksTableWidget->selectedItems().count()==0)
        {
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }
        else if(ui->clocksTableWidget->rowCount()!=0 && ui->clocksTableWidget->selectedItems().count()!=0)
        {
            imageTransition(true);
        }

        break;

    case 4:

        if(QFile::exists(gv.wallchHomePath+LW_PREVIEW_IMAGE)){
            imageTransition(false, gv.wallchHomePath+LW_PREVIEW_IMAGE);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }

        break;

    case 5:

        changeTextOfScreenLabelTo("Mellori Studio");

        break;

    }
}

void MainWindow::changeTextOfScreenLabelTo(const QString &text)
{
    if(!gv.previewImagesOnScreen){
        return;
    }

    hideScreenLabel();

    opacityEffect_->setOpacity(false);
    ui->screen_label_text->setGraphicsEffect(opacityEffect_);
    QPropertyAnimation* anim = new QPropertyAnimation(this);
    anim->setTargetObject(opacityEffect_);
    anim->setPropertyName("opacity");
    anim->setDuration(IMAGE_TRANSITION_SPEED+50);
    anim->setStartValue(opacityEffect_->opacity());
    anim->setEndValue(true);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    ui->screen_label_text->setText(text);
}

#ifdef Q_OS_UNIX
void MainWindow::unityProgressbarSetEnabled(bool enabled){
    if(gv.wallpapersRunning || gv.liveEarthRunning || gv.potdRunning || gv.wallpaperClocksRunning || gv.liveWebsiteRunning ){
        globalParser_->setUnityProgressBarEnabled(enabled);
        if(enabled){
            globalParser_->setUnityProgressbarValue(0);
        }
    }
}

#endif //#ifdef Q_OS_UNIX

void MainWindow::updateColorButton(QImage image)
{
    ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image.scaled(40, 19, Qt::IgnoreAspectRatio, Qt::FastTransformation))));
}

//Wallpapers code

void MainWindow::justChangeWallpaper(){
    if(!loadedPages_[0]){
        loadWallpapersPage();
        if(ui->wallpapersList->count()<LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
    }

    wallpaperManager_->setRandomWallpaperAsBackground();
}

void MainWindow::on_startButton_clicked(){
    if(!loadedPages_[0]){
        loadWallpapersPage();
        if(wallpaperManager_->wallpapersCount()<LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
    }

    if (!ui->startButton->isEnabled()){
        globalParser_->desktopNotify(tr("Not enough pictures to start chaning wallpapers."), false, "info");
        globalParser_->error("Too few images for wallpaper feature to start. Make sure there are at least 2 pictures at the selected folder.");
        return;
    }

    stopEverythingThatsRunning(1);
    startPauseWallpaperChangingProcess();
}

void MainWindow::startPauseWallpaperChangingProcess(){
    if(!currentFolderExists()){
        return;
    }
    if (actAsStart_){

        actAsStart_=false; //the next time act like pause is pressed
        gv.wallpapersRunning=true;
        globalParser_->updateStartup();
        startWasJustClicked_=true;

        ui->startButton->setText(tr("Pau&se"));
        stopButtonsSetEnabled(true);
        previousAndNextButtonsSetEnabled(true);
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-pause", QIcon(":/images/media-playback-pause.png")));

        QApplication::processEvents(QEventLoop::AllEvents);

        shuffleWasChecked_=ui->shuffle_images_checkbox->isChecked();

        if(!gv.processPaused){
            if(shuffleWasChecked_){
                if(firstRandomImageIsntRandom_){
                    firstRandomImageIsntRandom_=false;
                    if(ui->wallpapersList->currentItem()->isSelected()){
                        wallpaperManager_->setRandomMode(true, -1, ui->wallpapersList->currentRow());
                    }
                }
                else
                {
                    wallpaperManager_->setRandomMode(true);
                }
            }
            else
            {
                wallpaperManager_->setRandomMode(false);
            }
        }

        if(gv.processPaused){
            //If the process was paused, then we need to continue from where it is left, not from the next second
            secondsLeft_+=1;
        }

        globalParser_->resetSleepProtection(secondsLeft_);

        if(!gv.processPaused){
            //if the process wasn't paused, then the progressbar is hidden. Add an animation so as to show it
            animateProgressbarOpacity(1);
            if(gv.typeOfInterval==2)
            {
                srand(time(0));
                secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
                initialRandomSeconds_=secondsLeft_;
                totalSeconds_=secondsLeft_;
            }
        }
        gv.processPaused=false;

#ifdef Q_OS_UNIX
        if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityProgressbarEnabled){
            dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
            globalParser_->setUnityProgressBarEnabled(true);
        }
#endif //#ifdef Q_OS_UNIX
        Q_EMIT signalRecreateTray();

        if(gv.pauseOnBattery){
            if(globalParser_->runsOnBattery()){
                //manually started
#ifdef Q_OS_UNIX
                if(batteryStatusChecker_->isActive()){
                    batteryStatusChecker_->stop();
                }
#endif
                manuallyStartedOnBattery_=true;
            }
            else
            {
                manuallyStartedOnBattery_=false;
            }
        }

        startUpdateSeconds();
        on_page_0_wallpapers_clicked();
    }
    else
    {
        actAsStart_=true; //<- the next time act like start is pressed
        gv.processPaused=true;
        stoppedBecauseOnBattery_=false;
        ui->startButton->setText(tr("&Start"));
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
#ifdef Q_OS_UNIX
        if(gv.currentDE == DesktopEnvironment::UnityGnome){
            dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
        }
#endif //#ifdef Q_OS_UNIX
        Q_EMIT signalRecreateTray();
        updateSecondsTimer_->stop();
        ui->timeForNext->setFormat(ui->timeForNext->format()+" - Paused.");
        previousAndNextButtonsSetEnabled(false);
        if(ui->wallpapersList->count() != 0){
            ui->shuffle_images_checkbox->setEnabled(true);
        }
        if(gv.independentIntervalEnabled){
            globalParser_->saveSecondsLeftNow(secondsLeft_, 0);
        }
    }
}

void MainWindow::on_stopButton_clicked(){
    if (!ui->stopButton->isEnabled()){
        return;
    }
    if(ui->wallpapersList->count()!=0){
        ui->shuffle_images_checkbox->setEnabled(true);
    }
    stoppedBecauseOnBattery_=false;
    wallpaperManager_->startOver();
    actAsStart_=true;
    ui->startButton->setText(tr("&Start"));
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
    animateProgressbarOpacity(0);
    gv.processPaused=false;
    firstRandomImageIsntRandom_=false;
    secondsLeft_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    previousAndNextButtonsSetEnabled(false);

    startButtonsSetEnabled(ui->wallpapersList->count() >= LEAST_WALLPAPERS_FOR_START);

    stopButtonsSetEnabled(false);
    gv.wallpapersRunning=false;
    globalParser_->updateStartup();
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityProgressbarEnabled){
        globalParser_->setUnityProgressBarEnabled(false);
    }
    if(gv.pauseOnBattery){
        if(batteryStatusChecker_->isActive()){
            batteryStatusChecker_->stop();
        }
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalRecreateTray();
    if(gv.independentIntervalEnabled){
        globalParser_->saveSecondsLeftNow(-1, 0);
    }
}

void MainWindow::on_next_Button_clicked()
{
    if (!ui->next_Button->isEnabled() || !currentFolderExists()){
        return;
    }

    updateSecondsTimer_->stop();
    secondsLeft_=0;
    startUpdateSeconds();
}

void MainWindow::on_previous_Button_clicked()
{
    if(!ui->previous_Button->isEnabled() || !currentFolderExists()){
        return;
    }

    updateSecondsTimer_->stop();

    wallpaperManager_->setBackground(wallpaperManager_->getPreviousWallpaper(), true, true, 1);
    if(gv.setAverageColor){
        setButtonColor();
    }

    if(gv.rotateImages && gv.iconMode){
        forceUpdateIconOf(wallpaperManager_->currentWallpaperIndex()-2);
    }


    if(gv.typeOfInterval){
        srand(time(0));
        secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
        initialRandomSeconds_=secondsLeft_;
    }
    else
    {
        findSeconds(true);
    }
    globalParser_->resetSleepProtection(secondsLeft_);

    startUpdateSeconds();
}

bool MainWindow::currentFolderExists()
{
    if(currentFolderIsAList_){
        short folders_list_count=currentFolderList_.count();
        for(short i=0;i<folders_list_count;i++){
            if(QDir(currentFolderList_.at(i)).exists()){
                return true;
                break;
            }
        }
    }
    else if(QDir(currentFolder_).exists()){
        return true;
    }
    currentFolderDoesNotExist();
    return false;
}

void MainWindow::beginFixCacheForFolders(){
    QtConcurrent::run(cacheManager_, &CacheManager::fixCacheSizeWithCurrentFoldersBeing, getCurrentWallpaperFolders());
}

void MainWindow::currentFolderDoesNotExist()
{
    int index=ui->pictures_location_comboBox->currentIndex();
    ui->pictures_location_comboBox->setItemText(index, currentFolderIsAList_ ? ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()+" (0)":globalParser_->basenameOf(ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()+" (0)" ));
    ui->wallpapersList->clear();
    wallpaperManager_->clearWallpapers();
    if(gv.wallpapersRunning){
        on_stopButton_clicked();
    }

    if(currentFolderIsAList_)
    {
        QString listOfFolders;
        short foldersListCount = currentFolderList_.count();
        for(short i = 0;i < foldersListCount; i++){
            listOfFolders.append(currentFolderList_.at(i) + "\n");
        }
        if(QMessageBox::question(this, tr("None of the below folders exist\n"), listOfFolders+
                                 tr("They could be on a hard drive on this computer so check if that disk is properly inserted, or it could be on a network so check if you are connected to the Internet or your network, and then try again. Delete this set of folders from the list?"))==QMessageBox::Yes)
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
            ui->pictures_location_comboBox->removeItem(index);
            savePicturesLocations();
        }
        else
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
        }
    }
    else
    {
        if(QMessageBox::question(this, tr("Location is not available"), currentFolder_+" "+tr("referes to a location that is unavailable.")+" "+
                                 tr("It could be on a hard drive on this computer so check if that disk is properly inserted, or it could be on a network so check if you are connected to the Internet or your network, and then try again. Delete this folder from the list?"))==QMessageBox::Yes)
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
            ui->pictures_location_comboBox->removeItem(index);
            savePicturesLocations();
        }
        else
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
        }
    }
}

void MainWindow::on_wallpapersList_itemSelectionChanged()
{
    if(!gv.mainwindowLoaded || !gv.previewImagesOnScreen){
        return;
    }

    if(ui->wallpapersList->selectedItems().count()==1){

        QString filename = getPathOfListItem();

        if(!processingOnlineRequest_){
            ui->screen_label_info->setText(fixBasenameSize(globalParser_->basenameOf(filename)));
        }
        imageTransition(false, filename);
    }
}

void MainWindow::on_wallpapersList_itemDoubleClicked()
{
    if(!ui->wallpapersList->count()){
        return;
    }

    int curRow = ui->wallpapersList->currentRow();
    if(curRow < 0){
        return;
    }

    QString picture = getPathOfListItem(curRow);

    if(WallpaperManager::imageIsNull(picture)){
        return;
    }
    wallpaperManager_->addToPreviousWallpapers(picture);
    wallpaperManager_->setBackground(picture, true, true, 1);
    if(gv.setAverageColor){
        setButtonColor();
    }
    if(gv.rotateImages && gv.iconMode){
        forceUpdateIconOf(curRow);
    }
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::LXDE){
        DesktopStyle desktopStyle = getDesktopStyle();
        if(desktopStyle == NoneStyle){
            ui->image_style_combo->setCurrentIndex(2);
        }
    }
#endif
}

void MainWindow::startWithThisImage(){
    stopEverythingThatsRunning(0);
    if(ui->shuffle_images_checkbox->isChecked()){
        firstRandomImageIsntRandom_=true;
    }
    on_startButton_clicked();
}

void MainWindow::removeImageFromDisk(){
    if (QMessageBox::question(this, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the selected image?")) != QMessageBox::Yes)
    {
        return;
    }

    QString imageFilename = getPathOfListItem(ui->wallpapersList->currentRow());

    if(!QFile::remove(imageFilename)){
        QMessageBox::warning(this, tr("Error!"), tr("Image deletion failed possibly because you don't have the permissions to delete the image or the image doesn't exist"));
    }
    else
    {
        //remove the cache of the image, in case it exists
        cacheManager_->removeCacheOf(imageFilename);
    }

    if(ui->wallpapersList->count() <= LEAST_WALLPAPERS_FOR_START){
        startButtonsSetEnabled(false);
    }
}

void MainWindow::removeImagesFromDisk(){
    int selectedCount=ui->wallpapersList->selectedItems().count();
    if(QMessageBox::question(this, tr("Warning!"), tr("Image deletion.")+"<br><br>"+tr("This action is going to permanently delete")+" "+QString::number(selectedCount)+" "+tr("images. Are you sure?"))==QMessageBox::Yes){
        QStringList imagesThatFailedToDelete;
        for(int i=0;i<selectedCount;i++){
            QString currentImage;
            if(gv.iconMode){
                if(ui->wallpapersList->selectedItems().at(i)->statusTip().isEmpty()){
                    currentImage = ui->wallpapersList->selectedItems().at(i)->toolTip();
                }
                else
                {
                    currentImage = ui->wallpapersList->selectedItems().at(i)->statusTip();
                }
            }
            else
            {
                currentImage = ui->wallpapersList->selectedItems().at(i)->text();
            }
            if(!QFile::remove(currentImage)){
                imagesThatFailedToDelete << currentImage;
            }
            else
            {
                //remove the cache of the image, in case it exists
                cacheManager_->removeCacheOf(currentImage);
            }
        }
        if(imagesThatFailedToDelete.count() > 0){
            QMessageBox::warning(this, tr("Error!"), tr("There was a problem with the deletion of the following files:")+"\n"+imagesThatFailedToDelete.join("\n"));
        }
    }
}

void MainWindow::rotateRight(){
    if(!ui->wallpapersList->currentItem()->isSelected()){
        return;
    }
    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull()){
        return;
    }
    globalParser_->rotateImg(path, 6, false);
    if(gv.iconMode){
        forceUpdateIconOf(ui->wallpapersList->currentRow());
    }
    else if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }

    if(path==wallpaperManager_->currentBackgroundWallpaper()){
        wallpaperManager_->setBackground(path, false, false, 1);
    }
}

void MainWindow::rotateLeft(){
    if(!ui->wallpapersList->currentItem()->isSelected()){
        return;
    }
    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull()){
        return;
    }
    globalParser_->rotateImg(path, 8, false);
    if(gv.iconMode){
        forceUpdateIconOf(ui->wallpapersList->currentRow());
    }
    else if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }

    if(path==WallpaperManager::currentBackgroundWallpaper()){
        WallpaperManager::setBackground(path, false, false, 1);
    }
}

void MainWindow::copyImagePath(){
    QApplication::clipboard()->setText(getPathOfListItem());
}

void MainWindow::copyImage(){
    QApplication::clipboard()->setImage(QImage(getPathOfListItem()), QClipboard::Clipboard);
}

QString MainWindow::getPathOfListItem(int index /* = -1*/){
    /*
     * Returns the full path of the listwidget item at 'index'
     * Returns the full path of the selected item's path if index==-1
     */
    if(index==-1){
        index=ui->wallpapersList->currentRow();
    }
    if(gv.iconMode){
        if(ui->wallpapersList->item(index)->statusTip().isEmpty()){
            return ui->wallpapersList->item(index)->toolTip();
        }
        else
        {
            return ui->wallpapersList->item(index)->statusTip();
        }
    }
    else
    {
        return ui->wallpapersList->item(index)->text();
    }
}

void MainWindow::on_wallpapersList_customContextMenuRequested()
{
    if(!currentFolderExists())
        return;

    int selectedCount = ui->wallpapersList->selectedItems().count();

    if(selectedCount==0){
        return;
    }

    listwidgetMenu_ = new QMenu(this);

    if (selectedCount<2){

        QAction *enterAction = new QAction(tr("Set this item as Background"), listwidgetMenu_);
        enterAction->setShortcut(QKeySequence("Enter"));
        connect(enterAction, SIGNAL(triggered()), this, SLOT(on_wallpapersList_itemDoubleClicked()));
        listwidgetMenu_->addAction(enterAction);

        if(ui->wallpapersList->count()>2){
            listwidgetMenu_->addAction(tr("Start from this image"), this, SLOT(startWithThisImage()));
        }

        QAction *deleteAction = new QAction(tr("Delete image from disk"), listwidgetMenu_);
        deleteAction->setShortcut(QKeySequence::Delete);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(removeImageFromDisk()));
        listwidgetMenu_->addAction(deleteAction);

        listwidgetMenu_->addAction(tr("Rotate Right"), this, SLOT(rotateRight()));
        listwidgetMenu_->addAction(tr("Rotate Left"), this, SLOT(rotateLeft()));
        listwidgetMenu_->addAction(tr("Copy path to clipboard"), this, SLOT(copyImagePath()));
        listwidgetMenu_->addAction(tr("Copy image to clipboard"), this, SLOT(copyImage()));
        listwidgetMenu_->addAction(tr("Open folder"), this, SLOT(openImageFolder()));

        QAction *findAction = new QAction(tr("Find an image by name"), listwidgetMenu_);
        findAction->setShortcut(QKeySequence("Ctrl+F"));
        connect(findAction, SIGNAL(triggered()), this, SLOT(showHideSearchBoxMenu()));
        listwidgetMenu_->addAction(findAction);

        QAction *propertiesAction = new QAction(tr("Properties"), listwidgetMenu_);
        propertiesAction->setShortcut(QKeySequence("Alt+Enter"));
        connect(propertiesAction, SIGNAL(triggered()), this, SLOT(showProperties()));
        listwidgetMenu_->addAction(propertiesAction);
    }
    else
    {
        //more than one file has been selected, show shorter menu with massive options
        listwidgetMenu_->addAction(tr("Open containing folder of all images"), this, SLOT(openImageFolderMassive()));

        QAction *deleteAction = new QAction(tr("Delete images from disk"), listwidgetMenu_);
        deleteAction->setShortcut(QKeySequence::Delete);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(removeImagesFromDisk()));
        listwidgetMenu_->addAction(deleteAction);
    }

    listwidgetMenu_->popup(MENU_POPUP_POS);
}

void MainWindow::addFolderForMonitor(const QString &folder){
    if(QDir(folder).exists()){
        short count = ui->pictures_location_comboBox->count();
        for(short i=0; i<count; i++)
        {
            if(globalParser_->foldersAreSame(folder, ui->pictures_location_comboBox->itemData(i, Qt::UserRole).toString()))
            {
                ui->pictures_location_comboBox->setCurrentIndex(i);
                return;
            }
        }
        ui->pictures_location_comboBox->addItem(globalParser_->basenameOf(folder));
        ui->pictures_location_comboBox->setItemData(count, folder, Qt::UserRole);
        ui->pictures_location_comboBox->setCurrentIndex(count);
        savePicturesLocations();
    }
}

void MainWindow::changeImage(){
    //this function handles the change of the image of the wallpapers page.

    if(shuffleWasChecked_!=ui->shuffle_images_checkbox->isChecked()){
        shuffleWasChecked_=ui->shuffle_images_checkbox->isChecked();
        //the shuffle has changed state!
        if(ui->shuffle_images_checkbox->isChecked()){
            //from normal to random. The first random image must not be the current image
            wallpaperManager_->setRandomMode(true, wallpaperManager_->currentWallpaperIndex());
        }
        else
        {
            //from random it went to normal
            wallpaperManager_->convertRandomToNormal();
        }
    }

    QString image=wallpaperManager_->getNextWallpaper();

    wallpaperManager_->addToPreviousWallpapers(image);

    WallpaperManager::setBackground(image, true, true, 1);
    if(gv.setAverageColor){
        setButtonColor();
    }
    if(gv.rotateImages && gv.iconMode){
        forceUpdateIconOf(wallpaperManager_->currentWallpaperIndex());
    }
}

void MainWindow::searchFor(const QString &term){
    ui->wallpapersList->clearSelection();
    searchList_.clear();
    int listCount=ui->wallpapersList->count();
    if(gv.iconMode){
        for(int i=0;i<listCount;i++){
            if(ui->wallpapersList->item(i)->statusTip().isEmpty()){
                searchList_ << globalParser_->basenameOf(ui->wallpapersList->item(i)->toolTip());
            }
            else
            {
                searchList_ << globalParser_->basenameOf(ui->wallpapersList->item(i)->statusTip());
            }
        }
    }
    else
    {
        for(int i=0; i<listCount; i++){
            searchList_ << globalParser_->basenameOf(ui->wallpapersList->item(i)->text());
        }
    }
    match_->setPattern("*"+term+"*");
    currentSearchItemIndex = searchList_.indexOf(*match_, 0);
    if(currentSearchItemIndex < 0){
        doesntMatch();
    }
    else
    {
        ui->wallpapersList->scrollToItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->setCurrentItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->item(currentSearchItemIndex)->setSelected(true);
        doesMatch();
    }
}

void MainWindow::continueToNextMatch(){
    ui->wallpapersList->clearSelection();
    if(currentSearchItemIndex >= ui->wallpapersList->count()+1){
        //restart the search...
        currentSearchItemIndex=searchList_.indexOf(*match_, 0);
        if(currentSearchItemIndex<0){
            return;
        }
        ui->wallpapersList->scrollToItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->setCurrentItem(ui->wallpapersList->item(currentSearchItemIndex));
    }
    else
    {
        //continue the search..
        currentSearchItemIndex = searchList_.indexOf(*match_, currentSearchItemIndex+1);
        if(currentSearchItemIndex<0){
            //restart the search...
            currentSearchItemIndex=searchList_.indexOf(*match_, 0);
            if(currentSearchItemIndex<0){
                return;
            }
        }
        ui->wallpapersList->scrollToItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->setCurrentItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->item(currentSearchItemIndex)->setSelected(true);
    }
}

void MainWindow::continueToPreviousMatch(){
    ui->wallpapersList->clearSelection();
    if(currentSearchItemIndex < 0){
        //restart the search...
        currentSearchItemIndex=searchList_.lastIndexOf(*match_, searchList_.count()-1);
        if(currentSearchItemIndex<0){
            return;
        }
        ui->wallpapersList->scrollToItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->setCurrentItem(ui->wallpapersList->item(currentSearchItemIndex));
    }
    else
    {
        //continue the search..
        currentSearchItemIndex = searchList_.lastIndexOf(*match_, currentSearchItemIndex-1);
        if(currentSearchItemIndex < 0){
            //restart the search...
            currentSearchItemIndex=searchList_.lastIndexOf(*match_, searchList_.count()-1);
            if(currentSearchItemIndex<0){
                return;
            }
        }
        ui->wallpapersList->scrollToItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->setCurrentItem(ui->wallpapersList->item(currentSearchItemIndex));
        ui->wallpapersList->item(currentSearchItemIndex)->setSelected(true);
    }
}

void MainWindow::openImageFolder(){

    if(!ui->wallpapersList->currentItem()->isSelected()){
        return;
    }
    wallpaperManager_->openFolderOf(getPathOfListItem());
}

void MainWindow::openImageFolderMassive(){
    QStringList parentFolders;
    int selectedCount=ui->wallpapersList->selectedItems().count();
    short totalFolderCount=0;
    for(int i=0;i<selectedCount;i++){
        QString currentImage;
        if(gv.iconMode){
            if(ui->wallpapersList->selectedItems().at(i)->statusTip().isEmpty()){
                currentImage = ui->wallpapersList->selectedItems().at(i)->toolTip();
            }
            else{
                currentImage = ui->wallpapersList->selectedItems().at(i)->statusTip();
            }
        }
        else{
            currentImage = ui->wallpapersList->selectedItems().at(i)->text();
        }
        QString image_folder = globalParser_->dirnameOf(currentImage);
        if(!parentFolders.contains(image_folder)){
            parentFolders << image_folder;
            totalFolderCount++;
        }
    }
    if(totalFolderCount>5){
        if(QMessageBox::question(this, tr("Warning!"), tr("Too many folders to open.")+"<br><br>"+tr("This action is going to open")+" "+QString::number(totalFolderCount)+" "+tr("folders. Are you sure?"))!=QMessageBox::Yes){
            return;
        }
    }
    for(short i=0;i<totalFolderCount;i++){
        wallpaperManager_->openFolderOf(parentFolders.at(i));
    }
}

void MainWindow::on_timerSlider_valueChanged(int value)
{
    ui->wallpapers_slider_time->setText(globalParser_->secondsToMinutesHoursDays(gv.defaultIntervals.at(value-1)));

    if(!gv.mainwindowLoaded){
        return;
    }

    findSeconds(false);
    updateCheckTime_->start(3000);
    settings->setValue("delay", secondsInWallpapersSlider_);
    settings->setValue("timeSlider", ui->timerSlider->value());
    settings->sync();
}

void MainWindow::updateTiming(){
    if(updateCheckTime_->isActive()){
        updateCheckTime_->stop();
    }
    if(!loadedPages_[0]){
        return;
    }
    settings->setValue( "timeSlider", ui->timerSlider->value());
    settings->setValue( "delay", secondsInWallpapersSlider_ );
    settings->sync();
}

void MainWindow::checkBatteryStatus(){
    if(!globalParser_->runsOnBattery()){
#ifdef Q_OS_UNIX
        batteryStatusChecker_->stop();
#endif

        switch(previouslyRunningFeature_){
        default:
        case 0:
            on_startButton_clicked();
            break;
        case 1:
            on_activate_livearth_clicked();
            break;
        case 2:
            on_activate_potd_clicked();
            break;
        case 3:
            on_activate_clock_clicked();
            break;
        case 4:
            on_activate_website_clicked();
            break;
        }

        previouslyRunningFeature_=0;
    }
}

void MainWindow::startButtonsSetEnabled(bool enabled)
{
    ui->startButton->setEnabled(enabled);
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, !enabled);
    }
#endif //#ifdef Q_OS_UNIX
}

void MainWindow::stopButtonsSetEnabled(bool enabled)
{
    ui->stopButton->setEnabled(enabled);

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled && !gv.wallpapersRunning);
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
    }
#endif //#ifdef Q_OS_UNIX
}

void MainWindow::previousAndNextButtonsSetEnabled(bool enabled){
    ui->next_Button->setEnabled(enabled); ui->previous_Button->setEnabled(enabled);

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityNextAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
        dbusmenu_menuitem_property_set_bool(gv.unityPreviousAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
    }
#endif //#ifdef Q_OS_UNIX
}

void MainWindow::monitor(const QStringList &finalListOfPaths){
    /*
     * Note: This function SHOULD NOT be called in a loop
     * because it updates the UI with the itemCount
     * This means that 'paths' QStringList should contain
     * the FULL list of paths that are to be monitored
     * and added to the Wallpaper's List.
    */
    /*
     * Monitor outside
     * For the people inside
     * A prevention of crime
     * A passing of time
     *
     * A monitor outside
     * A monitor outside
     *
     * They come and they go
     * It's a passing of time
     * They come and they go
     * Whilst we sit in our homes
     *
     * Sit back, sit back, sit back
     * Sit back and enjoy
     *
     * Monitor - Siouxsie and the Banshees
     */

    int itemCount = 0;
    Q_FOREACH(QString path, finalListOfPaths){
        monitor(path, itemCount);
    }

    if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START){
        startButtonsSetEnabled(false);
    }
    else
    {
        startButtonsSetEnabled(true);
    }

    if(gv.iconMode){
        launchTimerToUpdateIcons();
    }

    switch(ui->pictures_location_comboBox->currentIndex()){
    case 0:
        ui->pictures_location_comboBox->setItemText(0, gv.currentOSName+" "+tr("Desktop Backgrounds")+" ("+QString::number(itemCount)+")");
        break;
    case 1:
        ui->pictures_location_comboBox->setItemText(1,tr("My Pictures")+" ("+QString::number(itemCount)+ ")");
        break;
    default:
        short index=ui->pictures_location_comboBox->currentIndex();
        if(currentFolderIsAList_){
            ui->pictures_location_comboBox->setItemText(index, ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()+" ("+QString::number(itemCount)+ ")");
        }
        else
        {
            ui->pictures_location_comboBox->setItemText(index, globalParser_->basenameOf(ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString())+" ("+QString::number(itemCount)+ ")");
        }
        break;
    }
}

void MainWindow::monitor(const QString &path, int &itemCount)
{
    if(monitoredFoldersList_.contains(path, Qt::CaseSensitive)){
        return;
    }
    watchFolders_->addPath(path);
    monitoredFoldersList_ << path;
    addFilesToWallpapers(path, itemCount);
}

void MainWindow::folderChanged()
{
    /*
     *  This function just launches the research_folders_timer.
     *  This is done in order to prevent researching the folders constantly, when multiple changes
     *  are being made in a very short period of time.
     */
    if(researchFoldersTimer_->isActive()){
        researchFoldersTimer_->stop();
    }
    researchFoldersTimer_->start(RESEARCH_FOLDERS_TIMEOUT);
}

void MainWindow::researchFolders()
{
    researchFoldersTimer_->stop();

    if(gv.previewImagesOnScreen){
        //keep the current selected file (it will be selected again after the folder researching has finished)
        if(ui->wallpapersList->selectedItems().count()){
            if(gv.iconMode){
                if(ui->wallpapersList->selectedItems().at(0)->statusTip().isEmpty()){
                    nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->toolTip();
                }
                else
                {
                    nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->statusTip();
                }
            }
            else
            {
                nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->text();
            }
        }
    }

    if(searchIsOn_){
        on_search_close_clicked();
    }

    wallpaperManager_->clearWallpapers();

    ui->search_box->clear();
    ui->wallpapersList->clear();
    monitoredFoldersList_.clear();
    resetWatchFolders();

    if(currentFolderIsAList_){
        short foldersListCount=currentFolderList_.count();
        QStringList folderListSubfolders;
        for(short i=0;i<foldersListCount;i++){
            if(QDir(currentFolderList_.at(i)).exists()){
                folderListSubfolders << globalParser_->listFolders(currentFolderList_.at(i), true, true);
            }
        }
        monitor(folderListSubfolders);
    }
    else
    {
        monitor(globalParser_->listFolders(currentFolder_, true, true));
    }

    if(ui->wallpapersList->count() < LEAST_WALLPAPERS_FOR_START){
        if(gv.wallpapersRunning){
            on_stopButton_clicked();
        }
    }
    else
    {
        startButtonsSetEnabled(true);
    }

    if(gv.wallpapersRunning){
        processRunningResetPictures();
    }

    if(gv.previewImagesOnScreen && ui->stackedWidget->currentIndex()==0){
        searchFor(nameOfSelectionPriorFolderChange_);
        ui->screen_label_info->clear();
        updateScreenLabel();
    }
}

void MainWindow::addFilesToWallpapers(const QString &path, int &itemCount){
    QString cleanPath = path.endsWith('/') ? path.left(path.count()-1) : path;
    QDir currrentDirectory(cleanPath, QString(""), QDir::Name, QDir::Files);
    if(gv.iconMode){
        Q_FOREACH(QString file, currrentDirectory.entryList(IMAGE_FILTERS)){
            QListWidgetItem *item = new QListWidgetItem;
            item->setIcon(imageLoading_);
            item->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
            item->setToolTip(cleanPath+'/'+file);
            wallpaperManager_->addWallpaper(cleanPath+'/'+file);
            ui->wallpapersList->addItem(item);
            itemCount++;
        }
    }
    else
    {
        QString fullPath;
        Q_FOREACH(QString file, currrentDirectory.entryList(IMAGE_FILTERS)){
            fullPath=cleanPath+'/'+file;
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(fullPath);
            item->setToolTip("");
            item->setStatusTip(fullPath);
            wallpaperManager_->addWallpaper(fullPath);
            ui->wallpapersList->addItem(item);
            itemCount++;
        }
    }
}

void MainWindow::changePathsToIcons()
{
    ui->wallpapersList->setIconSize(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
    ui->wallpapersList->setViewMode(QListView::IconMode);
    ui->wallpapersList->setUniformItemSizes(true);
    ui->wallpapersList->setDragEnabled(false);
    gv.iconMode=true;
    int listCount=ui->wallpapersList->count();

    for(int i=0;i<listCount;i++){
        QString status=ui->wallpapersList->item(i)->statusTip();
        ui->wallpapersList->item(i)->setToolTip(status);
        ui->wallpapersList->item(i)->setStatusTip("");
        ui->wallpapersList->item(i)->setText("");
        ui->wallpapersList->item(i)->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->wallpapersList->item(i)->setIcon(imageLoading_);
    }
    launchTimerToUpdateIcons();
    if(gv.previewImagesOnScreen && ui->stackedWidget->currentIndex()==0){
        hideScreenLabel();
        updateScreenLabel();
    }
    if(searchIsOn_){
        on_search_close_clicked();
    }
    ui->search_box->clear();
}

void MainWindow::changeIconsToPaths(){
    ui->wallpapersList->setViewMode(QListView::ListMode);
    ui->wallpapersList->setUniformItemSizes(false);
    ui->wallpapersList->setIconSize(QSize(-1, -1));
    gv.iconMode=false;
    int file_count=ui->wallpapersList->count();

    QFont font;
    font.defaultFamily();
    for(int i=0;i<file_count;i++){
        ui->wallpapersList->item(i)->setTextAlignment(Qt::AlignLeft);
        ui->wallpapersList->item(i)->setFont(font);
        if(!ui->wallpapersList->item(i)->statusTip().isEmpty()){
            ui->wallpapersList->item(i)->setText(ui->wallpapersList->item(i)->statusTip());
        }
        else
        {
            QString path=ui->wallpapersList->item(i)->toolTip();
            ui->wallpapersList->item(i)->setText(path);
            ui->wallpapersList->item(i)->setStatusTip(path);
            ui->wallpapersList->item(i)->setToolTip("");
        }
        ui->wallpapersList->item(i)->setIcon(QIcon(""));
        ui->wallpapersList->item(i)->setData(Qt::SizeHintRole, QVariant());
    }
    if(gv.previewImagesOnScreen && ui->stackedWidget->currentIndex()==0){
        hideScreenLabel();
        updateScreenLabel();
    }
    if(searchIsOn_){
        on_search_close_clicked();
    }
    ui->search_box->clear();
}

void MainWindow::launchTimerToUpdateIcons(){
    if(gv.iconMode){
        if(iconUpdater_->isActive()){
            iconUpdater_->stop();
        }

        currentlyUpdatingIcons_=false;
        iconUpdater_->start(300);
    }
}

void MainWindow::updateVisibleIcons(){
    if(!ui->wallpapersList->count()){
        return;
    }
    currentlyUpdatingIcons_=true;
    QListWidgetItem* minItem=ui->wallpapersList->itemAt(0, 0);
    QListWidgetItem* maxItem=ui->wallpapersList->itemAt(ui->wallpapersList->width()-30, ui->wallpapersList->height()-5);

    int diff=30;
    while(!maxItem){
        diff+=5;
        maxItem=ui->wallpapersList->itemAt(ui->wallpapersList->width()-diff, ui->wallpapersList->height()-5);
        if(ui->wallpapersList->width()-diff<0){
            maxItem=ui->wallpapersList->item(ui->wallpapersList->count()-1);
        }
    }

    int min_row=ui->wallpapersList->row(minItem);
    int max_row=ui->wallpapersList->row(maxItem);
    for (int row = min_row; row <= max_row; row++) {
        if(!currentlyUpdatingIcons_ || appAboutToClose_){
            //either a new timer has been launched for this function or the application is about to close.
            return;
        }
        if(!ui->wallpapersList->item(row)){
            continue; //<- in case the icons are loading and the user chooses to clear the list or the item has somehow been removed
        }
        if(ui->wallpapersList->item(row)->statusTip().isEmpty()){
            if(updateIconOf(row)==false){
                //some image was null, go for the next row (which now is row--)
                row--;
            }
        }
    }
}

bool MainWindow::updateIconOf(int index){
    //updates the icon of the given row (item). It only works in icon mode, of course.
    QString originalImagePath = ui->wallpapersList->item(index)->toolTip();

    QImage image = cacheManager_->controlCache(originalImagePath);

    if(image.isNull()){
        if(ui->wallpapersList->count() > 1){
            delete ui->wallpapersList->item(index);
            wallpaperManager_->getCurrentWallpapers().removeAt(index);
        }
        else
        {
            ui->wallpapersList->clear();
            wallpaperManager_->clearWallpapers();
        }
        return false;
    }

    ui->wallpapersList->item(index)->setIcon(QIcon(QPixmap::fromImage(image)));
    ui->wallpapersList->item(index)->setStatusTip(originalImagePath);
    ui->wallpapersList->item(index)->setToolTip("");
    qApp->processEvents(QEventLoop::AllEvents);
    return true;
}

void MainWindow::forceUpdateIconOf(int index){
    if(ui->wallpapersList->item(index)->statusTip().isEmpty()){
        //image icon has not been updated, anyway (it wasn't any time visible to the user), just delete it if it is cached
        cacheManager_->removeCacheOf(ui->wallpapersList->item(index)->toolTip());
        return;
    }
    //update the image and save the cache of it
    QImage thumbnail = cacheManager_->forceCache(ui->wallpapersList->item(index)->statusTip());

    ui->wallpapersList->item(index)->setIcon(QIcon(QPixmap::fromImage(thumbnail)));

    if(gv.previewImagesOnScreen && ui->wallpapersList->currentRow() == index && ui->stackedWidget->currentIndex() == 0){
        updateScreenLabel();
    }
}

QStringList MainWindow::getCurrentWallpaperFolders(){
    if(currentFolderIsAList_){
        return currentFolderList_;
    }
    else
    {
        return QStringList() << currentFolder_;
    }
}

void MainWindow::on_browse_folders_clicked()
{
    addDialogShown_=true;

    QString initial_folder=settings->value("last_opened_folder", "").toString();

    if(!QDir(initial_folder).exists() || initial_folder.isEmpty()){
        initial_folder=gv.defaultPicturesLocation;
        if(!QDir(initial_folder).exists()){
            initial_folder=gv.homePath;
        }
    }

    QString folder = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), initial_folder);

    addDialogShown_=false;
    if(folder.isEmpty()){
        return;
    }

    settings->setValue("last_opened_folder", folder);
    settings->sync();

    addFolderForMonitor(folder);
}

void MainWindow::enterPressed(){
    if(ui->search_box->hasFocus()){
        if(ui->search_box->text().isEmpty()){
            return;
        }
        continueToNextMatch();
    }
    else if(ui->stackedWidget->currentIndex()==0){
        on_wallpapersList_itemDoubleClicked();
    }
}

void MainWindow::delayed_pictures_location_change()
{
    ui->pictures_location_comboBox->setCurrentIndex(tempForDelayedPicturesLocationChange_);
}

bool MainWindow::atLeastOneFolderFromTheSetExists()
{
    currentFolderList_.clear();
    currentFolderList_=ui->pictures_location_comboBox->itemData(ui->pictures_location_comboBox->currentIndex(), Qt::UserRole+2).toString().split("\n");
    short foldersListCount=currentFolderList_.count();
    for(short i=0;i<foldersListCount;i++){
        if(QDir(currentFolderList_.at(i)).exists()){
            return true;
        }
    }
    return false;
}

void MainWindow::on_pictures_location_comboBox_currentIndexChanged(int index)
{
    if(changingPicturesLocations_)
        return;

    if(searchIsOn_){
        on_search_close_clicked();
    }
    ui->search_box->clear();

    if(!monitoredFoldersList_.isEmpty())
    {
        monitoredFoldersList_.clear();
        resetWatchFolders();
        ui->wallpapersList->clear();
    }

    wallpaperManager_->clearWallpapers();

    bool selectionIsOk=false;

    if(ui->pictures_location_comboBox->itemData(index, Qt::UserRole+1).toBool())
    {
        currentFolderIsAList_=true;
        if(atLeastOneFolderFromTheSetExists()){
            short foldersListCount=currentFolderList_.count();
            QStringList folderListSubfolders;
            for(short i=0;i<foldersListCount;i++){
                if(QDir(currentFolderList_.at(i)).exists()){
                    folderListSubfolders << globalParser_->listFolders(currentFolderList_.at(i), true, true);
                }
            }
            monitor(folderListSubfolders);
            selectionIsOk=true;
        }
    }
    else
    {
        currentFolderIsAList_=false;
        currentFolder_=ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString();
        if(QDir(currentFolder_).exists())
        {
            currentFolderIsAList_=false;
            monitor(globalParser_->listFolders(currentFolder_, true, true));
            selectionIsOk=true;
        }
    }

    if(selectionIsOk)
    {
        if ( wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START ){
            startButtonsSetEnabled(false);
        }
        else{
            startButtonsSetEnabled(true);
        }
        settings->setValue("currentFolder_index", ui->pictures_location_comboBox->currentIndex());
        settings->sync();

        if(ui->wallpapersList->count()){
            ui->wallpapersList->setCurrentRow(0);
        }

        if(gv.wallpapersRunning){
            processRunningResetPictures();
        }
    }
    else
    {
        startButtonsSetEnabled(false);
        if(gv.wallpapersRunning){
            on_stopButton_clicked();
        }
        if(gv.mainwindowLoaded)
        {
            if(index==0){
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your System's backgrounds folder."));
            }
            else if(index==1){
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your Pictures folder."));
            }
            else
            {
                currentFolderDoesNotExist();
            }
        }
    }
}

void MainWindow::processRunningResetPictures(){
    /*
     * This function handles the case when the parent folder for the wallpapers is changed
     */

    //folder has changed/updated
    if(ui->shuffle_images_checkbox->isChecked()){
        if(wallpaperManager_->wallpapersCount()<LEAST_WALLPAPERS_FOR_START){
            on_stopButton_clicked();//not enough pictures to continue
        }
        else
        {
            //generate new random images
            secondsLeft_=0;
            wallpaperManager_->startOver();
        }
    }
    else if(wallpaperManager_->wallpapersCount()>=LEAST_WALLPAPERS_FOR_START)
    {
        wallpaperManager_->startOver(); //start from the beginning of the new folder
        secondsLeft_=0;
    }
    else
    {
        on_stopButton_clicked();//not enough pictures to continue
    }
}

void MainWindow::savePicturesLocations()
{
    // Updates the configuration of the pictures_locations
    settings->setValue("currentFolder_index", ui->pictures_location_comboBox->currentIndex());
    settings->beginWriteArray("pictures_locations");
    settings->remove("");
    short count = ui->pictures_location_comboBox->count();
    for (short i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        if(ui->pictures_location_comboBox->itemData(i, Qt::UserRole+1).toBool()){
            QStringList folders_list = ui->pictures_location_comboBox->itemData(i, Qt::UserRole+2).toString().split("\n");
            short folders_list_count= folders_list.count();
            settings->setValue("item", ui->pictures_location_comboBox->itemData(i, Qt::UserRole).toString());
            settings->setValue("type", 1);
            settings->setValue("size", folders_list_count);
            settings->beginWriteArray(QString::number(i));
            for (short j = 0; j < folders_list_count; ++j) {
                settings->setArrayIndex(j);
                settings->setValue("item", folders_list.at(j));
            }
            settings->endArray();
        }
        else{
            settings->setValue("item", ui->pictures_location_comboBox->itemData(i, Qt::UserRole));
        }
    }
    settings->endArray();
    settings->sync();
}

void MainWindow::on_search_box_textChanged(const QString &arg1)
{
    if(!arg1.isEmpty()){
        searchFor(arg1);
    }
}

void MainWindow::on_search_up_clicked()
{
    if(ui->search_box->text().isEmpty()){
        return;
    }
    ui->search_box->setFocus();
    continueToPreviousMatch();
}

void MainWindow::on_search_down_clicked()
{
    ui->search_box->setFocus();
    enterPressed();
}

void MainWindow::doesntMatch(){
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::red);
    ui->search_box->setPalette(pal);
}

void MainWindow::doesMatch(){
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::black);
    ui->search_box->setPalette(pal);
}

void MainWindow::showHideSearchBoxMenu(){
    if(searchIsOn_){
        ui->search_box->setFocus();
        return;
    }
    showHideSearchBox();
}

void MainWindow::showHideSearchBox(){
    if(!ui->stackedWidget->currentIndex()==0){
        return;
    }

    openCloseSearch_->setStartValue(ui->search_widget->maximumHeight());

    if(!searchIsOn_)
    {
        ui->search_widget->show();
        openCloseSearch_->setEndValue(30);
        ui->search_box->setFocus();
        on_search_box_textChanged(ui->search_box->text()); //in case a previous search exists
    }
    else
    {
        //seach to be closed
        searchList_.clear();
        openCloseSearch_->setEndValue(0);
        //more icons may become available once the search box closes
        launchTimerToUpdateIcons();
    }
    openCloseSearch_->start();
    searchIsOn_=!searchIsOn_;
}

void MainWindow::openCloseSearchAnimationFinished()
{
    if(openCloseSearch_->endValue()==0){
        ui->search_widget->hide();
    }
}

void MainWindow::on_search_close_clicked()
{
    if(ui->stackedWidget->currentIndex()!=0){
        return;
    }
    showHideSearchBox();
}

//Live Earth code

void MainWindow::on_activate_livearth_clicked()
{
    if(!ui->activate_livearth->isEnabled()){
        return;
    }
    stopEverythingThatsRunning(2);
    on_page_1_earth_clicked();
    ui->activate_livearth->setEnabled(false);
    ui->deactivate_livearth->setEnabled(true);
    gv.liveEarthRunning=true;
    globalParser_->updateStartup();
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalRecreateTray();
    setProgressbarsValue(100);
    startUpdateSeconds();
    animateProgressbarOpacity(1);
}

void MainWindow::on_deactivate_livearth_clicked()
{
    if(!ui->deactivate_livearth->isEnabled()){
        return;
    }

    secondsLeft_=0;
    imageFetcher_->abort();
    gv.liveEarthRunning=false;
    globalParser_->updateStartup();
    processRequestStop();

    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    if(gv.independentIntervalEnabled){
        globalParser_->saveSecondsLeftNow(-1, 1);
    }
    animateProgressbarOpacity(0);
    ui->deactivate_livearth->setEnabled(false);
    ui->activate_livearth->setEnabled(true);
    stoppedBecauseOnBattery_=false;

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalUncheckRunningFeatureOnTray();
}

void MainWindow::on_le_tag_checkbox_clicked(bool checked)
{
    gv.leEnableTag = checked;
    ui->le_tag_button->setEnabled(checked);
    settings->setValue("le_enable_tag", checked);
    settings->sync();
}

void MainWindow::on_le_tag_button_clicked()
{
    lePointShown_=true;
    lepoint_ = new LEPoint(this);
    connect(lepoint_, SIGNAL(lePreferencesChanged()), this, SLOT(restartLeIfRunningAfterSettingChange()));
    lepoint_->setModal(true);
    lepoint_->setAttribute(Qt::WA_DeleteOnClose);
    connect(lepoint_, SIGNAL(destroyed()), this, SLOT(lePointDestroyed()));
    lepoint_->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    lepoint_->show();
}

void MainWindow::lePointDestroyed(){
    lePointShown_=false;
}

//Picture of they day code

void MainWindow::on_activate_potd_clicked()
{
    if(!ui->activate_potd->isEnabled()){
        return;
    }
    stopEverythingThatsRunning(3);
    on_page_2_potd_clicked();
    startPotd(true);
}

void MainWindow::startPotd(bool launchNow){
    ui->deactivate_potd->setEnabled(true);
    ui->activate_potd->setEnabled(false);
    gv.potdRunning=true;
    globalParser_->updateStartup();
    justUpdatedPotd_=false;
    if(launchNow){
        actionsOnWallpaperChange();
    }
    animateProgressbarOpacity(1);

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalRecreateTray();
    startUpdateSeconds();
    updatePotdProgress();
}

void MainWindow::on_deactivate_potd_clicked()
{
    if(!ui->deactivate_potd->isEnabled()){
        return;
    }

    secondsLeft_=0;
    processRequestStop();
    imageFetcher_->abort();
    gv.potdRunning=false;
    globalParser_->updateStartup();
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    animateProgressbarOpacity(0);
    ui->activate_potd->setEnabled(true);
    ui->deactivate_potd->setEnabled(false);
    stoppedBecauseOnBattery_=false;

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalUncheckRunningFeatureOnTray();
}

void MainWindow::restartPotdIfRunningAfterSettingChange(){
    if(!gv.potdRunning){
        settings->setValue("potd_preferences_have_changed", true);
        settings->sync();
        return;
    }
    on_deactivate_potd_clicked();
    globalParser_->remove(gv.wallchHomePath+POTD_IMAGE+"*");
    settings->setValue("last_day_potd_was_set", "");
    settings->sync();
    startPotd(true);
}

void MainWindow::restartLeIfRunningAfterSettingChange()
{
    if (!gv.liveEarthRunning) {
        return;
    }

    on_deactivate_livearth_clicked();
    on_activate_livearth_clicked();
}

//Wallpaper clocks code

void MainWindow::on_install_clock_clicked()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Import Wallpaper Clock(s)"), gv.homePath, "*.wcz");

    if(paths.isEmpty()){
        return;
    }

    Q_FOREACH(QString path, paths){
        installWallpaperClock(path);
    }
    if(!gv.wallpaperClocksRunning){
        ui->clocksTableWidget->selectRow(ui->clocksTableWidget->rowCount());
    }
    ui->clocksTableWidget->setFocus();
}

void MainWindow::installWallpaperClock(const QString &currentPath){

    currentWallpaperClockPath_=gv.wallchHomePath+"Wallpaper_clocks/"+QFileInfo(currentPath).baseName();
    if(QDir(gv.wallchHomePath+"Wallpaper_clocks/").exists()){
        if(QFile::exists(currentWallpaperClockPath_))
        {
            QMessageBox::warning(this, tr("Error!"), QString(tr("Wallpaper clock already installed.")+" ("+currentWallpaperClockPath_+")"));
            return;
        }
    }
    else
    {
        if(!QDir().mkpath(gv.wallchHomePath+"Wallpaper_clocks/")){
            QMessageBox::warning(this, tr("Error!"), QString(tr("Could not create an essential path for the Wallpaper clock use. Your Wallpaper clock could not be installed.")));
            return;
        }
    }

#ifdef Q_OS_UNIX
    unzipArchive_ = new QProcess(this);
    QStringList params;
    params << "-qq" << "-o" << currentPath << "-d" << currentWallpaperClockPath_;
    connect(unzipArchive_, SIGNAL(finished(int)), this, SLOT(addClockToList()));
    unzipArchive_->start("unzip", params);
    unzipArchive_->waitForFinished();
#else
    unzip(currentPath, currentWallpaperClockPath_);
#endif
    ui->clocksTableWidget->setFocus();
    if(!gv.wallpaperClocksRunning)
        ui->clocksTableWidget->selectRow(ui->clocksTableWidget->rowCount()-1);
}

void MainWindow::addClockToList(){
#ifdef Q_OS_UNIX
    if(unzipArchive_ && !unzipArchive_->isOpen()){
        unzipArchive_->deleteLater();
    }
#endif

    QFile file(currentWallpaperClockPath_+"/clock.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        globalParser_->error("I could not open your clock.ini file successfully!");
        return;
    }
    QString clockName, clockWidth, clockHeight;

    QTextStream in(&file);
    bool name_found=false, w_found=false, h_found=false;
    while(!in.atEnd())
    {
        QString line=in.readLine();
        if(!name_found){
            if(line.left(5)=="name=")
            {
                clockName=line.right(line.count()-5);
                name_found=true;
            }
        }
        if(!w_found){
            if(line.left(6)=="width=")
            {
                clockWidth=line.right(line.count()-6);
                w_found=true;
            }
        }
        if(!h_found){
            if(line.left(7)=="height=")
            {
                clockHeight=line.right(line.count()-7);
                h_found=true;
            }
        }

    }
    file.close();
    int old_r_count=ui->clocksTableWidget->rowCount();
    ui->clocksTableWidget->insertRow(ui->clocksTableWidget->rowCount());
    //adding radio button, which one is selected is the one that will be used when Wallpaper clocks are activated
    QWidget* wdg = new QWidget;
    QRadioButton *radiobutton_item = new QRadioButton(this);
    clocksRadioButtonsGroup->addButton(radiobutton_item);
    connect(radiobutton_item, SIGNAL(clicked()), this, SLOT(clockRadioButton_check_state_changed()));
    radiobutton_item->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout* layout = new QHBoxLayout(wdg);
    layout->addWidget(radiobutton_item);
    layout->setAlignment( Qt::AlignCenter );
    layout->setMargin(0);
    wdg->setLayout(layout);
    ui->clocksTableWidget->setCellWidget(old_r_count, 0, wdg);

    //adding preview image
    QLabel *label_item = new QLabel(this);
    label_item->setAlignment(Qt::AlignCenter);
    label_item->setPixmap(QPixmap(currentWallpaperClockPath_+"/preview100x75.jpg").scaled(80, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->clocksTableWidget->setCellWidget(old_r_count, 1, label_item);

    //adding title of clock and dimensions
    QTableWidgetItem *name_item = new QTableWidgetItem;
    name_item->setText(clockName+"\n"+clockWidth+"x"+clockHeight);
    name_item->setData(32, currentWallpaperClockPath_);
    name_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->clocksTableWidget->setItem(old_r_count,2,name_item);

    if(!gv.wallpaperClocksRunning)
    {
        ui->activate_clock->setEnabled(true);
    }
}

#ifdef Q_OS_WIN
void MainWindow::unzip(QString input, const QString &outputDirectory){
    QDir baseDir(outputDirectory);
    baseDir.mkpath(baseDir.path());
    QZipReader unzip(input, QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> allFiles = unzip.fileInfoList();

    Q_FOREACH (QZipReader::FileInfo fi, allFiles)
    {
        const QString absPath = outputDirectory + QDir::separator() + fi.filePath;
        if(fi.isDir){
            if (!baseDir.mkpath(fi.filePath)){
                continue;
            }
            if (!QFile::setPermissions(absPath, fi.permissions)){
                continue;
            }
        }
        else if (fi.isFile)
        {
            QFile file(absPath);
            if( file.open(QFile::WriteOnly) )
            {
                file.write(unzip.fileData(fi.filePath), unzip.fileData(fi.filePath).size());
                file.setPermissions(fi.permissions);
                file.close();
            }
        }
        qApp->processEvents(QEventLoop::AllEvents);
    }
    unzip.close();

    addClockToList();
}
#endif

void MainWindow::uninstall_clock()
{
    if(currentlyUninstallingWallpaperClock_ || ui->clocksTableWidget->rowCount()==0 ){
        return;
    }

    if(gv.wallpaperClocksRunning)
    {
        return;
    }

    currentlyUninstallingWallpaperClock_=true;

    if(!QDir(gv.defaultWallpaperClock).removeRecursively()){
        globalParser_->error("Could not delete wallpaper clock, check folder's and subfolders' permissions.");
    }

    if(ui->clocksTableWidget->rowCount()==1){
        ui->activate_clock->setEnabled(false);
        gv.defaultWallpaperClock="None";
        settings->setValue("default_wallpaper_clock", gv.defaultWallpaperClock);
        settings->sync();
        if(gv.previewImagesOnScreen){
            changeTextOfScreenLabelTo(tr("Preview not available"));
        }
    }

    ui->clocksTableWidget->removeRow(ui->clocksTableWidget->currentRow());

    currentlyUninstallingWallpaperClock_=false;
}

void MainWindow::on_clocksTableWidget_customContextMenuRequested()
{
    if(ui->clocksTableWidget->selectedRanges().count() == 0){
        return;
    }
    clockwidgetMenu_ = new QMenu(this);
    clockwidgetMenu_->addAction(tr("Uninstall"), this, SLOT(uninstall_clock()));
    clockwidgetMenu_->actions().at(0)->setIcon(QIcon::fromTheme("list-remove", QIcon(":/images/list-remove.png")));
    if(gv.wallpaperClocksRunning){
        clockwidgetMenu_->actions().at(0)->setEnabled(false);
    }
    clockwidgetMenu_->popup(MENU_POPUP_POS);
}

void MainWindow::clockRadioButton_check_state_changed()
{
    int rowCount=ui->clocksTableWidget->rowCount();
    for(int i=0; i<rowCount; i++)
    {
        QWidget* w = ui->clocksTableWidget->cellWidget(i, 0 );
        if( w )
        {
            QRadioButton* radioButton = w->findChild<QRadioButton*>();
            if( radioButton && radioButton->isChecked())
            {
                ui->clocksTableWidget->selectRow(i);
                break;
            }
        }
    }
}

void MainWindow::on_clocksTableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);

    if(currentRow==previousRow || (ui->clocksTableWidget->rowCount()==1 && currentlyUninstallingWallpaperClock_)){
        return;
    }

    gv.defaultWallpaperClock=ui->clocksTableWidget->item(currentRow,2)->data(32).toString();
    settings->setValue("default_wallpaper_clock", gv.defaultWallpaperClock);

    if(gv.wallpaperClocksRunning){
        globalParser_->readClockIni();
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                  ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
        if(gv.setAverageColor){
            setButtonColor();
        }
    }

    QWidget* w = ui->clocksTableWidget->cellWidget(currentRow, 0 );
    if( w )
    {
        QRadioButton* radioButton = w->findChild<QRadioButton*>();
        if( radioButton )
        {
            radioButton->setChecked(true);
        }
    }

    //just installed first wallpaper clock.. Selection has not yet been applied, so select first row in order to preview
    if(ui->clocksTableWidget->rowCount()==1 && ui->clocksTableWidget->selectedItems().count()==0){
        ui->clocksTableWidget->selectRow(0);
    }

    if(ui->clocksTableWidget->rowCount()==0 || ui->clocksTableWidget->selectedItems().count()==0)
    {
        if(gv.previewImagesOnScreen){
            hideScreenLabel();
        }
    }
    else
    {
        if(gv.previewImagesOnScreen){
            imageTransition(true);
        }
    }
}

void MainWindow::on_activate_clock_clicked()
{
    stopEverythingThatsRunning(4);
    if(!loadedPages_[3]){
        loadWallpaperClocksPage();
    }
    if(!ui->activate_clock->isEnabled()){
        globalParser_->desktopNotify(tr("No wallpaper clocks have been installed."), false, "info");
        globalParser_->error("No wallpaper clocks have been installed.");
        return;
    }

    on_page_3_clock_clicked();

    if(ui->minutes_checkBox->isChecked() || ui->hour_checkBox->isChecked() || (ui->am_checkBox->isChecked() && gv.amPmEnabled) ||
            ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked())
    {
        globalParser_->readClockIni();
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                                  ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }

        ui->activate_clock->setEnabled(false);
        ui->deactivate_clock->setEnabled(true);

        gv.wallpaperClocksRunning=true;
        globalParser_->updateStartup();
#ifdef Q_OS_UNIX
        if(gv.currentDE == DesktopEnvironment::UnityGnome){
            dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
        }
#endif //#ifdef Q_OS_UNIX
        Q_EMIT signalRecreateTray();

        calculateSecondsLeftForWallClocks();

        totalSeconds_=secondsLeft_;
        globalParser_->resetSleepProtection(secondsLeft_);
        startUpdateSeconds();
        setProgressbarsValue(100);
        animateProgressbarOpacity(1);
    }
    else
    {
        if (QMessageBox::question(this,tr("Wallpaper Clock"), tr("You haven't selected any of the options (day of week etc...), thus the wallpaper clock will not be activated. You can use the wallpaper of the selecte wallpaper clock as a background, instead.")) == QMessageBox::Yes)
        {
            QString pic=gv.defaultWallpaperClock+"/bg.jpg";
            WallpaperManager::setBackground(pic, true, true, 4);
            if(gv.setAverageColor){
                setButtonColor();
            }
        }
    }
}

void MainWindow::on_deactivate_clock_clicked()
{
    if(!ui->deactivate_clock->isEnabled()){
        return;
    }

    secondsLeft_=0;
    gv.wallpaperClocksRunning=false;
    globalParser_->updateStartup();
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    animateProgressbarOpacity(0);
    ui->deactivate_clock->setEnabled(false);
    ui->activate_clock->setEnabled(true);
    stoppedBecauseOnBattery_=false;

#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalUncheckRunningFeatureOnTray();
}

void MainWindow::calculateSecondsLeftForWallClocks(){
    QTime timeNow = QTime::currentTime();

    double secondsLeftForNextMinute=0;
    if(timeNow.minute()==59 && timeNow.hour()!=23){
        secondsLeftForNextMinute=timeNow.msecsTo(QTime(timeNow.hour()+1,0,1));
    }
    else if (timeNow.minute()==59 && timeNow.hour()==23){
        secondsLeftForNextMinute=timeNow.msecsTo(QTime(23,59,59))+2;
    }
    else{
        secondsLeftForNextMinute=timeNow.msecsTo(QTime(timeNow.hour(),timeNow.minute()+1,1));
    }

    secondsLeftForNextMinute-=500;

    secondsLeftForNextMinute/=1000;

    secondsLeftForNextMinute=qRound(secondsLeftForNextMinute);

    if(ui->minutes_checkBox->isChecked())
    {
        secondsLeft_=secondsLeftForNextMinute;
    }
    else if(ui->hour_checkBox->isChecked())
    {
        secondsLeft_=(((QString::number(timeNow.minute()/gv.refreshhourinterval).toInt()+1)*gv.refreshhourinterval-timeNow.minute()-1)*60+secondsLeftForNextMinute);
    }
    else if(ui->am_checkBox->isChecked() && gv.amPmEnabled)
    {
        float secondsLeftForAmPm=0;
        if (timeNow.hour()>=12){
            secondsLeftForAmPm=timeNow.secsTo(QTime(23,59,59))+2;
        }
        else
        {
            secondsLeftForAmPm=timeNow.secsTo(QTime(12,0,1));
        }
        secondsLeft_=secondsLeftForAmPm;
    }
    else if(ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked())
    {
        float seconds_left_for_next_day=timeNow.secsTo(QTime(23,59,59))+2;
        secondsLeft_=seconds_left_for_next_day;
    }
    else if(ui->month_checkBox->isChecked())
    {
        float seconds_left_for_next_day=timeNow.secsTo(QTime(23,59,59))+2;
        secondsLeft_=seconds_left_for_next_day;
    }
}

QImage MainWindow::wallpaperClocksPreview()
{
    const QString wallClockPath=gv.defaultWallpaperClock;
    bool minuteCheck=ui->minutes_checkBox->isChecked();
    bool hourCheck=ui->hour_checkBox->isChecked();
    bool amPmCheck=ui->am_checkBox->isChecked();
    bool dayWeekCheck=ui->day_week_checkBox->isChecked();
    bool dayMonthCheck=ui->day_month_checkBox->isChecked();
    bool monthCheck=ui->month_checkBox->isChecked();

    short hour_inte=0;
    short am_pm_en=0;
    short images_of_hour=0;

    //reading clock.ini of wallpaper clock in order to get the gv.refreshhourinterval,if there are images for am or pm and the total of hour images
    QFile file(wallClockPath+"/clock.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        globalParser_->error("I could not open the clock.ini file ("+wallClockPath+"/clock.ini) for reading successfully!");
        return QImage();
    }

    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line=in.readLine();
        if(line.startsWith("refreshhourinterval=")){
            hour_inte=QString(line.right(line.count()-20)).toInt();
        }
        else if(line.startsWith("ampmenabled=")){
            am_pm_en=QString(line.right(line.count()-12)).toInt();
        }
        else if(line.startsWith("ampmenabled=")){
            images_of_hour=QString(line.right(line.count()-11)).toInt();
        }
    }
    file.close();
    if(hour_inte==0){
        return QImage();
    }
    QDateTime datetime = QDateTime::currentDateTime();

    //12h format is needed for the analog clocks
    short hour_12h_format=0;
    QString am_or_pm;
    if (datetime.time().hour()>=12){
        am_or_pm="pm";
    }
    else
    {
        am_or_pm="am";
    }

    if(images_of_hour==24 || datetime.time().hour()<12 ){
        hour_12h_format=datetime.time().hour();
    }
    else if(images_of_hour!=24 && datetime.time().hour()>=12){
        hour_12h_format=datetime.time().hour()-12;
    }

    QImage merged = QImage(wallClockPath+"/bg.jpg");

    QPainter painter(&merged);
    if(clocksPreviewWatcher_->isCanceled()){
        return QImage();
    }

    if(monthCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/month"+QString::number(datetime.date().month())+".png"));
    }
    if(dayWeekCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/weekday"+QString::number(datetime.date().dayOfWeek())+".png"));
    }
    if(dayMonthCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/day"+QString::number(datetime.date().day())+".png"));
    }
    if(hourCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/hour"+QString::number(hour_12h_format*(60/hour_inte)+QString::number(datetime.time().minute()/hour_inte).toInt())+".png"));
    }
    if(minuteCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/minute"+QString::number(datetime.time().minute())+".png"));
    }
    if(am_pm_en && amPmCheck){
        painter.drawPixmap(0, 0, QPixmap(wallClockPath+"/"+am_or_pm+".png"));
    }

    painter.end();

    if(clocksPreviewWatcher_->isCanceled()){
        return QImage();
    }

    return merged;
}

void MainWindow::clockCheckboxClickedWhileRunning()
{
    if(ui->minutes_checkBox->isChecked() || ui->hour_checkBox->isChecked() || (ui->am_checkBox->isChecked() && gv.amPmEnabled) || ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked())
    {
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                                  ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }

        QTime time = QTime::currentTime();

        float seconds_left_for_min=0;
        if(time.minute()==59 && time.hour()!=23){
            seconds_left_for_min=time.secsTo(QTime(time.hour()+1,0,1));
        }
        else if (time.minute()==59 && time.hour()==23){
            seconds_left_for_min=time.secsTo(QTime(23,59,59))+2;
        }
        else
        {
            seconds_left_for_min=time.secsTo(QTime(time.hour(),time.minute()+1,1));
        }

        ui->activate_clock->setEnabled(false);
        ui->deactivate_clock->setEnabled(true);

        if(ui->minutes_checkBox->isChecked()){
            secondsLeft_=seconds_left_for_min;
        }
        else if(ui->hour_checkBox->isChecked()){
            secondsLeft_=(((QString::number(time.minute()/gv.refreshhourinterval).toInt()+1)*gv.refreshhourinterval-time.minute()-1)*60+seconds_left_for_min);
        }
        else if(ui->am_checkBox->isChecked() && gv.amPmEnabled)
        {
            float seconds_left_for_am_pm=0;
            if (time.hour()>=12){
                seconds_left_for_am_pm=time.secsTo(QTime(23,59,59))+2;
            }
            else{
                seconds_left_for_am_pm=time.secsTo(QTime(12,0,1));
            }
            secondsLeft_=seconds_left_for_am_pm;
        }
        else if(ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked())
        {
            float seconds_left_for_next_day=time.secsTo(QTime(23,59,59))+2;
            secondsLeft_=seconds_left_for_next_day;
        }

        totalSeconds_=secondsLeft_;
        startUpdateSeconds();
        setProgressbarsValue(100);
    }
    else
    {
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                                  ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }
        on_deactivate_clock_clicked();
    }
}

void MainWindow::on_wallpaper_clocks_source_linkActivated()
{
    globalParser_->openUrl("http://www.vladstudio.com/wallpaperclocks/browse.php");
}

void MainWindow::clockCheckboxClicked()
{
    imageTransition(true);

    settings->setValue("month", ui->month_checkBox->isChecked());
    settings->setValue("day_of_month", ui->day_month_checkBox->isChecked());
    settings->setValue("day_of_week", ui->day_week_checkBox->isChecked());
    settings->setValue("am_pm", ui->am_checkBox->isChecked());
    settings->setValue("hour", ui->hour_checkBox->isChecked());
    settings->setValue("minute", ui->minutes_checkBox->isChecked());
    settings->sync();

    if(gv.wallpaperClocksRunning)
    {
        if(wallpaperClockWait_->isActive()){
            wallpaperClockWait_->stop();
        }

        wallpaperClockWait_->start(500);
    }
}

//Live Website Code
void MainWindow::on_activate_website_clicked()
{   
    stopEverythingThatsRunning(5);
    if(!loadedPages_[4]){
        loadLiveWebsitePage();
    }
    if(websiteSnapshot_==NULL || !ui->activate_website->isEnabled() || !websiteConfiguredCorrectly()){
        return;
    }

    gv.websiteWebpageToLoad=gv.onlineLinkForHistory=ui->website->text();
    gv.websiteInterval=ui->website_slider->value();

    on_page_4_web_clicked();
    ui->stackedWidget->setCurrentIndex(4);

    ui->deactivate_website->setEnabled(true);
    ui->activate_website->setEnabled(false);
    ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT)+" "+tr("seconds")+"...");
    ui->timeout_text_label->show();
    ui->website_timeout_label->show();
    timePassedForLiveWebsiteRequest_=1;
    ui->live_website_login_widget->setEnabled(false);

    QApplication::processEvents(QEventLoop::AllEvents);

    gv.liveWebsiteRunning=true;
    globalParser_->updateStartup();
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalRecreateTray();
    setProgressbarsValue(100);
    animateProgressbarOpacity(1);

    prepareWebsiteSnapshot();
    websiteSnapshot_->setCrop(ui->website_crop_checkbox->isChecked(), gv.websiteCropArea);
    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    startUpdateSeconds();
}

void MainWindow::on_deactivate_website_clicked()
{
    if(!ui->deactivate_website->isEnabled()){
        return;
    }

    stoppedBecauseOnBattery_=false;
    gv.liveWebsiteRunning=false;
    globalParser_->updateStartup();
    secondsLeft_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    if(gv.independentIntervalEnabled){
        globalParser_->saveSecondsLeftNow(-1, 2);
    }
    ui->live_website_login_widget->setEnabled(true);
    animateProgressbarOpacity(0);
#ifdef Q_OS_UNIX
    if(gv.currentDE == DesktopEnvironment::UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
#endif //#ifdef Q_OS_UNIX
    Q_EMIT signalUncheckRunningFeatureOnTray();
    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    ui->deactivate_website->setEnabled(false); ui->activate_website->setEnabled(true);
    ui->timeout_text_label->hide();
    ui->website_timeout_label->hide();
    processRequestStop();
    websiteSnapshot_->stop();
}

void MainWindow::on_website_preview_clicked()
{
    if(gv.liveWebsiteRunning){
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" ("+tr("Live Website")+")");
        return;
    }

    if(websitePreviewShown_ || !websiteConfiguredCorrectly()){
        return;
    }

    if(changedWebPage_!=initialWebPage_ && !(changedWebPage_==initialWebPage_+"/" || changedWebPage_+"/"==initialWebPage_)){
        initialWebPage_=changedWebPage_;
    }

    QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);

    websitePreviewShown_=true;

    prepareWebsiteSnapshot();

    webPreview_ = new WebsitePreview(websiteSnapshot_, false, ui->website_crop_checkbox->isChecked(), gv.websiteCropArea, this);
    webPreview_->setModal(true);
    webPreview_->setAttribute(Qt::WA_DeleteOnClose);
    connect(webPreview_, SIGNAL(destroyed()), this, SLOT(websitePreviewDestroyed()));
    connect(webPreview_, SIGNAL(previewImageReady(QImage*)), this, SLOT(setWebsitePreviewImage(QImage*)));
    connect(webPreview_, SIGNAL(sendExtraCoordinates(QRect)), this, SLOT(readCoordinates(const QRect&)));
    webPreview_->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    webPreview_->show();
}

void MainWindow::liveWebsiteImageCreated(QImage *image, short errorCode){
    ui->timeout_text_label->hide();
    ui->website_timeout_label->hide();
    timePassedForLiveWebsiteRequest_=0;
    ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT)+" "+tr("seconds")+"...");
    processRequestStop();
    if(errorCode==0){
        //no error!
        globalParser_->remove(gv.wallchHomePath+LW_IMAGE+"*");
        QString filename=gv.wallchHomePath+LW_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+".png";
        image->save(filename);
        delete image;

        WallpaperManager::setBackground(filename, true, true, 5);
        if(gv.setAverageColor){
            setButtonColor();
        }
        QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        QFile(filename).link(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        updateScreenLabel();
    }
    else
    {
        switch(errorCode){
        case 1:
            globalParser_->error("Some of the requested pages failed to load successfully.");
            break;
        case 2:
            globalParser_->error("Simple authentication failed. Please check your username and/or password.");
            break;
        case 3:
            globalParser_->error("Username and/or password fields are not found. Please check that you are pointing at the login page.");
            break;
        case 4:
            globalParser_->error("The timeout has been reached and the image has yet to be created!");
            break;
        default:
            globalParser_->error("Unknown error! Please try with a different web page.");
            break;
        }
    }
}

void MainWindow::prepareWebsiteSnapshot(){
    /*
     * This does not prepare websiteSnapshot for
     * the cropping that it has to do (for compatibility
     * with the crop_image and website_preview dialogs)
     */
    websiteSnapshot_->setParameters(QUrl(ui->website->text()), gv.screenWidth, gv.screenHeight);

    websiteSnapshot_->setWaitAfterFinish(gv.websiteWaitAfterFinishSeconds);
    websiteSnapshot_->setJavascriptConfig(gv.websiteJavascriptEnabled, gv.websiteJavascriptCanReadClipboard);
    websiteSnapshot_->setJavaEnabled(gv.websiteJavaEnabled);
    websiteSnapshot_->setLoadImagesEnabled(gv.websiteLoadImages);

    if(ui->add_login_details->isChecked()){
        QString final_webpage=ui->final_webpage->text();
        if(!ui->redirect_checkBox->isChecked() || final_webpage.isEmpty()){
            final_webpage=ui->website->text();
        }
        if(gv.websiteSimpleAuthEnabled){
            websiteSnapshot_->setSimpleAuthentication(ui->username->text(), ui->password->text(), final_webpage);
        }
        else
        {
            if(gv.websiteExtraUsernames.count()>0 || gv.websiteExtraPasswords.count()>0)
            {
                websiteSnapshot_->setComplexAuthenticationWithPossibleFields(ui->username->text(), ui->password->text(), final_webpage, gv.websiteExtraUsernames, gv.websiteExtraPasswords, true);
            }
            else
            {
                websiteSnapshot_->setComplexAuthentication(ui->username->text(), ui->password->text(), final_webpage);
            }
        }
    }
    else
    {
        websiteSnapshot_->disableAuthentication();
    }

    websiteSnapshot_->setTimeout(WEBSITE_TIMEOUT);
}

void MainWindow::setWebsitePreviewImage(QImage *image){
    image->save(gv.wallchHomePath+LW_PREVIEW_IMAGE);
    delete image;
    updateScreenLabel();
}

void MainWindow::on_edit_crop_clicked()
{
    if(gv.liveWebsiteRunning){
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" ("+tr("Live Website")+")");
        return;
    }

    if(changedWebPage_!=initialWebPage_ && !(changedWebPage_==initialWebPage_+"/" || changedWebPage_+"/"==initialWebPage_)){
        initialWebPage_=changedWebPage_;
    }

    QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);

    websitePreviewShown_=true;

    prepareWebsiteSnapshot();

    webPreview_ = new WebsitePreview(websiteSnapshot_, true, false, gv.websiteCropArea, this);
    webPreview_->setModal(true);
    webPreview_->setAttribute(Qt::WA_DeleteOnClose);
    connect(webPreview_, SIGNAL(destroyed()), this, SLOT(websitePreviewDestroyed()));
    connect(webPreview_, SIGNAL(sendExtraCoordinates(QRect)), this, SLOT(readCoordinates(const QRect&)));
    webPreview_->setWindowFlags(Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    webPreview_->show();
}

void MainWindow::websitePreviewDestroyed(){
    websitePreviewShown_=false;
}

void MainWindow::on_website_crop_checkbox_clicked(bool checked)
{
    if(gv.liveWebsiteRunning){
        ui->website_crop_checkbox->setChecked(!checked);
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" ("+tr("Live Website")+")");
        return;
    }

    ui->edit_crop->setEnabled(checked);
}

void MainWindow::readCoordinates(const QRect &cropArea){
    gv.websiteCropArea=cropArea;
    settings->setValue("website_crop_area", gv.websiteCropArea);
    settings->sync();
    updateScreenLabel();
}
void MainWindow::on_website_textEdited(const QString &arg1)
{
    changedWebPage_=arg1;
}

bool MainWindow::websiteConfiguredCorrectly(){
    if(!ui->website->text().startsWith("h") && !ui->website->text().startsWith("f")){
        ui->website->setText("http://"+ui->website->text());
    }

    if(ui->website->text().count()<=4 || !ui->website->text().contains(".")){
        QMessageBox::warning(this, tr("Error"), tr("Invalid address specified!"));
        return false;
    }

    if(ui->add_login_details->isChecked()){
        if(ui->username->text().isEmpty() || ui->password->text().isEmpty()){
            QMessageBox::warning(this, tr("Error"), tr("Invalid username or password specified."));
            return false;
        }
    }
    return true;
}

void MainWindow::picturesLocationsChanged()
{
    changingPicturesLocations_=true;
    short currentFolder=settings->value("currentFolder_index", 0).toInt();
    short count=ui->pictures_location_comboBox->count();
    for(short i=count-1;i>1;i--){
        ui->pictures_location_comboBox->removeItem(i);
    }
    short size=settings->beginReadArray("pictures_locations");
    for(short i=2;i<size;i++){
        settings->setArrayIndex(i);
        if(settings->value("type").toBool()){
            short list_size=settings->beginReadArray(QString::number(i));
            QString all_folders;
            for(short i=0;i<list_size;i++){
                settings->setArrayIndex(i);
                all_folders+=settings->value("item").toString();
                if(i!=list_size-1){
                    all_folders+="\n";
                }
            }
            settings->endArray();
            ui->pictures_location_comboBox->addItem(settings->value("item").toString());
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, 1, Qt::UserRole+1);
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, settings->value("item").toString(), Qt::UserRole);
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, all_folders, Qt::UserRole+2);
        }
        else {
            QString qpath=settings->value("item").toString();
            ui->pictures_location_comboBox->addItem(globalParser_->basenameOf(qpath));
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, qpath, Qt::UserRole);
        }
    }
    settings->endArray();
    changingPicturesLocations_=false;
    ui->pictures_location_comboBox->setCurrentIndex(currentFolder);
}

//Pages code

void MainWindow::loadWallpapersPage(){

    if(wallpaperManager_==NULL){
        wallpaperManager_=new WallpaperManager();
    }

    ui->previous_Button->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/images/media-seek-backward.png")));
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
    ui->stopButton->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/images/media-playback-stop.png")));
    ui->next_Button->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/images/media-seek-forward.png")));
    ui->search_close->setIcon(QIcon::fromTheme("window-close", QIcon(":/images/window-close.png")));
    ui->search_down->setIcon(QIcon::fromTheme("go-down", QIcon(":/images/go-down.png")));
    ui->search_up->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/go-up.png")));

    ui->wallpapersList->setItemDelegate(new HideGrayLinesDelegate(ui->wallpapersList));

    ui->wallpapersList->setDragDropMode(QAbstractItemView::NoDragDrop);
    imageLoading_=QIcon::fromTheme("image-loading", QIcon(":/images/loading.png"));

    ui->pictures_location_comboBox->setItemText(0, gv.currentOSName+" "+tr("Desktop Backgrounds"));
    ui->pictures_location_comboBox->setItemData(0, gv.currentDeDefaultWallpapersPath, Qt::UserRole);
    ui->pictures_location_comboBox->setItemText(1, tr("My Pictures"));
    ui->pictures_location_comboBox->setItemData(1, gv.defaultPicturesLocation, Qt::UserRole);

    short size=settings->beginReadArray("pictures_locations");
    for(short i=2;i<size;i++){
        settings->setArrayIndex(i);
        QString folderPath=settings->value("item").toString();
        if(settings->value("type").toBool()){
            short setFolderCount=settings->beginReadArray(QString::number(i));
            QString allFoldersOfTheSet;
            for(short j=0;j<setFolderCount;j++){
                settings->setArrayIndex(j);
                allFoldersOfTheSet+=settings->value("item", QString()).toString();
                if(j!=setFolderCount-1){
                    allFoldersOfTheSet+="\n";
                }
            }
            settings->endArray();
            ui->pictures_location_comboBox->addItem(folderPath);
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, 1, Qt::UserRole+1);
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, folderPath, Qt::UserRole);
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, allFoldersOfTheSet, Qt::UserRole+2);
        }
        else
        {
            ui->pictures_location_comboBox->addItem(globalParser_->basenameOf(folderPath));
            ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, folderPath, Qt::UserRole);
        }
    }
    settings->endArray();

    short currentFolder = settings->value("currentFolder_index", 0).toInt();
    if(currentFolder>size)
        currentFolder=0;

    if(ui->pictures_location_comboBox->itemData(currentFolder, Qt::UserRole+1).toBool()){
        currentFolderIsAList_=true;
        if(atLeastOneFolderFromTheSetExists()){
            tempForDelayedPicturesLocationChange_=currentFolder;
            QTimer::singleShot(100, this, SLOT(delayed_pictures_location_change()));
        }
        else if(currentFolder==0){
            on_pictures_location_comboBox_currentIndexChanged(0);
        }
        else {
            ui->pictures_location_comboBox->setCurrentIndex(currentFolder);
        }
    }
    else{
        currentFolder_=ui->pictures_location_comboBox->itemData(currentFolder, Qt::UserRole).toString();
        if(!QDir(currentFolder_).exists())
        {
            tempForDelayedPicturesLocationChange_=currentFolder;
            QTimer::singleShot(100, this, SLOT(delayed_pictures_location_change()));
        }
        else if(currentFolder==0){
            on_pictures_location_comboBox_currentIndexChanged(0);
        }
        else {
            ui->pictures_location_comboBox->setCurrentIndex(currentFolder);
        }
    }

    gv.typeOfInterval=settings->value("typeOfIntervals", 0).toInt();
    ui->stackedWidget_2->setCurrentIndex(gv.typeOfInterval);

    if(settings->value("from_combo", 1).toInt()==0){
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt());
    }
    else if(settings->value("from_combo", 1).toInt()==1){
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt()/60);
    }
    else
    {
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt()/3600);
    }

    if(settings->value("till_combo", 1).toInt()==0){
        ui->random_to->setValue(settings->value("random_time_to", 20).toInt());
    }
    else if(settings->value("till_combo", 1).toInt()==1){
        ui->random_to->setValue(settings->value("random_time_to", 20).toInt()/60);
    }
    else
    {
        ui->random_to->setValue(settings->value("random_time_to", 20).toInt()/3600);
    }

    ui->days_spinBox->setValue(settings->value("days_box", 0).toInt());
    ui->hours_spinBox->setValue(settings->value("hours_box", 0).toInt());
    ui->minutes_spinBox->setValue(settings->value("minutes_box", 30).toInt());
    ui->seconds_spinBox->setValue(settings->value("seconds_box", 0).toInt());

    ui->random_time_from_combobox->setCurrentIndex(settings->value("from_combo", 1).toInt());
    ui->random_time_to_combobox->setCurrentIndex(settings->value("till_combo", 1).toInt());

    ui->timerSlider->setValue(settings->value("timeSlider", 7).toInt());
    if(ui->timerSlider->value()==7){
        on_timerSlider_valueChanged(7);
    }
    launchTimerToUpdateIcons();

    openCloseSearch_ = new QPropertyAnimation(ui->search_widget, "maximumHeight");
    connect(openCloseSearch_, SIGNAL(finished()), this, SLOT(openCloseSearchAnimationFinished()));
    openCloseSearch_->setDuration(GENERAL_ANIMATION_SPEED);

    loadedPages_[0]=true;
}

void MainWindow::loadLePage(){
    loadedPages_[1] = true;

    gv.leEnableTag = settings->value("le_enable_tag", false).toBool();

    ui->le_tag_checkbox->setChecked(gv.leEnableTag);
    ui->le_tag_button->setEnabled(gv.leEnableTag);
}

void MainWindow::loadPotdPage(){
    loadedPages_[2] = true;

    gv.potdIncludeDescription = settings->value("potd_include_description", true).toBool();
    gv.leEnableTag = settings->value("le_enable_tag", false).toBool();
    gv.potdDescriptionBottom = settings->value("potd_description_bottom", true).toBool();
    gv.potdDescriptionFont = settings->value("potd_description_font", "Ubuntu").toString();
    gv.potdDescriptionColor = settings->value("potd_text_color", "#FFFFFF").toString();
    gv.potdDescriptionBackgroundColor = settings->value("potd_background_color", "#000000").toString();
    gv.potdDescriptionRightMargin = settings->value("potd_description_right_margin", 0).toInt();
    gv.potdDescriptionBottomTopMargin = settings->value("potd_description_bottom_top_margin", 0).toInt();
#ifdef Q_OS_UNIX
    gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (gv.currentDE == DesktopEnvironment::UnityGnome ? 125 : 0)).toInt();
#else
    gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (0)).toInt();
#endif

    ui->label_5->setText("<span style=\"font-style:italic;\">"+tr("Style")+"</span><span style=\" font-weight:600; font-style:italic;\"> "+tr("Scale")+"</span><span style=\" font-style:italic;\"> "+tr("is highly recommended for this feature")+"</span>");
    ui->include_description_checkBox->setChecked(gv.potdIncludeDescription);
    ui->edit_potd->setEnabled(gv.potdIncludeDescription);
}

void MainWindow::loadWallpaperClocksPage()
{
    loadedPages_[3]=true;

    ui->install_clock->setIcon(QIcon::fromTheme("list-add", QIcon(":/images/list-add.png")));

    HideGrayLinesDelegate *delegate = new HideGrayLinesDelegate(ui->clocksTableWidget);
    ui->clocksTableWidget->setItemDelegate(delegate);

    ui->wallpaper_clocks_source->setText("<span style=\"font-style:italic;\">"+tr("Download wallpaper clocks from")+" </span><a href=\"http://www.vladstudio.com/wallpaperclocks/browse.php\"><span style=\"font-style:italic; text-decoration: underline; color: #ff5500;\">VladStudio.com</span></a>");
    ui->clocksTableWidget->setColumnWidth(0, 20);

    //add to the listwidget the wallpaper clocks that exist in ~/.wallch/Wallpaper_clocks (AppData/Local/Mellori Studio/Wallch/Wallpaper_clocks for Windows)
#ifdef Q_OS_UNIX
    unzipArchive_=NULL;
#endif
    Q_FOREACH(currentWallpaperClockPath_, globalParser_->listFolders(gv.wallchHomePath+"Wallpaper_clocks", true, false))
    {
        addClockToList();
    }

    short clocksCount=ui->clocksTableWidget->rowCount();

    for(short i=0;i<clocksCount;i++){
        if(ui->clocksTableWidget->item(i,2)->data(32)==gv.defaultWallpaperClock){
            ui->clocksTableWidget->selectRow(i);
            ui->clocksTableWidget->setFocus();
            break;
        }
    }

    ui->activate_clock->setEnabled(!gv.wallpaperClocksRunning && clocksCount!=0);

    if(!clocksCount)
    {
        gv.defaultWallpaperClock="None";
        settings->setValue("default_wallpaper_clock", gv.defaultWallpaperClock);
        settings->sync();
    }

    ui->month_checkBox->setChecked(settings->value("month",true).toBool());
    ui->day_month_checkBox->setChecked(settings->value("day_of_month",true).toBool());
    ui->day_week_checkBox->setChecked(settings->value("day_of_week",true).toBool());
    ui->am_checkBox->setChecked(settings->value("am_pm",true).toBool());
    ui->hour_checkBox->setChecked(settings->value("hour",true).toBool());
    ui->minutes_checkBox->setChecked(settings->value("minute",true).toBool());
}

void MainWindow::loadLiveWebsitePage(){
    //add login details information
    openCloseAddLogin_ = new QPropertyAnimation(ui->live_website_login_widget, "maximumHeight");
    connect(openCloseAddLogin_, SIGNAL(finished()), this, SLOT(openCloseAddLoginAnimationFinished()));
    openCloseAddLogin_->setDuration(GENERAL_ANIMATION_SPEED);
    gv.websiteWebpageToLoad=settings->value("website", "http://google.com").toString();
    gv.websiteInterval=settings->value("website_interval", 6).toInt();
    gv.websiteCropEnabled=settings->value("website_crop", false).toBool();
    gv.websiteCropArea=settings->value("website_crop_area", QRect(0, 0, gv.screenWidth, gv.screenHeight)).toRect();
    gv.websiteLoginEnabled=settings->value("website_login", false).toBool();
    gv.websiteLoginUsername=settings->value("website_username", "").toString();
    gv.websiteLoginPasswd=settings->value("website_password", "").toString();
    if(!gv.websiteLoginPasswd.isEmpty()){
        gv.websiteLoginPasswd=globalParser_->base64Decode(gv.websiteLoginPasswd);
    }
    gv.websiteRedirect=settings->value("website_redirect", false).toBool();
    gv.websiteFinalPageToLoad=settings->value("website_final_webpage", "").toString();
    gv.websiteSimpleAuthEnabled=settings->value("website_simple_auth", false).toBool();
    gv.websiteWaitAfterFinishSeconds=settings->value("website_wait_after_finish", 3).toInt();
    gv.websiteJavascriptEnabled=settings->value("website_js_enabled", true).toBool();
    gv.websiteJavascriptCanReadClipboard=settings->value("website_js_can_read_clipboard", false).toBool();
    gv.websiteJavaEnabled=settings->value("website_java_enabled", false).toBool();
    gv.websiteLoadImages=settings->value("website_load_images", true).toBool();
    gv.websiteExtraUsernames=settings->value("website_extra_usernames", QStringList()).toStringList();
    gv.websiteExtraPasswords=settings->value("website_extra_passwords", QStringList()).toStringList();

    ui->website->setText(gv.websiteWebpageToLoad);
    short old_website_slider_value=ui->website_slider->value();
    ui->website_slider->setValue(gv.websiteInterval);
    if(gv.websiteInterval==old_website_slider_value){
        on_website_slider_valueChanged(gv.websiteInterval);
    }
    ui->website_crop_checkbox->setChecked(gv.websiteCropEnabled);
    ui->edit_crop->setEnabled(ui->website_crop_checkbox->isEnabled() && gv.websiteCropEnabled);
    ui->add_login_details->setChecked(gv.websiteLoginEnabled);
    if(gv.websiteLoginEnabled){
        on_add_login_details_clicked(true);
    }
    ui->username->setText(gv.websiteLoginUsername);
    ui->password->setText(gv.websiteLoginPasswd);
    ui->redirect_checkBox->setChecked(gv.websiteRedirect);
    ui->final_webpage->setEnabled(gv.websiteRedirect);
    ui->final_webpage->setText(gv.websiteFinalPageToLoad);
    ui->timeout_text_label->hide();
    ui->website_timeout_label->hide();
    if(gv.previewImagesOnScreen)
    {
        ui->verticalLayout_11->addWidget(ui->timeout_text_label);
        ui->verticalLayout_11->addWidget(ui->website_timeout_label);
    }
    else
    {
        ui->horizontalLayout_23->addWidget(ui->timeout_text_label);
        ui->horizontalLayout_23->addWidget(ui->website_timeout_label);
    }

    changedWebPage_=initialWebPage_=ui->website->text();
    if(websiteSnapshot_==NULL){
        websiteSnapshot_ = new WebsiteSnapshot();
    }

    loadedPages_[4]=true;
}

void MainWindow::loadMelloriPage()
{
    loadedPages_[5]=true;

    ui->melloristudio_label->setText(tr("Wallch is developed by")+" <span style=\" font-weight:600; font-style:bold;\">Mellori Studio</span>.");
    ui->melloristudio_link_label->setText(tr("Learn more about our projects at:")+" <a href=\"http://melloristudio.com\"><span style=\" text-decoration: underline; color:#ff5500;\">melloristudio.com</span></a>");
}

void MainWindow::openCloseAddLoginAnimationFinished(){
    if(openCloseAddLogin_->endValue().toInt()==0){
        ui->live_website_login_widget->hide();
        this->repaint();
    }
}

void MainWindow::disableLiveWebsitePage(){
    ui->website_interval_slider_label->setEnabled(false);
    ui->label_11->setEnabled(false);
    ui->label_23->setEnabled(false);
    ui->website->setEnabled(false);
    ui->website_slider->setEnabled(false);
    ui->website_crop_checkbox->setEnabled(false);
    ui->edit_crop->setEnabled(false);
    ui->activate_website->setEnabled(false);
    ui->website_preview->setEnabled(false);
    ui->deactivate_website->setEnabled(false);
    ui->timeout_text_label->setEnabled(false);
    ui->website_timeout_label->setEnabled(false);
    ui->add_login_details->setEnabled(false);
    ui->live_website_login_widget->setEnabled(false);
}

void MainWindow::on_add_login_details_clicked(bool checked)
{
    if(gv.liveWebsiteRunning){
        ui->add_login_details->setChecked(!checked);
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" ("+tr("Live Website")+")");
        return;
    }

    openCloseAddLogin_->setStartValue(ui->live_website_login_widget->maximumHeight());
    if(!checked){
        openCloseAddLogin_->setEndValue(0);
    }
    else
    {
        ui->live_website_login_widget->show();
        openCloseAddLogin_->setEndValue(200);
    }
    openCloseAddLogin_->start();
}

void MainWindow::on_page_0_wallpapers_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==0){
        return;
    }

    if(!loadedPages_[0]){

        loadWallpapersPage();

        if(wallpaperManager_->wallpapersCount()<=1){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
    }
    launchTimerToUpdateIcons();
    on_timerSlider_valueChanged(ui->timerSlider->value());

    ui->page_0_wallpapers->setChecked(true);//in case it is called via the shortcut
    ui->stackedWidget->setCurrentIndex(0);
    ui->page_0_wallpapers->raise();
    ui->sep1->raise();
    if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }
}

void MainWindow::on_page_1_earth_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==1){
        return;
    }

    if(!loadedPages_[1]){
        loadLePage();
    }

    ui->page_1_earth->setChecked(true);
    ui->stackedWidget->setCurrentIndex(1);
    ui->page_1_earth->raise();
    ui->sep1->raise();
    ui->sep2->raise();

    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_2_potd_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==2){
        return;
    }

    if(!loadedPages_[2]){
        loadPotdPage();
    }

    ui->page_2_potd->setChecked(true);
    ui->stackedWidget->setCurrentIndex(2);
    ui->page_2_potd->raise();
    ui->sep2->raise();
    ui->sep3->raise();

    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_3_clock_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==3){
        return;
    }

    if(!loadedPages_[3]){
        loadWallpaperClocksPage();
    }

    ui->page_3_clock->setChecked(true);
    ui->stackedWidget->setCurrentIndex(3);
    ui->page_3_clock->raise();
    ui->sep3->raise();
    ui->sep4->raise();

    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_4_web_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==4){
        return;
    }

    if(!loadedPages_[4]){
        loadLiveWebsitePage();
    }

    ui->page_4_web->setChecked(true);
    ui->stackedWidget->setCurrentIndex(4);
    ui->page_4_web->raise();
    ui->sep4->raise();
    ui->sep5->raise();

    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_5_other_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==6){
        return;
    }

    if(!loadedPages_[5]){
        loadMelloriPage();
    }

    ui->page_5_other->setChecked(true);
    ui->stackedWidget->setCurrentIndex(5);
    ui->page_5_other->raise();
    ui->sep5->raise();
    ui->sep6->raise();
    if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }
}

void MainWindow::previousPage(){
    short currentPage = ui->stackedWidget->currentIndex()-1;
    if(currentPage<0){
        currentPage=6;
    }
    switch (currentPage){
    case 0:
        on_page_0_wallpapers_clicked();
        break;
    case 1:
        on_page_1_earth_clicked();
        break;
    case 2:
        on_page_2_potd_clicked();
        break;
    case 3:
        on_page_3_clock_clicked();
        break;
    case 4:
        on_page_4_web_clicked();
        break;
    case 5:
        on_page_5_other_clicked();
        break;
    default:
        on_page_0_wallpapers_clicked();
    }
}

void MainWindow::nextPage(){
    short currentPage = ui->stackedWidget->currentIndex()+1;
    if(currentPage>7){
        currentPage=0;
    }
    switch (currentPage){
    case 0:
        on_page_0_wallpapers_clicked();
        break;
    case 1:
        on_page_1_earth_clicked();
        break;
    case 2:
        on_page_2_potd_clicked();
        break;
    case 3:
        on_page_3_clock_clicked();
        break;
    case 4:
        on_page_4_web_clicked();
        break;
    case 5:
        on_page_5_other_clicked();
        break;
    default:
        on_page_0_wallpapers_clicked();
    }
}

//Actions code

void MainWindow::on_actionOpen_Image_triggered()
{
    if(!WallpaperManager::currentBackgroundExists())
        return;

    if(!QDesktopServices::openUrl(QUrl("file:///"+WallpaperManager::currentBackgroundWallpaper()))){
        globalParser_->error("I probably could not open "+WallpaperManager::currentBackgroundWallpaper());
    }
}

void MainWindow::on_actionOpen_Folder_triggered()
{
    if(WallpaperManager::currentBackgroundExists())
        wallpaperManager_->openFolderOf();
}

void MainWindow::on_actionCopy_Path_triggered()
{
    if(WallpaperManager::currentBackgroundExists())
        QApplication::clipboard()->setText(WallpaperManager::currentBackgroundWallpaper());
}

void MainWindow::on_actionCopy_Image_triggered()
{
    if(WallpaperManager::currentBackgroundExists())
        QApplication::clipboard()->setImage(QImage(WallpaperManager::currentBackgroundWallpaper()), QClipboard::Clipboard);
}

void MainWindow::on_actionDelete_triggered()
{
    if(!WallpaperManager::currentBackgroundExists())
        return;

    if(QMessageBox::question(0, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the current image?"))==QMessageBox::Yes)
    {
        if(!QFile::remove(WallpaperManager::currentBackgroundWallpaper())){
            QMessageBox::warning(0, tr("Error"), tr("There was a problem deleting the current image. Please make sure you have the permission to delete the image or that the image exists."));
        }
    }
}

void MainWindow::on_actionProperties_triggered()
{   
    if(propertiesShown_ || !WallpaperManager::currentBackgroundExists()){
        return;
    }
    QString imageFilename=WallpaperManager::currentBackgroundWallpaper();

    propertiesShown_=true;
    properties_ = new Properties(imageFilename, false, 0, 0);
    properties_->setModal(true);
    properties_->setAttribute(Qt::WA_DeleteOnClose);
    connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
    properties_->show();
}

void MainWindow::doQuit()
{
    actionsOnClose();
    globalParser_->debug("See ya next time!");
    attachedMemory_->detach();
    qApp->quit();
}

void MainWindow::on_actionContents_triggered()
{
    globalParser_->openUrl(HELP_URL);
}

void MainWindow::on_actionDonate_triggered()
{
    globalParser_->openUrl(DONATE_URL);
}

void MainWindow::on_actionDownload_triggered()
{
    globalParser_->openUrl("http://melloristudio.com/wallch/1000-HD-Wallpapers");
}

void MainWindow::on_actionReport_A_Bug_triggered()
{
    globalParser_->openUrl("https://bugs.launchpad.net/wallpaper-changer/+filebug");
}

void MainWindow::on_actionGet_Help_Online_triggered()
{
    globalParser_->openUrl("https://answers.launchpad.net/wallpaper-changer/+addquestion");
}

void MainWindow::on_actionWhat_is_my_screen_resolution_triggered()
{
    QMessageBox msgBox;msgBox.setWindowTitle(tr("What is my Screen Resolution"));
    msgBox.setText("<b>"+tr("Your screen resolution is")+" <span style=\" font-size:20pt;\">"+
                   QString::number(gv.screenWidth)+"</span><span style=\" font-size:15pt;\">x</span><span style=\" font-size:20pt;\">"+
                   QString::number(gv.screenHeight)+"</span>.</b><br>"+tr("The number above represents your screen/monitor resolution (In width and height)."));
    msgBox.setIconPixmap(QIcon(":/images/monitor.png").pixmap(QSize(100,100)));
    msgBox.setWindowIcon(QIcon(":/images/wallch.png"));
    msgBox.exec();
}

//Dialogs Code

void MainWindow::on_actionStatistics_triggered()
{
    if(statisticsShown_){
        return;
    }

    statisticsShown_=true;
    statistics_ = new Statistics(this);
    statistics_->setModal(true);
    statistics_->setAttribute(Qt::WA_DeleteOnClose);
    connect(statistics_, SIGNAL(destroyed()), this, SLOT(statisticsDestroyed()));
    statistics_->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    statistics_->show();
}

void MainWindow::statisticsDestroyed(){
    statisticsShown_=false;
}

void MainWindow::on_actionHistory_triggered()
{
    if(historyShown_){
        return;
    }
    historyShown_=true;
    history_ = new History (this);
    history_->setModal(true);
    history_->setAttribute(Qt::WA_DeleteOnClose);
    connect(history_, SIGNAL(destroyed()), this, SLOT(historyDestroyed()));
    history_->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    history_->show();
    history_->activateWindow();
}

void MainWindow::historyDestroyed()
{
    historyShown_=false;
}

void MainWindow::on_action_About_triggered()
{
    if(aboutShown_){
        return;
    }
    aboutShown_=true;
    about_ = new About(this);
    about_->setModal(true);
    about_->setAttribute(Qt::WA_DeleteOnClose);
    connect(about_, SIGNAL(destroyed()), this, SLOT(aboutDestroyed()));
    about_->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    about_->show();
    about_->activateWindow();
}

void MainWindow::aboutDestroyed(){
    aboutShown_=false;
}

void MainWindow::on_action_Preferences_triggered()
{
    if(gv.preferencesDialogShown){
        return;
    }
    gv.preferencesDialogShown=true;
    preferences_ = new Preferences(this);
    preferences_->setModal(true);
    preferences_->setAttribute(Qt::WA_DeleteOnClose);
    connect(preferences_, SIGNAL(destroyed()), this, SLOT(preferencesDestroyed()));
    connect(preferences_, SIGNAL(changePathsToIcons()), this, SLOT(changePathsToIcons()));
    connect(preferences_, SIGNAL(changeIconsToPaths()), this, SLOT(changeIconsToPaths()));
    connect(preferences_, SIGNAL(researchFolders()), this, SLOT(researchFolders()));
    connect(preferences_, SIGNAL(previewChanged()), this, SLOT(wait_preview_changed()));
    connect(preferences_, SIGNAL(intervalTypeChanged()), this, SLOT(intervalTypeChanged()));
    connect(preferences_, SIGNAL(changeThemeTo(QString)), this, SLOT(changeCurrentThemeTo(const QString&)));
    connect(preferences_, SIGNAL(maxCacheChanged(qint64)), cacheManager_, SLOT(setMaxCache(qint64)));
#ifdef Q_OS_UNIX
    connect(preferences_, SIGNAL(unityProgressbarChanged(bool)), this, SLOT(unityProgressbarSetEnabled(bool)));
#endif //#ifdef Q_OS_UNIX
    preferences_->setWindowFlags(Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    preferences_->show();
    preferences_->activateWindow();
}

void MainWindow::preferencesDestroyed(){
    gv.preferencesDialogShown = false;
}

void MainWindow::on_set_desktop_color_clicked()
{
    if(colorsGradientsShown_){
        return;
    }
    colorsGradientsShown_=true;
    colorsGradients_ = new ColorsGradients(this);

    colorsGradients_->setModal(true);
    colorsGradients_->setAttribute(Qt::WA_DeleteOnClose);
    colorsGradients_->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    connect(colorsGradients_, SIGNAL(destroyed()), this, SLOT(colorsGradientsDestroyed()));
    connect(colorsGradients_, SIGNAL(updateTv()), this, SLOT(updateScreenLabel()));
    connect(colorsGradients_, SIGNAL(updateDesktopColor(QString)), this, SLOT(setButtonColor(const QString&)));
    connect(colorsGradients_, SIGNAL(updateColorButtonSignal(QImage)), this, SLOT(updateColorButton(QImage)));
    colorsGradients_->show();
    colorsGradients_->activateWindow();
}

void MainWindow::colorsGradientsDestroyed()
{
    colorsGradientsShown_=false;
}

void MainWindow::on_potd_viewer_Button_clicked()
{
    if(potdViewerShown_){
        return;
    }
    potdViewerShown_=true;
    potdViewer_ = new PotdViewer(this);
    potdViewer_->setModal(true);
    potdViewer_->setAttribute(Qt::WA_DeleteOnClose);
    connect(potdViewer_, SIGNAL(destroyed()), this, SLOT(potdViewerDestroyed()));
    potdViewer_->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    potdViewer_->show();

}

void MainWindow::potdViewerDestroyed()
{
    potdViewerShown_=false;
}

void MainWindow::showProperties()
{
    if(propertiesShown_ || !ui->wallpapersList->currentItem()->isSelected() || ui->stackedWidget->currentIndex()!=0){
        return;
    }

    QString currentImage = getPathOfListItem();

    if(WallpaperManager::imageIsNull(currentImage))
    {
        QMessageBox::warning(this, tr("Properties"), tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
        return;
    }
    else
    {
        propertiesShown_=true;
        properties_ = new Properties(currentImage, true, ui->wallpapersList->currentRow(), this);

        properties_->setModal(true);
        properties_->setAttribute(Qt::WA_DeleteOnClose);
        connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
        connect(properties_, SIGNAL(requestNext(int)), this, SLOT(sendPropertiesNext(int)));
        connect(properties_, SIGNAL(requestPrevious(int)), this, SLOT(sendPropertiesPrevious(int)));
        connect(properties_, SIGNAL(averageColorChanged()), this, SLOT(setButtonColor()));
        connect(this, SIGNAL(givePropertiesRequest(QString, int)), properties_, SLOT(updateEntries(QString, int)));
        properties_->show();
        properties_->activateWindow();
    }
}

void MainWindow::propertiesDestroyed(){
    propertiesShown_=false;
}

void MainWindow::sendPropertiesNext(int current){
    int listCount=ui->wallpapersList->count();
    if(current>=(listCount-1)){
        if(listCount){
            Q_EMIT givePropertiesRequest(getPathOfListItem(0), 0);
        }
    }
    else
    {
        Q_EMIT givePropertiesRequest(getPathOfListItem(current+1), current+1);
    }
}

void MainWindow::sendPropertiesPrevious(int current){
    int listCount = ui->wallpapersList->count();
    if(current == 0){
        if(listCount){
            Q_EMIT givePropertiesRequest(getPathOfListItem(listCount-1), listCount-1);
        }
    }
    else
    {
        Q_EMIT givePropertiesRequest(getPathOfListItem(current-1), current-1);
    }
}

void MainWindow::on_include_description_checkBox_clicked(bool checked)
{
    gv.potdIncludeDescription = checked;
    ui->edit_potd->setEnabled(checked);
    settings->setValue("potd_include_description", checked);
    settings->sync();

    restartPotdIfRunningAfterSettingChange();
}

void MainWindow::on_edit_potd_clicked()
{
    if(potdPreviewShown_){
        return;
    }

    potdPreviewShown_=true;
    potdPreview_ = new PotdPreview(this);
    potdPreview_->setModal(true);
    potdPreview_->setAttribute(Qt::WA_DeleteOnClose);
    connect(potdPreview_, SIGNAL(potdPreferencesChanged()), this, SLOT(restartPotdIfRunningAfterSettingChange()));
    connect(potdPreview_, SIGNAL(destroyed()), this, SLOT(potdPreviewDestroyed()));
    potdPreview_->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    potdPreview_->show();
}

void MainWindow::potdPreviewDestroyed()
{
    potdPreviewShown_=false;
}

//functions only to save settings
void MainWindow::on_shuffle_images_checkbox_clicked()
{
    settings->setValue("random_images_enabled", ui->shuffle_images_checkbox->isChecked());
}

void MainWindow::on_stackedWidget_currentChanged(int page)
{
    if(page==0)
        ui->help_label->setText("Images stored in your local hard drive");
    else if(page==1)
        ui->help_label->setText("A \"live\" image of the sunlight and the clouds at earth, updating every ½ hour");
    else if(page==2)
        ui->help_label->setText("Outstanding photos that are selected daily from Wikipedia");
    else if(page==3)
        ui->help_label->setText("Wallpapers choosen from VladStudio that show the time and date");
    else if(page==4)
        ui->help_label->setText("Screenshot of a website");
    else
        ui->help_label->setText("About Us");

    settings->setValue("current_page" , page);
}

void MainWindow::update_website_settings()
{
    if(!loadedPages_[4])
        return;

    gv.websiteWebpageToLoad=ui->website->text();
    gv.websiteInterval=ui->website_slider->value();
    gv.websiteCropEnabled=ui->website_crop_checkbox->isChecked();
    settings->setValue("website", gv.websiteWebpageToLoad);
    settings->setValue("website_interval", gv.websiteInterval);
    settings->setValue("website_crop", gv.websiteCropEnabled);
    settings->setValue("website_login", ui->add_login_details->isChecked());
    settings->setValue("website_username", ui->username->text());
    settings->setValue("website_password", base64Encode(ui->password->text()));
    settings->setValue("website_final_webpage", ui->final_webpage->text());
    settings->setValue("website_redirect", ui->redirect_checkBox->isChecked());
    settings->sync();
}

void MainWindow::on_donateButton_clicked()
{
    globalParser_->openUrl(DONATE_URL);
}

void MainWindow::on_melloristudio_link_label_linkActivated(const QString &link)
{
    globalParser_->openUrl(link);
}

void MainWindow::on_donateButton_pressed()
{
    ui->donateButton->setIcon(QIcon(":/images/donate_pressed.png"));
}

void MainWindow::on_donateButton_released()
{
    ui->donateButton->setIcon(QIcon(":/images/donate.png"));
}

// Animations in case user changes preview screen to show/hide
void MainWindow::wait_preview_changed()
{
    QTimer::singleShot(100, this, SLOT(preview_changed()));
}

void MainWindow::preview_changed()
{
    rightWidgetAnimation_ = new QPropertyAnimation(ui->right_widget, "maximumWidth");
    rightWidgetAnimation_->setDuration(350);
    rightWidgetAnimation_->setStartValue(ui->right_widget->maximumWidth());

    if(gv.previewImagesOnScreen)
    {
        ui->right_widget->show();
        ui->screen_label->clear();
        ui->screen_label_text->clear();
        bottomWidgetsAnimation();
        QTimer::singleShot(300, this, SLOT(showPreview()));
        rightWidgetAnimation_->setEndValue(343);
    }
    else
    {
        ui->screen_label->clear();
        ui->screen_label_transition->clear();
        QTimer::singleShot(400, this, SLOT(hidePreview()));
        rightWidgetAnimation_->setEndValue(0);
        rightWidgetAnimation_->start();
    }
}

void MainWindow::hidePreview()
{
    ui->right_widget->hide();
    delete rightWidgetAnimation_;
    ui->timeForNext->setMinimumWidth(300);
    bottomWidgetsAnimation();
    QTimer::singleShot(260, this, SLOT(launchTimerToUpdateIcons()));
    if(loadedPages_[4])
    {
        ui->horizontalLayout_23->addWidget(ui->timeout_text_label);
        ui->horizontalLayout_23->addWidget(ui->website_timeout_label);
    }
    launchTimerToUpdateIcons();
}

void MainWindow::showPreview()
{
    ui->horizontalLayout_preview_on->addWidget(ui->set_desktop_color);
    ui->horizontalLayout_preview_on->addWidget(ui->image_style_combo);
    ui->horizontalLayout_progressbar->addWidget(ui->timeForNext);
    rightWidgetAnimation_->start();
    ui->timeForNext->setMinimumWidth(0);
    if(loadedPages_[4])
    {
        ui->verticalLayout_11->addWidget(ui->timeout_text_label);
        ui->verticalLayout_11->addWidget(ui->website_timeout_label);
    }
    updateScreenLabel();
    launchTimerToUpdateIcons();
}

void MainWindow::bottomWidgetsAnimation()
{
    widget3Animation_ = new QPropertyAnimation(ui->widget_3, "maximumHeight");
    widget3Animation_->setDuration(300);
    widget3Animation_->setStartValue(ui->widget_3->maximumHeight());
    if(!gv.previewImagesOnScreen)
    {
        ui->widget_3->show();
        ui->horizontalLayout_preview_off->addWidget(ui->set_desktop_color);
        ui->horizontalLayout_preview_off->addWidget(ui->image_style_combo);
        ui->horizontalLayout_preview_off->addWidget(ui->timeForNext);
        widget3Animation_->setEndValue(40);
    }
    else
    {
        connect(widget3Animation_, SIGNAL(finished()), this, SLOT(removeBottomwidgets()));
        widget3Animation_->setEndValue(0);
    }
    widget3Animation_->start();
}

void MainWindow::removeBottomwidgets()
{
    ui->widget_3->hide();
    delete widget3Animation_;
}

//Function to recieve Windows system messages.
//Needed to recieve signals if user plugs/unplugs laptop, AND SOON I WILL IMPLEMENT KEYBINDINGS HERE (stop shouting, leon)
//This is much better than the timer way, because qt recieves these messages with or without this function.

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
   (void)eventType;
   (void)result;
   MSG* msg = (MSG*)(message);
   if(msg->message == WM_POWERBROADCAST && gv.pauseOnBattery)
   {
       if(globalParser_->runsOnBattery())
       {
           //Just to note, when we plug the cable, Windows for some reason sends discharging,
           //and after 1 second sends charging so just ignore first discharging message
           bool array[5] = {gv.wallpapersRunning, gv.liveEarthRunning, gv.potdRunning, gv.wallpaperClocksRunning, gv.liveWebsiteRunning};

           for(int i=0;i<5;i++){
               if(array[i]){
                   previouslyRunningFeature_=i;
                   pauseEverythingThatsRunning();
                   stoppedBecauseOnBattery_=true;
                   break;
               }
           }
       }
       else
       {
           if(stoppedBecauseOnBattery_)
           {
               switch(previouslyRunningFeature_){
               default:
               case 0:
                   on_startButton_clicked();
                   break;
               case 1:
                   on_activate_livearth_clicked();
                   break;
               case 2:
                   on_activate_potd_clicked();
                   break;
               case 3:
                   on_activate_clock_clicked();
                   break;
               case 4:
                   on_activate_website_clicked();
                   break;
               }

               previouslyRunningFeature_=0;
               stoppedBecauseOnBattery_=false;
           }
       }
   }
   //else if(msg->message == WM_KEYDOWN)
   return false;
}
#endif

void MainWindow::intervalTypeChanged()
{
    gv.typeOfInterval=settings->value("typeOfIntervals", 0).toInt();
    ui->stackedWidget_2->setCurrentIndex(gv.typeOfInterval);
    if(gv.wallpapersRunning && gv.randomImagesEnabled)
    {
        initialRandomSeconds_=totalSeconds_;
    }
}

void MainWindow::on_random_from_valueChanged(int value)
{
    if(!loadedPages_[0])
        return;

    short combo_from=ui->random_time_from_combobox->currentIndex();
    short combo_till=ui->random_time_to_combobox->currentIndex();
    short value2=ui->random_to->value();

    if(combo_from==0 && value==0)
    {
        ui->random_from->setValue(1);
    }

    if(combo_from==0 && combo_till==0 && value>=value2-2){
        ui->random_to->setValue(value+3);
    }
    else if(((combo_from==1 && combo_till==1) || (combo_from==2 && combo_till==2)) && value>=value2){
        ui->random_to->setValue(value+1);
    }

    gv.randomTimeFrom = ui->random_from->value() * pow(60, ui->random_time_from_combobox->currentIndex());
    settings->setValue("random_time_from", gv.randomTimeFrom);
    settings->sync();
}

void MainWindow::on_random_to_valueChanged(int value2)
{
    if(!loadedPages_[0])
        return;

    short combo_from=ui->random_time_from_combobox->currentIndex();
    short combo_till=ui->random_time_to_combobox->currentIndex();
    short value=ui->random_from->value();

    if(combo_from==0 && combo_till==0 && value>=value2-2){
        if(value==1){
            ui->random_to->setValue(4);
            return;
        }
        ui->random_from->setValue(value2-3);
    }
    else if(((combo_from==1 && combo_till==1) || (combo_from==2 && combo_till==2)) && value>=value2){
        ui->random_from->setValue(value2-1);
    }
    gv.randomTimeTo=ui->random_to->value() * pow(60, ui->random_time_to_combobox->currentIndex());
    settings->setValue("random_time_to", gv.randomTimeTo);
    settings->sync();
}

void MainWindow::on_random_time_from_combobox_currentIndexChanged(int index)
{
    if(!loadedPages_[0])
        return;

    if(ui->random_time_to_combobox->currentIndex()<index){
        ui->random_time_to_combobox->setCurrentIndex(index);
    }
    settings->setValue("from_combo", ui->random_time_from_combobox->currentIndex());
    on_random_from_valueChanged(ui->random_from->value());
}

void MainWindow::on_random_time_to_combobox_currentIndexChanged(int index)
{
    if(!loadedPages_[0])
        return;

    if(ui->random_time_from_combobox->currentIndex()>index){
        ui->random_time_from_combobox->setCurrentIndex(index);
    }
    settings->setValue("till_combo", ui->random_time_to_combobox->currentIndex());
    on_random_to_valueChanged(ui->random_to->value());
}

void MainWindow::timeSpinboxChanged()
{
    int delay=ui->days_spinBox->value()*86400+ui->hours_spinBox->value()*3600+ui->minutes_spinBox->value()*60+ui->seconds_spinBox->value();
    if(delay==0)
    {
        delay=1;
        ui->seconds_spinBox->setValue(1);
    }
    findSeconds(false);
    updateCheckTime_->start(3000);
    settings->setValue("delay", delay);
    settings->sync();
}

void MainWindow::on_edit_pushButton_clicked()
{
    if(locationsShown_){
        return;
    }
    locationsShown_=true;
    locations_ = new PicturesLocations(this);
    locations_->setModal(true);
    locations_->setAttribute(Qt::WA_DeleteOnClose);
    connect(locations_, SIGNAL(destroyed()), this, SLOT(locationsDestroyed()));
    connect(locations_, SIGNAL(picturesLocationsChanged()), this, SLOT(picturesLocationsChanged()));
    locations_->setWindowFlags(Qt::Window);
    locations_->show();
    locations_->activateWindow();
}

void MainWindow::locationsDestroyed(){
    locationsShown_=false;
}
