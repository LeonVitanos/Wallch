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
#include <QShortcut>
#include <QScrollBar>
#include <QStandardItem>
#include <QColorDialog>

#ifdef ON_WIN32
#include <QtGui/private/qzipreader_p.h>
#endif

#include "mainwindow.h"

#include "math.h"

MainWindow *mainWindowInstance;

MainWindow::MainWindow(Global *globalParser, WebsiteSnapshot *websiteSnapshotP, int timeout_count, QStringList previous_pictures_from_main, int last_random_delay, int last_normal_picture, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWindowInstance=this;

    if(globalParser==NULL){
        globalParser_ = new Global(true);
    }
    else
    {
        globalParser_=globalParser;
    }
    connect(globalParser_, SIGNAL(onlineRequestFailed()), this, SLOT(onlineRequestFailed()));
    connect(globalParser_, SIGNAL(onlineImageRequestReady(QString)), this, SLOT(onlineImageRequestReady(QString)));

    if(websiteSnapshotP!=NULL){
        websiteSnapshot_=websiteSnapshotP;
    }
    else
    {
        websiteSnapshot_=NULL;
    }

    QDesktopWidget dim;
    gv.screenWidth=dim.width();
    gv.screenHeight=dim.height();

    imagePreviewResizeFactorX_ = SCREEN_LABEL_SIZE_X*2.0/(gv.screenWidth*1.0);
    imagePreviewResizeFactorY_ = SCREEN_LABEL_SIZE_Y*2.0/(gv.screenHeight*1.0);

    //Moving the window to the center of screen!
    this->move(QApplication::desktop()->availableGeometry().center() - this->rect().center());
    this->resize(QSize(this->width(),this->minimumHeight()));

    gv.potdOnlineUrl=settings->value("potd_online_url", POTD_ONLINE_URL).toString();
    gv.potdOnlineUrlB=settings->value("potd_online_urlB", POTD_ONLINE_URL_B).toString();
    gv.liveEarthOnlineUrl=settings->value("line_earth_online_url", LIVEARTH_ONLINE_URL).toString();
    gv.liveEarthOnlineUrlB=settings->value("live_earth_online_urlB", LIVEARTH_ONLINE_URL_B).toString();
    gv.potdIncludeDescription = settings->value("potd_include_description", true).toBool();
    gv.potdDescriptionBottom = settings->value("potd_description_bottom", true).toBool();
    gv.potdDescriptionFont = settings->value("potd_description_font", "Ubuntu").toString();
    gv.potdDescriptionColor = settings->value("potd_text_color", "#FFFFFF").toString();
    gv.potdDescriptionBackgroundColor = settings->value("potd_background_color", "#000000").toString();
#ifdef ON_LINUX
    gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (gv.currentDE==UnityGnome ? 125 : 0)).toInt();
#else
    gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (0)).toInt();
#endif
    gv.potdDescriptionRightMargin = settings->value("potd_description_right_margin", 0).toInt();
    gv.potdDescriptionBottomTopMargin = settings->value("potd_description_bottom_top_margin", 0).toInt();

    on_include_description_checkBox_clicked(gv.potdIncludeDescription);

    gv.iconMode=settings->value("icon_style", true).toBool();

    if(gv.iconMode){
        ui->listWidget->setIconSize(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->listWidget->setUniformItemSizes(true);
    }
    else
    {
        ui->listWidget->setViewMode(QListView::ListMode);
    }

    connect(&futureWatcherWallClocksPreview_, SIGNAL(finished()), this, SLOT(wallpaperClocksPreviewImageGenerationFinished()));
    connect(&futureWatcherWallpapersPreview_, SIGNAL(finished()), this, SLOT(wallpapersPreviewImageGenerationFinished()));

    resetWatchFolders();
    setupTimers();
    connectSignalSlots();
    setupKeyboardShortcuts();
    setupThemeFallbacks();
    changeCurrentThemeTo(gv.currentTheme);

    clocksRadioButtonsGroup = new QButtonGroup(this);

    //setting the slider's value...
    secondsInWallpapersSlider_=settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();
    gv.defaultWallpaperClock=settings->value("default_wallpaper_clock", "").toString();
    gv.randomImagesEnabled=settings->value("random_images_enabled", false).toBool();

    timeForNext_ = new QProgressBar(this);
    timeForNext_->setMaximumWidth(290);

    opacityEffect_ = new QGraphicsOpacityEffect(this);
    opacityEffect2_ = new QGraphicsOpacityEffect(this);

    gv.previewImagesOnScreen=settings->value("preview_images_on_screen", true).toBool();
    if(!gv.previewImagesOnScreen)
    {
        ui->preview_widget->setMinimumHeight(0);
        ui->preview_widget->hide();
    }

    gv.independentIntervalEnabled=settings->value("independent_interval_enabled", true).toBool();

    gv.rotateImages=settings->value("rotation", false).toBool();
    gv.firstTimeout=settings->value("first_timeout", false).toBool();
    gv.showNotification=settings->value("notification", false).toBool();
    gv.saveHistory = settings->value("history", true).toBool();
    gv.setAverageColor = settings->value("average_color", false).toBool();
#ifdef ON_LINUX
    gv.unityProgressbarEnabled=settings->value("unity_progressbar_enabled", true).toBool();
#endif
    gv.randomTimeEnabled=settings->value("random_time_enabled", false).toBool();
    gv.randomTimeFrom=settings->value("random_time_from", 300).toInt();
    gv.randomTimeTo=settings->value("random_time_to", 1200).toInt();
    gv.useShortcutNext=settings->value("use_shortcut_next", false).toBool();
    if(gv.randomTimeFrom>gv.randomTimeTo-3){
        Global::error("The randomness is misconfigured, please head to the Preferences dialog to configure it properly!");
        gv.randomTimeEnabled=false;
    }

    if(gv.randomImagesEnabled){
        srand(time(0));
    }

    if(gv.wallpapersRunning)
    {
        loadWallpapersPage();

        if(ui->listWidget->count() < LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else{
            startButtonsSetEnabled(true);
        }

        if(gv.processPaused){
            actAsStart_=true;
            ui->startButton->setText(tr("&Start"));
            ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/Pictures/media-playback-start.svg")));
            animateProgressbarOpacity(1);
            findSeconds(true);
            secondsLeft_=(timeout_count+1);
            indexOfCurrentImage_=last_normal_picture;
            initialRandomSeconds_=last_random_delay;
            Global::resetSleepProtection(secondsLeft_);
            updateSeconds();
            stopButtonsSetEnabled(true);
#ifdef ON_LINUX
            if(gv.currentDE==UnityGnome){
                dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
            }
#endif
            previousAndNextButtonsSetEnabled(false);
            timeForNext_->setFormat(timeForNext_->format()+" - Paused.");
        }
        else
        {
            //starting the process...
            actAsStart_=false;
            ui->startButton->setText(tr("Pau&se"));
            ui->startButton->setIcon(QIcon::fromTheme("media-playback-pause", QIcon(":/icons/Pictures/media-playback-pause.svg")));
            animateProgressbarOpacity(1);
            findSeconds(true);
            startWasJustClicked_=true;
            secondsLeft_=timeout_count;
            indexOfCurrentImage_=last_normal_picture;
            initialRandomSeconds_=last_random_delay;
            startUpdateSeconds();
            stopButtonsSetEnabled(true);
            previousAndNextButtonsSetEnabled(true);
        }
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if(gv.liveEarthRunning)
    {
        secondsLeft_=timeout_count;
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
        totalSeconds_=secondsLeft_=timeout_count;
        startUpdateSeconds();
        setProgressbarsValue(100);
        on_page_3_clock_clicked();
        animateProgressbarOpacity(1);
    }
    else if(gv.liveWebsiteRunning)
    {
        secondsLeft_=timeout_count;
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
            case  3:
                on_page_3_clock_clicked();
                break;
            case 4:
                on_page_4_web_clicked();
                break;
        }

        //the app has opened normally, so there is no point in keeping a previous independent interval
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(-1, 0);
        }
    }

    //in case there were any previous pictures in main (from the --start argument)...
    previousBackgrounds_=previous_pictures_from_main;

    ui->shuffle_images_checkbox->setChecked(gv.randomImagesEnabled);
    if(gv.randomTimeEnabled){
        ui->timerSlider->setEnabled(false);
        ui->wallpapers_slider_time->setEnabled(false);
        ui->interval_label->setEnabled(false);
    }

    if(gv.currentDE==LXDE){
        ui->set_desktop_color->hide();
    }

    //enable shortcut key
    if(gv.useShortcutNext){
        gv.nextShortcut=settings->value("shortcut", "").toString();
#ifdef ON_LINUX
        keybinder_init();
        bindKey(gv.nextShortcut);
#endif
    }

    //enabling dragandrop at the whole program
    setAcceptDrops(true);

    connect(ui->listWidget->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(launchTimerToUpdateIcons()));

    ui->statusBar->addPermanentWidget(timeForNext_, 0);

    refreshIntervals();

    ui->label_3->setText("<html><head/><body><p>"+tr("Picture of the day is being chosen from")+" <a href=\"en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day\"><span style=\" text-decoration: underline; color:#ff5500;\">Wikipedia</span></a></p></body></html>");
    ui->melloristudio_label->setText("<html><head/><body><p>"+tr("Wallch is developed by")+" <span style=\" font-weight:600; font-style:bold;\">Mellori Studio</span>.</span></p></body></html>");
    ui->melloristudio_link_label->setText("<html><head/><body><p>"+tr("Learn more about our projects at:")+" <a href=\"http://melloristudio.com\"><span style=\" text-decoration: underline; color:#ff5500;\">melloristudio.com</span></a></p></body></html>");
    ui->label_5->setText("<html><head/><body><p><span style=\" font-style:italic;\">"+tr("Style")+"</span><span style=\" font-weight:600; font-style:italic;\"> "+tr("Scale")+"</span><span style=\" font-style:italic;\"> "+tr("is highly recommended for this feature")+"</span></p></body></html>");
    ui->label_9->setText("<html><head/><body><p><span style=\" font-style:italic;\">"+tr("Download wallpaper clocks from")+" </span><a href=\"http://www.vladstudio.com/wallpaperclocks/browse.php\"><span style=\" font-style:italic; text-decoration: underline; color:#ff5500;\">VladStudio.com</span></a><span style=\" font-style:italic;\"></span></p></body></html>");

    processingOnlineRequest_=false;

    //processing request (loading gif)
    ui->process_request_label->hide();
    processingRequestGif_ = new QMovie(this);
    processingRequestGif_->setFileName(":/icons/Pictures/process_request.gif");
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
    event->ignore();
}

void MainWindow::actionsOnClose()
{
    appAboutToClose_=true;
    this->hide();
#ifdef ON_LINUX
    app_indicator_set_status(gv.applicationIndicator, APP_INDICATOR_STATUS_PASSIVE);
#endif
}

void MainWindow::changeEvent(QEvent *e)
{
    if(gv.preferencesDialogShown || addDialogShown_ || historyShown_){
        this->showNormal();
    }
    switch (e->type()){
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    case QEvent::WindowStateChange:
        if(isMinimized()){
            this->hide();
        }
        break;

    default:
        break;
    }
    QMainWindow::changeEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e){
    if(!gv.mainwindowLoaded){
        return;
    }
    if(!ui->stackedWidget->currentIndex()){
        launchTimerToUpdateIcons();
    }

    //screen preview is not getting updated when we resize the window, so we do it manually
    if(ui->screen_label->pixmap())
    {
        QPixmap *pixmap = new QPixmap(*ui->screen_label->pixmap());
        if(gv.previewImagesOnScreen && gv.mainwindowLoaded)
        {
            if(pixmap){
                ui->screen_label_transition->setPixmap(*pixmap);
            }
            else{
                ui->screen_label_transition->clear();
            }

            ui->screen_label->setPixmap(*pixmap);
        }
    }
    else
    {
        QString temp=ui->screen_label_text->text();
        ui->screen_label_text->clear();
        ui->screen_label_text->setText(temp);
    }

    QMainWindow::resizeEvent(e);
    if(this->width()<this->minimumWidth()){
        QTimer::singleShot(100, this, SLOT(wrongSizeOfWindow()));
    }
}

void MainWindow::wrongSizeOfWindow()
{
    this->resize(QSize(this->minimumWidth(),this->height()));
}

void MainWindow::strongShowApp(){
#ifdef ON_WIN32
    this->showNormal();
    this->activateWindow();
#else
    this->show();
    this->activateWindow();

    Qt::WindowFlags eFlags = this->windowFlags();
    eFlags |= Qt::WindowStaysOnTopHint;
    this->setWindowFlags(eFlags);
    this->show();
    eFlags &= ~Qt::WindowStaysOnTopHint;
    this->setWindowFlags(eFlags);
    this->show();
#endif
}


void MainWindow::strongMinimize(){
    this->setWindowState(Qt::WindowMaximized); //dirty trick
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::setupTimers(){
    //Timer to updates progressbar and launch the changing process
    updateSecondsTimer_ = new QTimer(this);
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

    cacheSizeChecker_ = new QTimer(this);
    connect(cacheSizeChecker_, SIGNAL(timeout()), this, SLOT(fixCacheSizeThreaded()));
    cacheSizeChecker_->setSingleShot(true);

    //Timer to hide progressbar
    hideProgress_ = new QTimer(this);
    connect(hideProgress_, SIGNAL(timeout()), this, SLOT(hideTimeForNext()));
    hideProgress_->setSingleShot(true);
#ifdef ON_LINUX
    indicatorChangeNormalTimer_ = new QTimer(this);
    connect(indicatorChangeNormalTimer_, SIGNAL(timeout()), this, SLOT(indicatorSetNormal()));
    indicatorChangeNormalTimer_->setSingleShot(true);
#endif
}

void MainWindow::setupThemeFallbacks(){
    //File Menubar
    ui->actionQuit_Ctrl_Q->setIcon(QIcon::fromTheme("application-exit", QIcon(":/icons/Pictures/application-exit.svg")));

    //Extras Menubar
    ui->actionHistory->setIcon(QIcon::fromTheme("task-due", QIcon(":/icons/Pictures/task-due.svg")));
    ui->action_Preferences->setIcon(QIcon::fromTheme("preferences-desktop", QIcon(":/icons/Pictures/preferences-desktop.svg")));

    //Wallpapers page
    ui->previous_Button->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/icons/Pictures/media-seek-backward.svg")));
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/Pictures/media-playback-start.svg")));
    ui->stopButton->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/icons/Pictures/media-playback-stop.svg")));
    ui->next_Button->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/icons/Pictures/media-seek-forward.svg")));
    ui->search_close->setIcon(QIcon::fromTheme("window-close", QIcon(":/icons/Pictures/window-close.svg")));
    ui->search_down->setIcon(QIcon::fromTheme("go-down", QIcon(":/icons/Pictures/go-down.svg")));
    ui->search_up->setIcon(QIcon::fromTheme("go-up", QIcon(":/icons/Pictures/go-up.svg")));

    //Wallpaper clocks page
    ui->install_clock->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/Pictures/list-add.svg")));

    //Live website page
    ui->edit_crop->setIcon(QIcon::fromTheme("applications-accessories", QIcon(":/icons/Pictures/applications-accessories.svg")));
}

void MainWindow::connectSignalSlots(){
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
    (void) new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(showHideSearchBox()));
    (void) new QShortcut(Qt::Key_Return, this, SLOT(enterPressed()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousPage()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextPage()));
    (void) new QShortcut(Qt::ALT + Qt::Key_F4, this, SLOT(escapePressed()));
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

void MainWindow::onlineRequestFailed(){
    bool somethingWasRunning=processingOnlineRequest_;
    closeWhatsRunning();
    processRequestStop();
    if(somethingWasRunning)
    {
        QMessageBox::warning(this, tr("Internet Connection"), tr("There was a problem while trying to download what you requested. Please check your internet connection."));
    }
}

void MainWindow::onlineImageRequestReady(QString image){
    imageTransition(image);
    processRequestStop();
}

void MainWindow::escapePressed(){
    if(ui->stackedWidget->currentIndex()==0 && ui->search_box->hasFocus() && searchIsOn_){
        on_search_close_clicked();
    }
    else{
        strongMinimize();
    }
}

void MainWindow::deletePressed(){
    if(ui->stackedWidget->currentIndex()==0 && ui->listWidget->selectedItems().count()>0){
        if(ui->listWidget->selectedItems().count()>1){
            removeImagesFromDisk();
        }
        else{
            removeImageFromDisk();
        }
    }
    else if(ui->stackedWidget->currentIndex()==3 && ui->clocksTableWidget->selectedItems().count()>0){
        uninstall_clock();
    }
}

void MainWindow::startUpdateSeconds(){
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }

    updateSeconds();
    updateSecondsTimer_->start(1000);
    if(timeForNext_->isHidden()){
        timeForNext_->show();
#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
            Global::setUnityProgressBarEnabled(true);
        }
#endif
    }
}

void MainWindow::updatePotdProgress(){
    int secsToPotd = Global::getSecondsTillHour("00:00");

    if(secsToPotd<=3600){
        //less than one hour left
        timeForNext_->setFormat(secondsToHms(secsToPotd));
    }
    else
    {
        timeForNext_->setFormat(secondsToHm(secsToPotd));
    }

    setProgressbarsValue(short(((float) (secsToPotd/86400.0)) * 100));
}

void MainWindow::setProgressbarsValue(short value){
    timeForNext_->setValue(value);
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
        Global::setUnityProgressbarValue((float)(value/100.0));
    }
#endif
}

void MainWindow::imageTransition(const QString &filename)
{
    if(!gv.previewImagesOnScreen || !gv.mainwindowLoaded){
        return;
    }

    if(filename=="preview_clock"){
        if(!futureWatcherWallClocksPreview_.isFinished()){
            futureWatcherWallClocksPreview_.cancel();
        }
        QFuture<QImage*> future = QtConcurrent::run(std::bind(&MainWindow::wallpaperClocksPreview, this, gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(),
                          ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(), ui->day_week_checkBox->isChecked(),
                          ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked()));
        futureWatcherWallClocksPreview_.setFuture(future);
    }
    else
    {
        if(!futureWatcherWallpapersPreview_.isFinished()){
            futureWatcherWallpapersPreview_.cancel();
        }
        QFuture<QImage*> future = QtConcurrent::run(this, &MainWindow::scaleWallpapersPreview, filename);
        futureWatcherWallpapersPreview_.setFuture(future);
    }
}

void MainWindow::wallpaperClocksPreviewImageGenerationFinished(){
    if(futureWatcherWallClocksPreview_.isCanceled()){
        QImage *result=futureWatcherWallClocksPreview_.result();
        if(result!=NULL){
            delete result;
        }
        return;
    }
    QImage *image = futureWatcherWallClocksPreview_.result();
    QImage *old = image;
    image = new QImage(old->scaled(QSize(old->width()*imagePreviewResizeFactorX_, old->height()*imagePreviewResizeFactorY_), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    delete old;
    setupAnimationsAndChangeImage(image);
}

void MainWindow::wallpapersPreviewImageGenerationFinished(){
    if(futureWatcherWallpapersPreview_.isCanceled()){
        QImage *result=futureWatcherWallpapersPreview_.result();
        if(result!=NULL){
            delete result;
        }
        return;
    }
    setupAnimationsAndChangeImage(futureWatcherWallpapersPreview_.result());
}

QImage *MainWindow::scaleWallpapersPreview(QString filename){
    QImageReader reader(filename);
    reader.setScaledSize(QSize(reader.size().width()*imagePreviewResizeFactorX_, reader.size().height()*imagePreviewResizeFactorY_));
    QImage *image = new QImage(reader.read());
    return image;
}

void MainWindow::setupAnimationsAndChangeImage(QImage *image){
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

    QtConcurrent::run(this, &MainWindow::setPreviewImage, image);
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

void MainWindow::setPreviewImage(QImage *image){
    if(!gv.previewImagesOnScreen){
        if(image!=NULL){
            delete image;
        }
        return;
    }

    if(image && !image->isNull()){
#ifdef ON_LINUX

        DesktopStyle desktopStyle=getDesktopStyle();

        if(image->format()==QImage::Format_Indexed8){
            //painting on QImage::Format_Indexed8 is not supported
            QImage *old=image;
            image = new QImage(QSize(old->width(), old->height()), QImage::Format_ARGB32_Premultiplied);
            QPainter painter(image);
            painter.drawImage(QPoint(0, 0), *old);
            painter.end();
            delete old;
        }
        QImage colorsGradientsImage(75, 75, QImage::Format_RGB32);

        //bring me some food right meow!
        bool previewColorsAndGradientsMeow=true;

        if(desktopStyle==Tile || desktopStyle==Stretch){
            //the preview of the background color(s) is meaningless for the current styling
            previewColorsAndGradientsMeow=false;
        }
        ColoringType coloringType=Global::getColoringType();
        if(previewColorsAndGradientsMeow || desktopStyle==NoneStyle){
            if(desktopStyle==NoneStyle){
                QImage *old=image;
                image = new QImage();
                delete old;
            }
            QString primaryColor;
            if(gv.setAverageColor)
            {
                primaryColor = QColor::fromRgb(image->scaled(1, 1, Qt::IgnoreAspectRatio, Qt::FastTransformation).pixel(0, 0)).name();
            }
            else
            {
                primaryColor = Global::getPrimaryColor();
            }

            colorsGradientsImage.fill(primaryColor);

            if(coloringType!=SolidColor && coloringType!=NoneColor)
            {
                QString secondaryColor=Global::getSecondaryColor();

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

                if(coloringType==VerticalColor){
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
            if(image->width() < vScreenWidth || image->height() < vScreenHeight){
                QImage originalImage = *image;
                //find how many times (rounded UPWARDS)
                short timesX = (vScreenWidth+originalImage.width()-1)/originalImage.width();
                short timesY = (vScreenHeight+originalImage.height()-1)/originalImage.height();

                QImage *old=image;
                image = new QImage(image->copy(0, 0, vScreenWidth, vScreenHeight));
                delete old;

                image->fill(Qt::black);

                QPainter painter(image);

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
                if(gv.currentDE==UnityGnome){
                    //both dimensions are bigger than the screen's. Center both dimensions.
                    QImage originalImage = *image;

                    QImage *old=image;
                    image = new QImage(image->copy(0, 0, vScreenWidth, vScreenHeight));
                    delete old;

                    image->fill(Qt::black);

                    QPainter painter(image);
                    painter.drawPixmap((vScreenWidth-originalImage.width())/2, (vScreenHeight-originalImage.height())/2, QPixmap::fromImage(originalImage));
                    painter.end();
                }
                else
                {
                    //just start from the top left of the image and fill the rest.
                    QImage *old=image;
                    image = new QImage(image->copy(0, 0, vScreenWidth, vScreenHeight));
                    delete old;
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
            QImage originalImage = *image;

            QImage *old=image;
            image = new QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);
            delete old;

            image->fill(Qt::transparent);

            QPainter painter(image);

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
            QImage originalImage = *image;

            QImage *old=image;
            image = new QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);
            delete old;

            image->fill(Qt::transparent);

            QPainter painter(image);
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
            QImage originalImage = *image;

            QImage *old=image;
            image = new QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);
            delete old;

            image->fill(Qt::transparent);

            QPainter painter(image);

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
            QImage *old=image;
            if(image->width() > 2*SCREEN_LABEL_SIZE_X || image->height() > 2*SCREEN_LABEL_SIZE_Y){
                image = new QImage(image->scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation).scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            else
            {
                image = new QImage(image->scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            delete old;
        }

        if(previewColorsAndGradientsMeow || desktopStyle==NoneStyle){
            colorsGradientsImage=colorsGradientsImage.scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation);
            QPainter painter;
            painter.begin(&colorsGradientsImage);
            painter.drawPixmap(0, 0, QPixmap::fromImage(*image));
            painter.end();
            QImage *old=image;
            image = new QImage(colorsGradientsImage);
            delete old;
        }
#else
        QImage *old = image;
        image = new QImage(image->scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation).scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        delete old;
#endif //#ifdef ON_LINUX
        ui->screen_label->setPixmap(QPixmap::fromImage(*image));
    }
    delete image;
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
        timeForNext_->show();
    }
    else
    {
        hideProgress_->start(400);
    }

#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
        Global::setUnityProgressBarEnabled(show);
    }
#endif //#ifdef ON_LINUX

    timeForNext_->setGraphicsEffect(opacityEffect_);
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
    timeForNext_->hide();
    timeForNext_->setFormat(tr("Calculating..."));
    setProgressbarsValue(100);
}

void MainWindow::findSeconds(bool typeCountSeconds)
{
    int count_seconds=gv.customIntervalsInSeconds.at(ui->timerSlider->value()-1);
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
        QString filename=Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
        if(!filename.isEmpty() && ui->stackedWidget->currentIndex()==2 && QFile::exists(filename)){
            imageTransition(filename);
        }
    }
}

void MainWindow::setStyle(){
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome || gv.currentDE==Mate){
        ui->image_style_combo->addItem(tr("Tile"));
        ui->image_style_combo->addItem(tr("Zoom"));
        ui->image_style_combo->addItem(tr("Center"));
        ui->image_style_combo->addItem(tr("Scale"));
        if(gv.currentDE==Mate){
            ui->image_style_combo->addItem(tr("Stretched"));
        }
        else
        {
            ui->image_style_combo->addItem(tr("Fill"));
        }
        ui->image_style_combo->addItem(tr("Span"));

        QString style;
        if(gv.currentDE==Mate){
            style=Global::gsettingsGet("org.mate.background", "picture-options");
        }
        else
        {
            style=Global::gsettingsGet("org.gnome.desktop.background", "picture-options");
        }
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
    else if(gv.currentDE==XFCE){
        ui->image_style_combo->addItem(tr("None"));
        ui->image_style_combo->addItem(tr("Centered"));
        ui->image_style_combo->addItem(tr("Tiled"));
        ui->image_style_combo->addItem(tr("Stretched"));
        ui->image_style_combo->addItem(tr("Scaled"));
        ui->image_style_combo->addItem(tr("Zoomed"));
        int index=0;
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-style")){
                QString imageStyle=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry);
                index=imageStyle.toInt();
            }
        }
        if(index<ui->image_style_combo->count() && index>=0){
            ui->image_style_combo->setCurrentIndex(index);
        }
    }
    else if(gv.currentDE==LXDE){
        ui->image_style_combo->addItem(tr("Empty"));
        ui->image_style_combo->addItem(tr("Fill"));
        ui->image_style_combo->addItem(tr("Fit"));
        ui->image_style_combo->addItem(tr("Center"));
        ui->image_style_combo->addItem(tr("Tile"));
        int value = Global::getPcManFmValue("wallpaper_mode").toInt();
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

#endif //#ifdef ON_LINUX

    gv.mainwindowLoaded=true;
    updateScreenLabel();
}

DesktopStyle MainWindow::getDesktopStyle(){
    DesktopStyle desktopStyle=Stretch;
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome || gv.currentDE==Mate){
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
    else if(gv.currentDE==XFCE){
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
    else if(gv.currentDE==LXDE){
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

void MainWindow::setButtonColor()
{
    QString colorName;
#ifdef ON_LINUX
    colorName=Global::getPrimaryColor();
#else
    QSettings collorSetting("HKEY_CURRENT_USER\\Control Panel\\Colors", QSettings::NativeFormat);
    colorName=collorSetting.value("Background", "0 0 0").toString();
    QList<int> rgb;
    QString temp;
    for(short i=0; colorName.size()>i; i++)
    {
        if(i==colorName.size()-1)
        {
            temp.append(colorName.at(i));
            rgb.append(temp.toInt());
        }
        else if(colorName.at(i)==' ')
        {
            rgb.append(temp.toInt());
            temp.clear();
        }
        else{
            temp.append(colorName.at(i));
        }
    }
    colorName=QColor(rgb.at(0), rgb.at(1), rgb.at(2)).name();
#endif //#ifdef ON_LINUX
    setButtonColor(colorName);
}

void MainWindow::setButtonColor(const QString &color_name){
#ifdef ON_WINDOWS
    QImage image(40, 19, QImage::Format_ARGB32_Premultiplied);
    image.fill(color_name);
    ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image)));
#else

#endif

    ColoringType coloringType=Global::getColoringType();
    if(coloringType == SolidColor){
        QImage image(40, 19, QImage::Format_ARGB32_Premultiplied);
        image.fill(color_name);
        ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image)));
    }
    else
    {
        QString secondaryColor=Global::getSecondaryColor();

        QImage image(75, 75, QImage::Format_RGB32);
        image.fill(Qt::white);
        QColor color(color_name);
        image.fill(color);
        color.setNamedColor(secondaryColor);
        short alpha=255;
        short softness=0;
        QPainter paint;
        paint.begin(&image);
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
            paint.fillRect(i-1, 0, 1, 75, QColor(color));
        }
        paint.end();
        if(coloringType == VerticalColor)
        {
            image = image.transformed(QTransform().rotate(+90), Qt::SmoothTransformation);
        }
        updateColorButton(image);
    }
}

void MainWindow::on_image_style_combo_currentIndexChanged(int index)
{
    if(!gv.mainwindowLoaded){
        return;
    }

#ifdef ON_LINUX
    if(gv.currentDE==Gnome || gv.currentDE==UnityGnome || gv.currentDE==Mate){
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
        if(gv.currentDE==Mate){
            Global::gsettingsSet("org.mate.background", "picture-options", type);
        }
        else
        {
            Global::gsettingsSet("org.gnome.desktop.background", "picture-options", type);
        }
    }
    else if(gv.currentDE==XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-style")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << QString::number(index));
            }
        }
    }
    else if(gv.currentDE==LXDE){
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
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID) Global::currentBackgroundImage().toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#else
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID) Global::currentBackgroundImage().toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#endif

#endif //#ifdef ON_LINUX

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
        if(gv.randomTimeEnabled)
        {
            srand(time(0));
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
                Global::saveSecondsLeftNow(-1, 0);
            }
            changeImage();
        }
        Global::saveSecondsLeftNow(secondsLeft_, 0);
    }
    else if(gv.wallpaperClocksRunning){
        QString currentWallpaperClock=Global::wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                       ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }

        if(ui->minutes_checkBox->isChecked()){
            secondsLeft_=60;
        }
        else if (ui->hour_checkBox->isChecked()){
            secondsLeft_=gv.refreshhourinterval*60;
        }
        else if(ui->am_checkBox->isChecked() && gv.amPmEnabled){
            secondsLeft_=43200;
        }
        else if(ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked()){
            secondsLeft_=86400;
        }
        totalSeconds_=secondsLeft_;
    }
    else if(gv.liveWebsiteRunning){
        ui->timeout_text_label->show();
        ui->website_timeout_label->show();
        processRequestStart();
        websiteSnapshot_->start();
        secondsLeft_=Global::websiteSliderValueToSeconds(ui->website_slider->value());
        Global::saveSecondsLeftNow(secondsLeft_, 2);
    }
    else if(gv.liveEarthRunning){
        processRequestStart();
        globalParser_->livearth();
        secondsLeft_=1800;
        Global::saveSecondsLeftNow(secondsLeft_, 1);
    }
    else if(gv.potdRunning){
        justUpdatedPotd_=true;
        //time has come
        QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
        QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
            //the previous time that photoofday changed was another day or the settings have changed
            processRequestStart();
            globalParser_->potd();
        }
        else
        {
            /*
             * The previous time was today, so the picture must be still there,
             * so re-set it as background, no need to download anything!
             */
            QString filename = Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
            Global::debug("Picture of the day has already been downloaded.");
            if(filename.isEmpty()){
                Global::error("Probably you set the picture of the day today but you removed the original\n"\
                              "downloaded photo. If it didn't happen like this, please report this as a bug!\n"\
                              "Wallch will now attemp to download again the Picture Of The Day.");
                processRequestStart();
                globalParser_->potd();
            }
            Global::setBackground(filename, true, true, 3);
        }
    }
}

void MainWindow::updateSeconds(){
    /*
     * This function updates the seconds of the progressbar of Wallch and launches
     * the corresponding action when the seconds have passed!
     */

    if(gv.potdRunning){
        if(Global::timeNowToString()=="00:00"){
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
        if(secondsLeft_!=gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval))
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

        timeForNext_->setFormat(secondsToHms(secondsLeft_));

        if(gv.wallpapersRunning)
        {
            if(gv.randomTimeEnabled){
                setProgressbarsValue((secondsLeft_*100)/initialRandomSeconds_);
            }
            else{
                setProgressbarsValue((secondsLeft_*100)/(totalSeconds_));
            }
        }
        else if(gv.wallpaperClocksRunning){
            setProgressbarsValue(secondsLeft_*100/totalSeconds_);
        }
        else if(gv.liveWebsiteRunning){
            setProgressbarsValue((secondsLeft_*100)/(Global::websiteSliderValueToSeconds(ui->website_slider->value())));
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

#ifdef ON_LINUX
void MainWindow::bindKey(const QString &key){
    if(!keybinder_bind(key.toLocal8Bit().data(), nextKeySignal, NULL)){
        //key could not be binded!
        Global::error("I probably could not bind the key sequence: '"+key+"'");
    }
}

void MainWindow::unbindKey(const QString &key){
    keybinder_unbind(key.toLocal8Bit().data(), nextKeySignal);
}
#endif

void MainWindow::on_website_slider_valueChanged(int value)
{
    ui->website_interval_slider_label->setText(secondsToMh(Global::websiteSliderValueToSeconds(value)));
}

void MainWindow::setThemeToAmbiance(){
    ui->page_0_wallpapers->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_1_earth->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_2_potd->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_3_clock->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_4_web->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_5_other->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->sep1->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep2->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep3->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep4->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep5->setPixmap(AMBIANCE_SEPARATOR);
    ui->widget->setStyleSheet("QWidget { background-image: url(:/icons/Pictures/ambiance_not_checked.png); }");
}

void MainWindow::setThemeToRadiance(){
    ui->page_0_wallpapers->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_1_earth->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_2_potd->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_3_clock->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_4_web->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_5_other->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->sep1->setPixmap(RADIANCE_SEPARATOR);
    ui->sep2->setPixmap(RADIANCE_SEPARATOR);
    ui->sep3->setPixmap(RADIANCE_SEPARATOR);
    ui->sep4->setPixmap(RADIANCE_SEPARATOR);
    ui->sep5->setPixmap(RADIANCE_SEPARATOR);
    ui->widget->setStyleSheet("QWidget { background-image: url(:/icons/Pictures/radiance_not_checked.png); }");
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
        //autodetect!
        QString curTheme;
#ifdef ON_LINUX
        if(gv.currentDE==Gnome || gv.currentDE==UnityGnome){
            curTheme = Global::gsettingsGet("org.gnome.desktop.interface", "gtk-theme");
        }
        else
        {
            if(gv.currentTheme=="ambiance"){
                curTheme="Ambiance";
            }
            else
            {
                curTheme="Radiance";
            }
        }
#endif
        if(curTheme=="Ambiance"){
            gv.currentTheme="ambiance";
            setThemeToAmbiance();
        }
        else
        {
            gv.currentTheme="radiance";
            setThemeToRadiance();
        }
    }
}

void MainWindow::closeEverythingThatsRunning(short excludingFeature)
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
        return QString::number(finalSeconds)+"s";
    }
    else if(!hours_left){
        if(finalSeconds){
            return QString::number(minutes_left)+"m " + QString::number(finalSeconds)+"s";
        }
        else{
            return QString::number(minutes_left)+"m";
        }
    }
    else if(!minutes_left){
        if(finalSeconds){
            return QString::number(hours_left) + "h " + QString::number(finalSeconds)+"s";
        }
        else{
            return QString::number(hours_left) + "h";
        }
    }
    else
    {
        if(finalSeconds){
            return QString::number(hours_left) + "h " + QString::number(minutes_left)+"m " + QString::number(finalSeconds)+"s";
        }
        else{
            return QString::number(hours_left) + "h " + QString::number(minutes_left)+"m";
        }
    }
}

QString MainWindow::secondsToHm(int seconds){
    if(seconds<=60){
        return QString("1m");
    }
    else if(seconds<=3600){
        return QString::number(seconds/60)+QString("m");
    }
    else
    {
        int hours=seconds/3600;
        seconds-=hours*3600;
        int minutes=seconds/60;
        if(minutes){
            return QString::number(hours)+QString("h ")+QString::number(minutes)+QString("m");
        }
        else{
            return QString::number(hours)+QString("h");
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
    setButtonColor(Global::setAverageColor(image));
}

QString MainWindow::fixBasenameSize(const QString &basename){
    if(basename.length()>35){
        return basename.left(32)+"...";
    }
    return basename;
}

void MainWindow::tvPreview(bool show)
{
    showTvMinimumHeight_ = new QPropertyAnimation(ui->preview_widget, "minimumHeight");
    showTvMinimumHeight_->setDuration(350);
    showTvMinimumHeight_->setStartValue(ui->preview_widget->minimumHeight());
    if(show)
    {
        ui->preview_widget->show();
        ui->screen_label->clear();
        ui->screen_label_text->clear();
        QTimer::singleShot(400, this, SLOT(updateScreenLabel()));
        showTvMinimumHeight_->setEndValue(281);
    }
    else
    {
        QTimer::singleShot(430, this, SLOT(hideTv()));
        showTvMinimumHeight_->setEndValue(0);
    }
    showTvMinimumHeight_->start();
}

void MainWindow::hideTv()
{
    ui->preview_widget->hide();
    delete showTvMinimumHeight_;
}

void MainWindow::updateScreenLabel()
{
    ui->screen_label_info->clear();
    ui->screen_label_text->clear();

    switch(ui->stackedWidget->currentIndex()){
    case 0:

        if(ui->listWidget->count()==0 || ui->listWidget->selectedItems().count()==0)
        {
            changeTextOfScreenLabelTo(tr("No image selected to preview"));
        }
        else{
            on_listWidget_itemSelectionChanged();
        }

        break;

    case 1:
    {

        QString filename=Global::getFilename(gv.wallchHomePath+"liveEarth*");
        if(!filename.isEmpty()){
            imageTransition(filename);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("No live earth has\nbeen downloaded to preview"));
        }

        break;

    }
    case 2:
    {

        QString filename=Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
        if(!filename.isEmpty()){
            imageTransition(filename);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("No picture of the day has\nbeen downloaded to preview"));
        }

        break;

    }
    case 3:

        if(ui->clocksTableWidget->rowCount()==0)
        {
            changeTextOfScreenLabelTo(tr("No wallpaper clock has been\ninstalled to preview"));
        }
        else if(ui->clocksTableWidget->rowCount()!=0 && ui->clocksTableWidget->selectedItems().count()==0)
        {
            changeTextOfScreenLabelTo(tr("No wallpaper clock has been\nselected to preview"));
        }
        else if(ui->clocksTableWidget->rowCount()!=0 && ui->clocksTableWidget->selectedItems().count()!=0)
        {
            imageTransition("preview_clock");
        }

        break;

    case 4:

        if(QFile::exists(gv.wallchHomePath+LW_PREVIEW_IMAGE)){
            imageTransition(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        }
        else
        {
            changeTextOfScreenLabelTo(tr("No website image to preview"));
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

#ifdef ON_LINUX
void MainWindow::unityProgressbarSetEnabled(bool enabled){
    if(gv.wallpapersRunning || gv.liveEarthRunning || gv.potdRunning || gv.wallpaperClocksRunning || gv.liveWebsiteRunning ){
        Global::setUnityProgressBarEnabled(enabled);
        if(enabled){
            Global::setUnityProgressbarValue(0);
        }
    }
}

void MainWindow::indicatorSetNormal(){
    Global::changeIndicatorIcon("normal");
}

#endif //#ifdef ON_LINUX

void MainWindow::updateColorButton(QImage image)
{
    ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image.scaled(40, 19, Qt::IgnoreAspectRatio, Qt::FastTransformation))));
}

//Wallpapers code

void MainWindow::justChangeWallpaper(){
    if(!wallpapersPageLoaded_){
        loadWallpapersPage();
        if(ui->listWidget->count()<LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
        if(gv.randomTimeEnabled){
            ui->timerSlider->setEnabled(false);
            ui->wallpapers_slider_time->setEnabled(false);
            ui->interval_label->setEnabled(false);
        }
        else
        {
            ui->timerSlider->setEnabled(true);
            ui->wallpapers_slider_time->setEnabled(true);
            ui->interval_label->setEnabled(true);
        }
    }
    srand(time(0));
    Global::setBackground(getPathOfListItem(rand()%ui->listWidget->count()), true, true, 1);
}

void MainWindow::on_startButton_clicked(){
    if(!wallpapersPageLoaded_){
        loadWallpapersPage();
        if(ui->listWidget->count()<LEAST_WALLPAPERS_FOR_START){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
        if(gv.randomTimeEnabled){
            ui->timerSlider->setEnabled(false);
            ui->wallpapers_slider_time->setEnabled(false);
            ui->interval_label->setEnabled(false);
        }
        else
        {
            ui->timerSlider->setEnabled(true);
            ui->wallpapers_slider_time->setEnabled(true);
            ui->interval_label->setEnabled(true);
        }
    }

    if (!ui->startButton->isEnabled()){
        //this is the case it is launched from the indicator
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
#endif //#ifdef ON_LINUX
        globalParser_->desktopNotify(tr("Not enough pictures to start chaning wallpapers."), false, "info");
        Global::error("Too few images for wallpaper feature to start. Make sure there are at least 2 pictures at the selected folder.");
        return;
    }

    closeEverythingThatsRunning(1);
    startPauseWallpaperChangingProcess();
}

void MainWindow::startPauseWallpaperChangingProcess(){
    if(!currentFolderExists()){
        return;
    }
    if (actAsStart_){
        actAsStart_=false; //the next time act like pause is pressed
        gv.wallpapersRunning=true;
        Global::updateStartup();
        startWasJustClicked_=true;

        ui->startButton->setText(tr("Pau&se"));
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-pause", QIcon(":/icons/Pictures/media-playback-pause.svg")));

        shuffleWasChecked_=ui->shuffle_images_checkbox->isChecked();
        if(!gv.processPaused){
            if(shuffleWasChecked_){
                if(firstRandomImageIsntRandom_){
                    firstRandomImageIsntRandom_=false;
                    Global::generateRandomImages(ui->listWidget->count(), indexOfCurrentImage_);
                    indexOfCurrentImage_=0;
                }
                else
                {
                    Global::generateRandomImages(ui->listWidget->count(), -1);
                }
            }
        }

        if(gv.processPaused){
            //If the process was paused, then we need to continue from where it is left, not from the next second
            secondsLeft_+=1;
        }

        Global::resetSleepProtection(secondsLeft_);

        if(!gv.processPaused){
            //if the process wasn't paused, then the progressbar is hidden. Add an animation so as to show it
            animateProgressbarOpacity(1);
            if(gv.randomTimeEnabled)
            {
                srand(time(0));
                secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
                initialRandomSeconds_=secondsLeft_;
                totalSeconds_=secondsLeft_;
            }
        }
        gv.processPaused=false;
#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
            dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
            Global::setUnityProgressBarEnabled(true);
        }
        Global::changeIndicatorSelection("wallpapers");
#else
        Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX

        startUpdateSeconds();
        stopButtonsSetEnabled(true);
        previousAndNextButtonsSetEnabled(true);
        on_page_0_wallpapers_clicked();
    }
    else
    {
        actAsStart_=true; //<- the next time act like start is pressed
        gv.processPaused=true;
        ui->startButton->setText(tr("&Start"));
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/Pictures/media-playback-start.svg")));
#ifdef ON_LINUX
        Global::changeIndicatorSelection("pause");
        if(gv.currentDE==UnityGnome){
            dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
        }
#else
        Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX
        updateSecondsTimer_->stop();
        timeForNext_->setFormat(timeForNext_->format()+" - Paused.");
        previousAndNextButtonsSetEnabled(false);
        if(ui->listWidget->count() != 0){
            ui->shuffle_images_checkbox->setEnabled(true);
        }
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(secondsLeft_, 0);
        }
    }
}

void MainWindow::on_stopButton_clicked(){
    if (!ui->stopButton->isEnabled()){
        return;
    }
    if(ui->listWidget->count()!=0){
        ui->shuffle_images_checkbox->setEnabled(true);
    }
    actAsStart_=true;
    ui->startButton->setText(tr("&Start"));
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/Pictures/media-playback-start.svg")));
    animateProgressbarOpacity(0);
    gv.processPaused=false;
    firstRandomImageIsntRandom_=false;
    indexOfCurrentImage_=0;
    secondsLeft_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    previousAndNextButtonsSetEnabled(false);
    if(ui->listWidget->count() < LEAST_WALLPAPERS_FOR_START){
        startButtonsSetEnabled(false);
    }
    else
    {
        startButtonsSetEnabled(true);
    }

    stopButtonsSetEnabled(false);
    gv.wallpapersRunning=false;
    Global::updateStartup();
#ifdef ON_LINUX
    Global::changeIndicatorSelection("none");
    if(gv.currentDE==UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
    if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
        Global::setUnityProgressBarEnabled(false);
    }
#else
    Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(-1, 0);
    }
}

void MainWindow::on_next_Button_clicked()
{
    if (!ui->next_Button->isEnabled() || !currentFolderExists()){
        return;
    }

    updateSecondsTimer_->stop();
#ifdef ON_LINUX
    Global::changeIndicatorIcon("right");
    indicatorChangeNormalTimer_->start(INDICATOR_SCROLL_INTERVAL);
#endif
    secondsLeft_=0;
    startUpdateSeconds();
}

void MainWindow::on_previous_Button_clicked()
{
    if(!ui->previous_Button->isEnabled() || !currentFolderExists()){
        return;
    }

    updateSecondsTimer_->stop();

    QString image;
    if(ui->shuffle_images_checkbox->isChecked()){
        if(previousBackgrounds_.count()>1){
            image=previousBackgrounds_.at(previousBackgrounds_.count()-2);
            if(QImage(image).isNull()){
                return;
            }
            previousBackgrounds_.removeLast();
        }
        else
        {
            image=getPathOfListItem(gv.randomImages[gv.currentRandomImageIndex]);

            if(QImage(image).isNull()){
                return;
            }

            int listCount=ui->listWidget->count();

            if(gv.currentRandomImageIndex>=(listCount-1)){
                Global::generateRandomImages(listCount, -1);
            }
            else
            {
                gv.currentRandomImageIndex++;
            }

        }
        Global::setBackground(image, true, true, 1);
        if(gv.rotateImages && gv.iconMode){
            forceUpdateIconOf(gv.randomImages[gv.currentRandomImageIndex]);
        }
    }
    else
    {
        if(indexOfCurrentImage_==1){
            indexOfCurrentImage_=ui->listWidget->count()+1;
        }

        image=getPathOfListItem(indexOfCurrentImage_-2);

        if(QImage(image).isNull()){
            return;
        }

        Global::setBackground(image, true, true, 1);
        if(gv.rotateImages && gv.iconMode){
            forceUpdateIconOf(indexOfCurrentImage_-2);
        }
        indexOfCurrentImage_--;
    }

    if(gv.randomTimeEnabled){
        srand(time(0));
        secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
        initialRandomSeconds_=secondsLeft_;
    }
    else
    {
        findSeconds(true);
    }
#ifdef ON_LINUX
    Global::changeIndicatorIcon("left");
    indicatorChangeNormalTimer_->start(INDICATOR_SCROLL_INTERVAL);
#endif
    Global::resetSleepProtection(secondsLeft_);

    startUpdateSeconds();
}

bool MainWindow::currentFolderExists()
{
    if(QDir(currentFolder_).exists()){
        return true;
    }
    else
    {
        int index=ui->pictures_location_comboBox->currentIndex();
        ui->pictures_location_comboBox->setItemText(index, Global::basenameOf(currentFolder_)+" (0)" );
        ui->listWidget->clear();
        if(gv.wallpapersRunning){
            on_stopButton_clicked();
        }
        if(QMessageBox::question(this, tr("Location is not available"), currentFolder_+" "+tr("referes to a location that is unavailable.")+" "+
         tr("It could be on a hard drive on this computer so check if that disk is properly inserted, or it could be on a network so check if you are connected to the Internet or your network, and then try again. Delete this folder from the list?"))==QMessageBox::Yes)
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
            ui->pictures_location_comboBox->removeItem(index);
            updatePicturesLocations();
        }
        else
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
        }
        return false;
    }
}

void MainWindow::on_listWidget_itemSelectionChanged()
{
    if(!gv.mainwindowLoaded || !gv.previewImagesOnScreen){
        return;
    }

    if(ui->listWidget->selectedItems().count()==1){

        QString filename=getPathOfListItem();

        if(!processingOnlineRequest_){
            ui->screen_label_info->setText(fixBasenameSize(Global::basenameOf(filename)));
        }
        imageTransition(filename);
    }
}

void MainWindow::on_listWidget_itemDoubleClicked()
{
    if(!ui->listWidget->count()){
        return;
    }

    int curRow=ui->listWidget->currentRow();
    if(curRow<0){
        return;
    }

    QString pic = getPathOfListItem(curRow);

    if(!QFile::exists(pic) || QImage(pic).isNull()){
        return;
    }
    Global::addPreviousBackground(previousBackgrounds_, pic);
    Global::setBackground(pic, true, true, 1);
    if(gv.rotateImages && gv.iconMode){
        forceUpdateIconOf(curRow);
    }
    if(gv.currentDE==LXDE){
        DesktopStyle desktopStyle=getDesktopStyle();
        if(desktopStyle==NoneStyle){
            ui->image_style_combo->setCurrentIndex(2);
        }
    }
}

void MainWindow::startWithThisImage(){
    closeEverythingThatsRunning(0);
    indexOfCurrentImage_=ui->listWidget->currentRow();
    if(ui->shuffle_images_checkbox->isChecked()){
        firstRandomImageIsntRandom_=true;
    }
    on_startButton_clicked();
}

void MainWindow::removeImageFromDisk(){
    if (QMessageBox::question(this, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the selected image?")) == QMessageBox::Yes)
    {
        QString image_filename = getPathOfListItem(ui->listWidget->currentRow());

        if(!QFile::remove(image_filename)){
            QMessageBox::warning(this, tr("Error!"), tr("Image deletion failed possibly because you don't have the permissions to delete the image or the image doesn't exist"));
        }
        else
        {
            //remove the cache of the image, in case it exists
            QString cache_filename=gv.cachePath+Global::originalToCacheName(image_filename);
            qint64 cache_file_size=QFile(cache_filename).size();
            if(QFile::remove(cache_filename)){
                //file successfully removed, remove the size of it from the current cache size!
                Global::addToCacheSize(cache_file_size*-1);
            }
        }
    }
    else
    {
        return;
    }

    if(ui->listWidget->count()<=1){
        startButtonsSetEnabled(false);
    }
}

void MainWindow::removeImagesFromDisk(){
    int selectedCount=ui->listWidget->selectedItems().count();
    if(QMessageBox::question(this, tr("Warning!"), tr("Image deletion.")+"<br><br>"+tr("This action is going to permanently delete")+" "+QString::number(selectedCount)+" "+tr("images. Are you sure?"))==QMessageBox::Yes){
        QStringList imagesThatFailedToDelete;
        for(int i=0;i<selectedCount;i++){
            QString curImage;
            if(gv.iconMode){
                if(ui->listWidget->selectedItems().at(i)->statusTip().isEmpty()){
                    curImage = ui->listWidget->selectedItems().at(i)->toolTip();
                }
                else
                {
                    curImage = ui->listWidget->selectedItems().at(i)->statusTip();
                }
            }
            else
            {
                curImage = ui->listWidget->selectedItems().at(i)->text();
            }
            if(!QFile::remove(curImage)){
                imagesThatFailedToDelete << curImage;
            }
            else
            {
                //remove the cache of the image, in case it exists
                QString cacheFilename=gv.cachePath+Global::originalToCacheName(curImage);
                qint64 cacheFileSize=QFile(cacheFilename).size();
                if(QFile::remove(cacheFilename)){
                    //file successfully removed, remove the size of it from the current cache size!
                    Global::addToCacheSize(cacheFileSize*-1);
                }
            }
        }
        if(imagesThatFailedToDelete.count()>0){
            QString all_images = "\n"+imagesThatFailedToDelete.join("\n");
            QMessageBox::warning(this, tr("Error!"), tr("There was a problem with the deletion of the following files:")+all_images);
        }
    }
}

void MainWindow::rotateRight(){
    if(!ui->listWidget->currentItem()->isSelected()){
        return;
    }
    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull()){
        return;
    }
    Global::rotateImg(path, 6, false);
    if(gv.iconMode){
        forceUpdateIconOf(ui->listWidget->currentRow());
    }
    else if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }

    if(path==Global::currentBackgroundImage()){
        Global::setBackground(path, false, false, 1);
    }
}

void MainWindow::rotateLeft(){
    if(!ui->listWidget->currentItem()->isSelected()){
        return;
    }
    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull()){
        return;
    }
    Global::rotateImg(path, 8, false);
    if(gv.iconMode){
        forceUpdateIconOf(ui->listWidget->currentRow());
    }
    else if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }

    if(path==Global::currentBackgroundImage()){
        Global::setBackground(path, false, false, 1);
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
     * Returns the full path of the current item's path if index==-1
     */
    if(index==-1){
        index=ui->listWidget->currentRow();
    }
    if(gv.iconMode){
        if(ui->listWidget->item(index)->statusTip().isEmpty()){
            return ui->listWidget->item(index)->toolTip();
        }
        else
        {
            return ui->listWidget->item(index)->statusTip();
        }
    }
    else
    {
        return ui->listWidget->item(index)->text();
    }
}

void MainWindow::on_listWidget_customContextMenuRequested()
{
    if(!currentFolderExists())
        return;

    int selectedCount = ui->listWidget->selectedItems().count();

    if(selectedCount==0){
        return;
    }

    if (selectedCount<2){
        listwidgetMenu_ = new QMenu(this);
        listwidgetMenu_->addAction(tr("Set this item as Background"), this, SLOT(on_listWidget_itemDoubleClicked()));
        if(ui->listWidget->count()>2){
            listwidgetMenu_->addAction(tr("Start from this image"), this, SLOT(startWithThisImage()));
        }
        listwidgetMenu_->addAction(tr("Delete image from disk"), this, SLOT(removeImageFromDisk()));
        listwidgetMenu_->addAction(tr("Rotate Right"), this, SLOT(rotateRight()));
        listwidgetMenu_->addAction(tr("Rotate Left"), this, SLOT(rotateLeft()));
        listwidgetMenu_->addAction(tr("Copy path to clipboard"), this, SLOT(copyImagePath()));
        listwidgetMenu_->addAction(tr("Copy image to clipboard"), this, SLOT(copyImage()));
        listwidgetMenu_->addAction(tr("Open folder"), this, SLOT(openImageFolder()));
        listwidgetMenu_->addAction(tr("Find an image by name"), this, SLOT(showHideSearchBoxMenu()));
        listwidgetMenu_->addAction(tr("Properties"), this, SLOT(showProperties()));
        listwidgetMenu_->popup(MENU_POPUP_POS);
    }
    else
    {
        //more than one file has been selected, show shorter menu with massive options
        listwidgetMenu_ = new QMenu(this);
        listwidgetMenu_->addAction(tr("Open containing folder of all images"), this, SLOT(openImageFolderMassive()));
        listwidgetMenu_->addAction(tr("Delete images from disk"), this, SLOT(removeImagesFromDisk()));
        listwidgetMenu_->popup(MENU_POPUP_POS);
    }
}

void MainWindow::addFolderForMonitor(const QString &folder){
    if(QDir(folder).exists()){
        short count = ui->pictures_location_comboBox->count();
        for(short i=0; i<count; i++)
        {
            if(Global::foldersAreSame(folder, ui->pictures_location_comboBox->itemData(i).toString()))
            {
                ui->pictures_location_comboBox->setCurrentIndex(i);
                return;
            }
        }
        ui->pictures_location_comboBox->addItem(Global::basenameOf(folder));
        ui->pictures_location_comboBox->setItemData(count, folder);
        ui->pictures_location_comboBox->setCurrentIndex(count);
        updatePicturesLocations();
    }
}

void MainWindow::changeImage(){
    //this function handles the change of the image of the wallpapers page.
    QString image;

    if(shuffleWasChecked_!=ui->shuffle_images_checkbox->isChecked()){
        shuffleWasChecked_=ui->shuffle_images_checkbox->isChecked();
        //the suffle has changed state!
        if(shuffleWasChecked_){
            Global::generateRandomImages(ui->listWidget->count(), -1);
        }
        else
        {
            processRunningResetPictures(true);
        }
    }
    if(shuffleWasChecked_){
        int listCount = ui->listWidget->count();
        if(gv.currentRandomImageIndex>=gv.randomImages.count()){
            Global::generateRandomImages(listCount, -1);
        }

        image=getPathOfListItem(gv.randomImages[gv.currentRandomImageIndex]);

        if(!QImage(image).isNull()){
            Global::addPreviousBackground(previousBackgrounds_, image);
            Global::setBackground(image, true, true, 1);
            if(gv.rotateImages && gv.iconMode){
                forceUpdateIconOf(gv.randomImages[gv.currentRandomImageIndex]);
            }
        }
        if(gv.currentRandomImageIndex>=(listCount-1)){
            Global::generateRandomImages(listCount, -1); //the loop has ended...
        }
        else
        {
            gv.currentRandomImageIndex++;
        }
    }
    else
    {
        if(indexOfCurrentImage_>=ui->listWidget->count() || indexOfCurrentImage_<0){
            indexOfCurrentImage_=0;
        }

        image=getPathOfListItem(indexOfCurrentImage_);

        if(!QImage(image).isNull()){
            Global::setBackground(image, true, true, 1);
            if(gv.rotateImages && gv.iconMode){
                forceUpdateIconOf(indexOfCurrentImage_);
            }
        }
        indexOfCurrentImage_++;
    }
}

void MainWindow::searchFor(const QString &term){
    ui->listWidget->clearSelection();
    searchList_.clear();
    int listCount=ui->listWidget->count();
    if(gv.iconMode){
        for(int i=0;i<listCount;i++){
            if(ui->listWidget->item(i)->statusTip().isEmpty()){
                searchList_ << Global::basenameOf(ui->listWidget->item(i)->toolTip());
            }
            else
            {
                searchList_ << Global::basenameOf(ui->listWidget->item(i)->statusTip());
            }
        }
    }
    else
    {
        for(int i=0; i<listCount; i++){
            searchList_ << Global::basenameOf(ui->listWidget->item(i)->text());
        }
    }
    match_->setPattern("*"+term+"*");
    currentSearchItemIndex = searchList_.indexOf(*match_, 0);
    if(currentSearchItemIndex < 0){
        doesntMatch();
    }
    else
    {
        ui->listWidget->scrollToItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->setCurrentItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->item(currentSearchItemIndex)->setSelected(true);
        doesMatch();
    }
}

void MainWindow::continueToNextMatch(){
    ui->listWidget->clearSelection();
    if(currentSearchItemIndex >= ui->listWidget->count()+1){
        //restart the search...
        currentSearchItemIndex=searchList_.indexOf(*match_, 0);
        if(currentSearchItemIndex<0){
            return;
        }
        ui->listWidget->scrollToItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->setCurrentItem(ui->listWidget->item(currentSearchItemIndex));
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
        ui->listWidget->scrollToItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->setCurrentItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->item(currentSearchItemIndex)->setSelected(true);
    }
}

void MainWindow::continueToPreviousMatch(){
    ui->listWidget->clearSelection();
    if(currentSearchItemIndex < 0){
        //restart the search...
        currentSearchItemIndex=searchList_.lastIndexOf(*match_, searchList_.count()-1);
        if(currentSearchItemIndex<0){
            return;
        }
        ui->listWidget->scrollToItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->setCurrentItem(ui->listWidget->item(currentSearchItemIndex));
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
        ui->listWidget->scrollToItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->setCurrentItem(ui->listWidget->item(currentSearchItemIndex));
        ui->listWidget->item(currentSearchItemIndex)->setSelected(true);
    }
}

void MainWindow::openImageFolder(){

    if(!ui->listWidget->currentItem()->isSelected()){
        return;
    }
    Global::openFolderOf(getPathOfListItem());
}

void MainWindow::openImageFolderMassive(){
    QStringList parent_folders;
    int selected_count=ui->listWidget->selectedItems().count();
    short total_folders_count=0;
    for(int i=0;i<selected_count;i++){
        QString currentImage;
        if(gv.iconMode){
            if(ui->listWidget->selectedItems().at(i)->statusTip().isEmpty()){
                currentImage = ui->listWidget->selectedItems().at(i)->toolTip();
            }
            else{
                currentImage = ui->listWidget->selectedItems().at(i)->statusTip();
            }
        }
        else{
            currentImage = ui->listWidget->selectedItems().at(i)->text();
        }
        QString image_folder = Global::dirnameOf(currentImage);
        if(!parent_folders.contains(image_folder)){
            parent_folders << image_folder;
            total_folders_count++;
        }
    }
    if(total_folders_count>5){
        if(QMessageBox::question(this, tr("Warning!"), tr("Too many folders to open.")+"<br><br>"+tr("This action is going to open")+" "+QString::number(total_folders_count)+" "+tr("folders. Are you sure?"))!=QMessageBox::Yes){
            return;
        }
    }
    for(short i=0;i<total_folders_count;i++)
        Global::openFolderOf(parent_folders.at(i));
}

void MainWindow::on_timerSlider_valueChanged(int value)
{
    if(gv.mainwindowLoaded){
        findSeconds(false);
        updateCheckTime_->start(3000);
    }
    ui->wallpapers_slider_time->setText(customIntervals_.at(value-1));
    settings->setValue("delay", secondsInWallpapersSlider_);
    settings->setValue( "timeSlider", ui->timerSlider->value());
}

void MainWindow::updateTiming(){
    if(updateCheckTime_->isActive()){
        updateCheckTime_->stop();
    }
    if(!wallpapersPageLoaded_){
        return;
    }
    settings->setValue( "timeSlider", ui->timerSlider->value());
    settings->setValue( "delay", secondsInWallpapersSlider_ );
    settings->sync();
}

void MainWindow::startButtonsSetEnabled(bool enabled)
{
    ui->startButton->setEnabled(enabled);
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, !enabled);
    }
#endif //#ifdef ON_LINUX
}

void MainWindow::stopButtonsSetEnabled(bool enabled)
{
    ui->stopButton->setEnabled(enabled);

#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
    }
#endif //#ifdef ON_LINUX
}

void MainWindow::previousAndNextButtonsSetEnabled(bool enabled){
    ui->next_Button->setEnabled(enabled); ui->previous_Button->setEnabled(enabled);

#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityLauncherEntry){
        dbusmenu_menuitem_property_set_bool(gv.unityNextAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
        dbusmenu_menuitem_property_set_bool(gv.unityPreviousAction, DBUSMENU_MENUITEM_PROP_VISIBLE, enabled);
    }
#endif //#ifdef ON_LINUX
}

void MainWindow::random_time_changed(short index)
{
    if(index)
    {
        ui->timerSlider->setEnabled(false); ui->wallpapers_slider_time->setEnabled(false); ui->interval_label->setEnabled(false);
    }
    else
    {
        ui->timerSlider->setEnabled(true); ui->wallpapers_slider_time->setEnabled(true); ui->interval_label->setEnabled(true);
    }
}

void MainWindow::monitor(const QStringList &paths){
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
    int itemCount=0;
    Q_FOREACH(QString path, paths){
        monitor(path, itemCount);
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
            ui->pictures_location_comboBox->setItemText(ui->pictures_location_comboBox->currentIndex(), Global::basenameOf(currentFolder_)+" ("+QString::number(itemCount)+ ")");
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

    if(ui->listWidget->count() < LEAST_WALLPAPERS_FOR_START){
        startButtonsSetEnabled(false);
    }
    else
    {
        startButtonsSetEnabled(true);
    }
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
        if(ui->listWidget->selectedItems().count()){
            if(gv.iconMode){
                if(ui->listWidget->selectedItems().at(0)->statusTip().isEmpty()){
                    nameOfSelectionPriorFolderChange_=ui->listWidget->selectedItems().at(0)->toolTip();
                }
                else
                {
                    nameOfSelectionPriorFolderChange_=ui->listWidget->selectedItems().at(0)->statusTip();
                }
            }
            else
            {
                nameOfSelectionPriorFolderChange_=ui->listWidget->selectedItems().at(0)->text();
            }
        }
    }

    if(searchIsOn_){
        on_search_close_clicked();
    }

    ui->search_box->clear();
    ui->listWidget->clear();
    monitoredFoldersList_.clear();
    resetWatchFolders();

    monitor(Global::listFolders(currentFolder_, true, true));

    if(ui->listWidget->count() < LEAST_WALLPAPERS_FOR_START){
        if(gv.wallpapersRunning){
            on_stopButton_clicked();
        }
    }
    else
    {
        startButtonsSetEnabled(true);
    }

    if(gv.wallpapersRunning){
        processRunningResetPictures(false);
    }

    if(gv.previewImagesOnScreen && ui->stackedWidget->currentIndex()==0){
        searchFor(nameOfSelectionPriorFolderChange_);
        ui->screen_label_info->clear();
        updateScreenLabel();
    }
}

void MainWindow::addFilesToWallpapers(const QString &path, int &itemCount){
    QDir currrentDirectory(path, QString(""), QDir::Name, QDir::Files);
    if(gv.iconMode){
        Q_FOREACH(QString file, currrentDirectory.entryList(IMAGE_FILTERS)){
            QListWidgetItem *item = new QListWidgetItem;
            item->setIcon(imageLoading_);
            item->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
            item->setToolTip(path+"/"+file);
            ui->listWidget->addItem(item);
            itemCount++;
        }
    }
    else
    {
        QString temp;
        Q_FOREACH(QString file, currrentDirectory.entryList(IMAGE_FILTERS)){
            temp=path+"/"+file;
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(temp);
            item->setToolTip(tr("Right-click for options"));
            item->setStatusTip(temp);
            ui->listWidget->addItem(item);
            itemCount++;
        }
    }
}

void MainWindow::changePathsToIcons()
{
    ui->listWidget->setIconSize(WALLPAPERS_LIST_ICON_SIZE);
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setUniformItemSizes(true);
    ui->listWidget->setDragEnabled(false);
    gv.iconMode=true;
    int listCount=ui->listWidget->count();

    for(int i=0;i<listCount;i++){
        QString status=ui->listWidget->item(i)->statusTip();
        ui->listWidget->item(i)->setToolTip(status);
        ui->listWidget->item(i)->setStatusTip("");
        ui->listWidget->item(i)->setText("");
        ui->listWidget->item(i)->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->listWidget->item(i)->setIcon(imageLoading_);
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
    ui->listWidget->setViewMode(QListView::ListMode);
    ui->listWidget->setUniformItemSizes(false);
    ui->listWidget->setIconSize(QSize(-1, -1));
    gv.iconMode=false;
    int file_count=ui->listWidget->count();

    QFont font;
    font.defaultFamily();
    for(int i=0;i<file_count;i++){
        ui->listWidget->item(i)->setTextAlignment(Qt::AlignLeft);
        ui->listWidget->item(i)->setFont(font);
        if(!ui->listWidget->item(i)->statusTip().isEmpty()){
            ui->listWidget->item(i)->setText(ui->listWidget->item(i)->statusTip());
        }
        else
        {
            QString path=ui->listWidget->item(i)->toolTip();
            ui->listWidget->item(i)->setText(path);
            ui->listWidget->item(i)->setStatusTip(path);
            ui->listWidget->item(i)->setToolTip(tr("Right-click for options"));
        }
        ui->listWidget->item(i)->setIcon(QIcon(""));
        ui->listWidget->item(i)->setData(Qt::SizeHintRole, QVariant());
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
    if(!ui->listWidget->count()){
        return;
    }
    currentlyUpdatingIcons_=true;
    QListWidgetItem* minItem=ui->listWidget->itemAt(0, 0);
    QListWidgetItem* maxItem=ui->listWidget->itemAt(ui->listWidget->width()-30, ui->listWidget->height()-5);

    int diff=30;
    while(!maxItem){
        diff+=5;
        maxItem=ui->listWidget->itemAt(ui->listWidget->width()-diff, ui->listWidget->height()-5);
        if(ui->listWidget->width()-diff<0){
            maxItem=ui->listWidget->item(ui->listWidget->count()-1);
        }
    }

    int min_row=ui->listWidget->row(minItem);
    int max_row=ui->listWidget->row(maxItem);
    for (int row = min_row; row <= max_row; row++) {
        if(!currentlyUpdatingIcons_ || appAboutToClose_){
            //either a new timer has been launched for this function or the application is about to close.
            return;
        }
        if(!ui->listWidget->item(row)){
            continue; //<- in case the icons are loading and the user chooses to clear the list or the item has somehow been removed
        }
        if(ui->listWidget->item(row)->statusTip().isEmpty()){
            if(updateIconOf(row)==false){
                //some image was null, go for the next row (which now is row--)
                row--;
            }
        }
    }
}

bool MainWindow::updateIconOf(int index){
    //updates the icon of the given row (item). It only works in icon mode, of course.
    QString originalImagePath=ui->listWidget->item(index)->toolTip();
    QString cachedImagePath=Global::originalToCacheName(originalImagePath);
    bool haveToCache=true;

    QString imagePath;

    if(QFile(gv.cachePath+cachedImagePath).exists())
    {
        imagePath=gv.cachePath+cachedImagePath;
        haveToCache=false;
    }
    else
    {
        imagePath=originalImagePath;
    }

    QImageReader imageReader(imagePath);

    if(!imageReader.canRead()){
        if(ui->listWidget->count()>1){
            QString currentPath;
            if(ui->listWidget->item(index)->toolTip().isEmpty()){
                currentPath=ui->listWidget->item(index)->statusTip();
            }
            else{
                currentPath=ui->listWidget->item(index)->toolTip();
            }
            delete ui->listWidget->item(index);
        }
        else{
            ui->listWidget->clear();
        }
        return false;
    }
    else
    {
        QImage img;
        if(haveToCache){
            /*
             * The reason that here a check for the dimensions of the image is not being done
             * (so as not to cache it if it is already too small) is because of this bug:
             * https://bugreports.qt-project.org/browse/QTBUG-24831
             * I could create a very small symlink to the target of the image but QFile::size
             * does not return the size of the symlink but the size of the actual file and
             * this creates problems with managing the size of the cache. Also, saving images
             * slightly smaller than CACHED_IMAGES_SIZE as resized (to CACHED_IMAGES_SIZE)
             * resulted in smaller images than the originals(!). Finally, in Preferences it
             * is clearly stated how many images (on average) there are per cache MiB (CACHE_IMAGES_PER_MiB),
             * so even if I could get the symlink's size then CACHE_IMAGES_PER_MiB would be wrong
             * (in favor of the user, but still wrong).
             */
            imageReader.setScaledSize(QSize(CACHED_IMAGES_SIZE));
            img=imageReader.read();
            if(img.save(gv.cachePath+cachedImagePath)){
                cacheSizeChanged(gv.cachePath+cachedImagePath);
            }
        }
        else
        {
            img.load(imagePath);
        }
        ui->listWidget->item(index)->setIcon(QIcon(QPixmap::fromImage(img)));
        ui->listWidget->item(index)->setStatusTip(originalImagePath);
        ui->listWidget->item(index)->setToolTip(tr("Right-click for options"));
        qApp->processEvents(QEventLoop::AllEvents);
        return true;
    }
}

void MainWindow::forceUpdateIconOf(int index){
    if(ui->listWidget->item(index)->statusTip().isEmpty()){
        //image icon has not been updated, anyway (it wasn't any time visible to the user), just delete it if it is cached
        QString cache_name = gv.cachePath+Global::originalToCacheName(ui->listWidget->item(index)->toolTip());
        if(QFile(cache_name).exists()){
            QFile::remove(cache_name);
        }
        return;
    }
    //update the image and save the cache of it
    QString original_name=ui->listWidget->item(index)->statusTip();
    QImage img(original_name);
    img=img.scaled(CACHED_IMAGES_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QString cache_full_path=gv.cachePath+Global::originalToCacheName(original_name);
    if(img.save(cache_full_path)){
        cacheSizeChanged(cache_full_path);
    }
    ui->listWidget->item(index)->setIcon(QIcon(QPixmap::fromImage(img)));
    if(gv.previewImagesOnScreen && ui->listWidget->currentRow()==index && ui->stackedWidget->currentIndex()==0){
        updateScreenLabel();
    }
}

void MainWindow::fixCacheSizeThreaded(){
    QtConcurrent::run(this, &MainWindow::fixCacheSizeGlobalCallback);
}

void MainWindow::fixCacheSizeGlobalCallback(){
    Global(false).fixCacheSize(currentFolder_);
}

void MainWindow::cacheSizeChanged(const QString &newCacheImage){
    if(Global::addToCacheSize(QFile(newCacheImage).size())){
        if(cacheSizeChecker_->isActive()){
            cacheSizeChecker_->stop();
            cacheSizeChecker_->start(CHECK_CACHE_SIZE_TIMEOUT);
        }
        cacheSizeChecker_->start(CHECK_CACHE_SIZE_TIMEOUT);
    }
}

void MainWindow::on_browse_folders_clicked()
{
    addDialogShown_=true;

    QString initial_folder=settings->value("last_opened_folder", "").toString();

    if(!QDir(initial_folder).exists() || initial_folder.isEmpty()){
        initial_folder=gv.homePath+"/"+gv.defaultPicturesLocation;
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
        on_listWidget_itemDoubleClicked();
    }
}

void MainWindow::delayed_pictures_location_change()
{
    ui->pictures_location_comboBox->setCurrentIndex(tempForDelayedPicturesLocationChange_);
}

void MainWindow::on_pictures_location_comboBox_currentIndexChanged(int index)
{
    if(searchIsOn_){
        on_search_close_clicked();
    }
    ui->search_box->clear();

    if(!monitoredFoldersList_.isEmpty())
    {
        monitoredFoldersList_.clear();
        resetWatchFolders();
        ui->listWidget->clear();
    }

    currentFolder_=ui->pictures_location_comboBox->itemData(index).toString();
    if(QDir(currentFolder_).exists())
    {
        monitor(Global::listFolders(currentFolder_, true, true));
        if ( ui->listWidget->count() < LEAST_WALLPAPERS_FOR_START ){
            startButtonsSetEnabled(false);
        }
        else{
            startButtonsSetEnabled(true);
        }
        settings->setValue("currentFolder_index", ui->pictures_location_comboBox->currentIndex());
        settings->sync();

        if(ui->listWidget->count()){
            ui->listWidget->setCurrentRow(0);
        }

        if(gv.wallpapersRunning){
            processRunningResetPictures(false);
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
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your Pictures folder."));
            }
            else if(index==1){
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your Pictures folder."));
            }
            else{
                currentFolderExists();
            }
        }
    }
}

void MainWindow::processRunningResetPictures(bool fromRandomImagesToNormal){
    /*
     * This function handles the case when the parent folder for the wallpapers is changed
     * or when the shuffle_images_checkbox is checked/unchecked whilw the wallpaper changing process
     * is running normally. If the shuffle_images_checkbox is checked it will generate random images
     * but if it is not then it has to know if it had to do with the folder changing or with the
     * unchecking of shuffle_images_checkbox
     */

    int listCount = ui->listWidget->count();
    if(fromRandomImagesToNormal){
        //go to the next image of the previous random image

        if(gv.currentRandomImageIndex==0){
            gv.currentRandomImageIndex=gv.randomImages.count()-1;
        }
        else{
            gv.currentRandomImageIndex--;
        }

        if(gv.randomImages[gv.currentRandomImageIndex]==(gv.randomImages.count()-1)){
            //the previous random image was at the end of the list, the new has to be at the beginning
            indexOfCurrentImage_=0;
        }
        else
        {
            indexOfCurrentImage_=gv.randomImages[gv.currentRandomImageIndex]+1;
        }
    }
    else
    {
        //folder has changed/updated
        if(ui->shuffle_images_checkbox->isChecked()){
            if(listCount<LEAST_WALLPAPERS_FOR_START){
                on_stopButton_clicked();//not enough pictures to continue
            }
            else
            {
                secondsLeft_=0;
                Global::generateRandomImages(listCount, -1);
            }
        }
        else if(listCount>=LEAST_WALLPAPERS_FOR_START)
        {
            indexOfCurrentImage_=0; //start from the beginning of the new folder
            secondsLeft_=0;
        }
        else{
            on_stopButton_clicked();//not enough pictures to continue
        }
    }
}

void MainWindow::updatePicturesLocations()
{
    // Updates the configuration of the pictures_locations

    settings->beginWriteArray("pictures_locations");
    settings->remove("");
    short count = ui->pictures_location_comboBox->count();
    for (short i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        settings->setValue("item", ui->pictures_location_comboBox->itemData(i));
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
        searchList_.clear();
        openCloseSearch_->setEndValue(0);
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

void MainWindow::refreshIntervals()
{
    customIntervals_.clear();
    gv.customIntervalsInSeconds.clear();
    short size=settings->beginReadArray("custom_intervals_in_seconds");
    settings->endArray();
    if(size==0)
    {
        gv.customIntervalsInSeconds=DEFAULT_INTERVALS_IN_SECONDS;
        customIntervals_ = gv.defaultIntervals;
        settings->beginWriteArray("custom_intervals_in_seconds");
        for (short i = 0; i < 17; ++i) {
            settings->setArrayIndex(i);
            settings->setValue("item", gv.customIntervalsInSeconds.at(i));
        }
        settings->endArray();
        settings->sync();
        size=17;
    }
    else
    {
        settings->beginReadArray("custom_intervals_in_seconds");
        for(short i=0; i<size; i++)
        {
            settings->setArrayIndex(i);
            int seconds=settings->value("item").toInt();
            gv.customIntervalsInSeconds << seconds;
            customIntervals_ << Global::secondsToMinutesHoursDays(seconds);
        }
        settings->endArray();
    }

    for(short i=0; i<size; i++)
    {
        if(customIntervals_.at(i)==ui->wallpapers_slider_time->text())
        {
            ui->timerSlider->setValue(i+1);
            break;
        }
    }

    ui->timerSlider->setMaximum(customIntervals_.count());
    on_timerSlider_valueChanged(ui->timerSlider->value());
}

//Live Earth code

void MainWindow::on_activate_livearth_clicked()
{
    if(!ui->activate_livearth->isEnabled()){
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
#endif //#ifdef ON_LINUX
        return;
    }
    closeEverythingThatsRunning(2);
    on_page_1_earth_clicked();
    startLivearth();
}

void MainWindow::startLivearth()
{
    ui->activate_livearth->setEnabled(false);
    ui->deactivate_livearth->setEnabled(true);
    gv.liveEarthRunning=true;
    Global::updateStartup();
#ifdef ON_LINUX
    Global::changeIndicatorSelection("earth");
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
#else
    Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX
    setProgressbarsValue(100);
    startUpdateSeconds();
    animateProgressbarOpacity(1);
}

void MainWindow::on_deactivate_livearth_clicked()
{
    if(!ui->deactivate_livearth->isEnabled()){
        return;
    }

    gv.liveEarthRunning=false;
    Global::updateStartup();
    secondsLeft_=0;
    processRequestStop();
#ifdef ON_LINUX
    Global::changeIndicatorSelection("none");
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
#else
    Q_EMIT signalUncheckRunningFeatureOnTray();
#endif //#ifdef ON_LINUX
    stopLivearth();
    ui->deactivate_livearth->setEnabled(false);
    ui->activate_livearth->setEnabled(true);
}

void MainWindow::stopLivearth(){
    globalParser_->abortDownload();
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(-1, 1);
    }
    animateProgressbarOpacity(0);
}

//Picture of they day code

void MainWindow::on_activate_potd_clicked()
{
    if(!ui->activate_potd->isEnabled()){
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
#endif //#ifdef ON_LINUX
        return;
    }
    closeEverythingThatsRunning(3);
    on_page_2_potd_clicked();
    startPotd(true);
}

void MainWindow::startPotd(bool launch_now){
    ui->deactivate_potd->setEnabled(true);
    ui->activate_potd->setEnabled(false);
    gv.potdRunning=true;
    Global::updateStartup();
    justUpdatedPotd_=false;
    if(launch_now){
        actionsOnWallpaperChange();
    }
    animateProgressbarOpacity(1);
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
    Global::changeIndicatorSelection("potd");
#else
    Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX
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
    globalParser_->abortDownload();
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    animateProgressbarOpacity(0);
    gv.potdRunning=false;
    Global::updateStartup();
    ui->activate_potd->setEnabled(true);
    ui->deactivate_potd->setEnabled(false);
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
    Global::changeIndicatorSelection("none");
#else
    Q_EMIT signalUncheckRunningFeatureOnTray();
#endif //#ifdef ON_LINUX
}

void MainWindow::restartPotdIfRunningAfterSettingChange(){
    if(!gv.potdRunning){
        settings->setValue("potd_preferences_have_changed", true);
        settings->sync();
        return;
    }
    Global::remove(gv.wallchHomePath+POTD_IMAGE+"*");
    on_deactivate_potd_clicked();
    settings->setValue("last_day_potd_was_set", "");
    settings->sync();
    startPotd(true);
}

void MainWindow::on_label_3_linkActivated()
{
    Global::openUrl("http://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day");
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
        QDir().mkpath(gv.wallchHomePath+"Wallpaper_clocks/");
    }

#ifdef ON_LINUX
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
#ifdef ON_LINUX
    if(unzipArchive_&& !unzipArchive_->isOpen()){
        unzipArchive_->deleteLater();
    }
#endif

    QFile file(currentWallpaperClockPath_+"/clock.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        Global::error("I could not open your clock.ini file successfully!");
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

    ui->activate_clock->setEnabled(true);
}

#ifdef ON_WIN32
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
        Global::error("Could not delete wallpaper clock, check folder's and subfolders' permissions.");
    }

    if(ui->clocksTableWidget->rowCount()==1){
        ui->activate_clock->setEnabled(false);
        if(gv.previewImagesOnScreen){
            changeTextOfScreenLabelTo(tr("No wallpaper clock has been\ninstalled to preview"));
        }
    }

    ui->clocksTableWidget->removeRow(ui->clocksTableWidget->currentRow());

    currentlyUninstallingWallpaperClock_=false;
}

void MainWindow::on_clocksTableWidget_customContextMenuRequested()
{
    clockwidgetMenu_ = new QMenu(this);
    clockwidgetMenu_->addAction(tr("Unistall"), this, SLOT(uninstall_clock()));
    clockwidgetMenu_->actions().at(0)->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/Pictures/list-remove.svg")));
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
        Global::wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                         ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
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
            imageTransition("preview_clock");
        }
    }
}

void MainWindow::on_activate_clock_clicked()
{
    closeEverythingThatsRunning(4);
    if(!wallpaperClocksPageLoaded_){
        loadWallpaperClocksPage();
    }
    if(!ui->activate_clock->isEnabled()){
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
#endif //#ifdef ON_LINUX
        globalParser_->desktopNotify(tr("No default wallpaper clock has been set."), false, "info");
        Global::error("No default wallpaper clock has been set. Make sure that after installing the wallpaper clock you have clicked at set as default.");
        return;
    }

    on_page_3_clock_clicked();

    if(ui->minutes_checkBox->isChecked() || ui->hour_checkBox->isChecked() || (ui->am_checkBox->isChecked() && gv.amPmEnabled) ||
            ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked())
    {
        Global::readClockIni(gv.defaultWallpaperClock);
        QString currentWallpaperClock = Global::wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                         ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
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
        else{
            seconds_left_for_min=time.secsTo(QTime(time.hour(),time.minute()+1,1));
        }

        ui->activate_clock->setEnabled(false);
        ui->deactivate_clock->setEnabled(true);

        gv.wallpaperClocksRunning=true;
        Global::updateStartup();
#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome){
            dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
        }
        Global::changeIndicatorSelection("clocks");
#else
        Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX

        if(ui->minutes_checkBox->isChecked())
        {
            secondsLeft_=seconds_left_for_min;
        }
        else if(ui->hour_checkBox->isChecked())
        {
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
        else if(ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked())
        {
            float seconds_left_for_next_day=time.secsTo(QTime(23,59,59))+2;
            secondsLeft_=seconds_left_for_next_day;
        }
        else if(ui->month_checkBox->isChecked())
        {
            float seconds_left_for_next_day=time.secsTo(QTime(23,59,59))+2;
            secondsLeft_=seconds_left_for_next_day;
        }

        totalSeconds_=secondsLeft_;
        Global::resetSleepProtection(secondsLeft_);
        startUpdateSeconds();
        setProgressbarsValue(100);
        animateProgressbarOpacity(1);
    }
    else
    {
        if (QMessageBox::question(this,tr("Wallpaper Clock"), tr("You haven't selected any of the options (day of week etc...), thus the wallpaper clock will not be activated. You can use the wallpaper of the default wallpaper clock as a background, instead.")) == QMessageBox::Yes)
        {
            QString pic=gv.defaultWallpaperClock+"/bg.jpg";
            Global::setBackground(pic, true, true, 4);
        }
    }
}

void MainWindow::on_deactivate_clock_clicked()
{
    if(!ui->deactivate_clock->isEnabled()){
        return;
    }
    ui->deactivate_clock->setEnabled(false);
    ui->activate_clock->setEnabled(true);
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    animateProgressbarOpacity(0);
    gv.wallpaperClocksRunning=false;
    Global::updateStartup();

#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
    Global::changeIndicatorSelection("none");
#else
    Q_EMIT signalUncheckRunningFeatureOnTray();
#endif //#ifdef ON_LINUX

    secondsLeft_=0;
}

QImage *MainWindow::wallpaperClocksPreview(const QString &wallClockPath, bool minuteCheck, bool hourCheck, bool amPmCheck, bool dayWeekCheck, bool dayMonthCheck, bool monthCheck)
{
    short hour_inte=0;
    short am_pm_en=0;
    short images_of_hour=0;

    //reading clock.ini of wallpaper clock in order to get the gv.refreshhourinterval,if there are images for am or pm and the total of hour images
    QFile file(wallClockPath+"/clock.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        Global::error("I could not open the clock.ini file ("+wallClockPath+"/clock.ini) for reading successfully!");
        return NULL;
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
        return NULL;
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

    QImage *merged = new QImage(wallClockPath+"/bg.jpg");

    QPainter painter(merged);
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

    return merged;
}

void MainWindow::clockCheckboxClickedWhileRunning()
{
    if(ui->minutes_checkBox->isChecked() || ui->hour_checkBox->isChecked() || (ui->am_checkBox->isChecked() && gv.amPmEnabled) || ui->day_week_checkBox->isChecked() || ui->day_month_checkBox->isChecked() || ui->month_checkBox->isChecked())
    {
        QString currentWallpaperClock = Global::wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                         ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
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
        QString currentWallpaperClock = Global::wallpaperClockNow(gv.defaultWallpaperClock, ui->minutes_checkBox->isChecked(), ui->hour_checkBox->isChecked(), ui->am_checkBox->isChecked(),
                                                         ui->day_week_checkBox->isChecked(), ui->day_month_checkBox->isChecked(), ui->month_checkBox->isChecked());
        if(gv.setAverageColor){
            setAverageColor(currentWallpaperClock);
        }
        on_deactivate_clock_clicked();
    }
}

void MainWindow::on_label_9_linkActivated()
{
    Global::openUrl("http://www.vladstudio.com/wallpaperclocks/browse.php");
}

void MainWindow::clockCheckboxClicked()
{
    imageTransition("preview_clock");

    settings->setValue("month", ui->month_checkBox->isChecked());
    settings->setValue("day_of_month", ui->day_month_checkBox->isChecked());
    settings->setValue("day_of_week", ui->day_week_checkBox->isChecked());
    settings->setValue("am_pm", ui->am_checkBox->isChecked());
    settings->setValue("hour", ui->hour_checkBox->isChecked());
    settings->setValue("minute", ui->minutes_checkBox->isChecked());

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
    closeEverythingThatsRunning(5);
    if(!liveWebsitePageLoaded_){
        loadLiveWebsitePage();
    }
    if(websiteSnapshot_==NULL || !ui->activate_website->isEnabled() || !websiteConfiguredCorrectly()){
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
#endif
        return;
    }

    gv.websiteWebpageToLoad=gv.onlineLinkForHistory=ui->website->text();
    gv.websiteInterval=ui->website_slider->value();
    gv.websiteCropEnabled=ui->website_crop_checkbox->isChecked();

    on_page_4_web_clicked();
    ui->stackedWidget->setCurrentIndex(4);

    ui->deactivate_website->setEnabled(true);
    ui->activate_website->setEnabled(false);
    ui->verticalLayout_11->addWidget(ui->timeout_text_label);
    ui->verticalLayout_11->addWidget(ui->website_timeout_label);
    ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT)+" "+tr("seconds")+"...");
    ui->timeout_text_label->show();
    ui->website_timeout_label->show();
    timePassedForLiveWebsiteRequest_=1;
    ui->live_website_login_widget->setEnabled(false);

    gv.liveWebsiteRunning=true;
    Global::updateStartup();
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    }
    Global::changeIndicatorSelection("website");
#else
    Q_EMIT signalRecreateTray();
#endif //#ifdef ON_LINUX
    setProgressbarsValue(100);
    animateProgressbarOpacity(1);

    prepareWebsiteSnapshot();
    websiteSnapshot_->setCrop(ui->website_crop_checkbox->isChecked(), gv.websiteCropArea);
    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    startUpdateSeconds();
}

void MainWindow::on_deactivate_website_clicked()
{
    if(!ui->deactivate_website->isEnabled()){
        return;
    }
    gv.liveWebsiteRunning=false;
    Global::updateStartup();
    secondsLeft_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(-1, 2);
    }
    ui->live_website_login_widget->setEnabled(true);
    animateProgressbarOpacity(0);
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, false);
    }
    Global::changeIndicatorSelection("none");
#else
    Q_EMIT signalUncheckRunningFeatureOnTray();
#endif //#ifdef ON_LINUX
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
        Global::remove(gv.wallchHomePath+LW_IMAGE+"*");
        QString filename=gv.wallchHomePath+LW_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+".png";
        image->save(filename);
        delete image;

        Global::setBackground(filename, true, true, 5);
        QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        QFile(filename).link(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        updateScreenLabel();
    }
    else
    {
        switch(errorCode){
            case 1:
                Global::error("Some of the requested pages failed to load successfully.");
                break;
            case 2:
                Global::error("Simple authentication failed. Please check your username and/or password.");
                break;
            case 3:
                Global::error("Username and/or password fields are not found. Please check that you are pointing at the login page.");
                break;
            case 4:
                Global::error("The timeout has been reached and the image has yet to be created!");
                break;
            default:
                Global::error("Unknown error! Please try with a different web page.");
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

//Pages code

void MainWindow::loadWallpapersPage(){
    wallpapersPageLoaded_=true;

    HideGrayLinesDelegate *delegate = new HideGrayLinesDelegate(ui->listWidget);
    ui->listWidget->setItemDelegate(delegate);

    ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
    imageLoading_=QIcon::fromTheme("image-loading", QIcon(":icons/Pictures/image-loading.svg"));

    ui->pictures_location_comboBox->setItemText(0, gv.currentOSName+" "+tr("Desktop Backgrounds"));
    ui->pictures_location_comboBox->setItemData(0, gv.currentDeDefaultWallpapersPath);
    ui->pictures_location_comboBox->setItemText(1, tr("My Pictures"));
    ui->pictures_location_comboBox->setItemData(1, gv.homePath+"/"+gv.defaultPicturesLocation);

    short size=settings->beginReadArray("pictures_locations");
    for(short i=2;i<size;i++){
        settings->setArrayIndex(i);
        QString qpath=settings->value("item").toString();
        ui->pictures_location_comboBox->addItem(Global::basenameOf(qpath));
        ui->pictures_location_comboBox->setItemData(ui->pictures_location_comboBox->count()-1, qpath);
    }
    settings->endArray();

    short currentFolder = settings->value("currentFolder_index", 0).toInt();
    currentFolder_=ui->pictures_location_comboBox->itemData(currentFolder).toString();
    if(!QDir(currentFolder_).exists())
    {
        tempForDelayedPicturesLocationChange_=currentFolder;
        QTimer::singleShot(100, this, SLOT(delayed_pictures_location_change()));
    }
    else
    {
        if(currentFolder==0){
            on_pictures_location_comboBox_currentIndexChanged(0);
        }
        else
        {
            ui->pictures_location_comboBox->setCurrentIndex(currentFolder);
        }
    }

    refreshIntervals();

    short old_value=ui->timerSlider->value();
    ui->timerSlider->setValue(settings->value("timeSlider", 7).toInt());
    if(old_value==ui->timerSlider->value()){
        on_timerSlider_valueChanged(old_value);
    }
    launchTimerToUpdateIcons();

    openCloseSearch_ = new QPropertyAnimation(ui->search_widget, "maximumHeight");
    connect(openCloseSearch_, SIGNAL(finished()), this, SLOT(openCloseSearchAnimationFinished()));
    openCloseSearch_->setDuration(GENERAL_ANIMATION_SPEED);

    cacheSizeChecker_->start(CHECK_CACHE_SIZE_TIMEOUT);
}

void MainWindow::loadPotdPage(){
    potdPageLoaded_=true;
    ui->include_description_checkBox->setChecked(gv.potdIncludeDescription);
}

void MainWindow::loadWallpaperClocksPage()
{
    HideGrayLinesDelegate *delegate = new HideGrayLinesDelegate(ui->clocksTableWidget);
    ui->clocksTableWidget->setItemDelegate(delegate);

    ui->clocksTableWidget->setColumnWidth(0, 20);

    //add to the listwidget the wallpaper clocks that exist in ~/.wallch/Wallpaper_clocks (AppData/Local/Mellori Studio/Wallch/Wallpaper_clocks for Windows)
#ifdef ON_LINUX
    unzipArchive_=NULL;
#endif
    Q_FOREACH(currentWallpaperClockPath_, Global::listFolders(gv.wallchHomePath+"Wallpaper_clocks", true, false))
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

    ui->activate_clock->setEnabled(clocksCount!=0);

    ui->month_checkBox->setChecked(settings->value("month",true).toBool());
    ui->day_month_checkBox->setChecked(settings->value("day_of_month",true).toBool());
    ui->day_week_checkBox->setChecked(settings->value("day_of_week",true).toBool());
    ui->am_checkBox->setChecked(settings->value("am_pm",true).toBool());
    ui->hour_checkBox->setChecked(settings->value("hour",true).toBool());
    ui->minutes_checkBox->setChecked(settings->value("minute",true).toBool());

    wallpaperClocksPageLoaded_=true;
}

void MainWindow::loadLiveWebsitePage(){
    liveWebsitePageLoaded_=true;
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
        gv.websiteLoginPasswd=Global::base64Decode(gv.websiteLoginPasswd);
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
    changedWebPage_=initialWebPage_=ui->website->text();
    if(websiteSnapshot_!=NULL){
        return;
    }
    websiteSnapshot_ = new WebsiteSnapshot();
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

    if(!wallpapersPageLoaded_){

        loadWallpapersPage();

        if(ui->listWidget->count()<=1){
            startButtonsSetEnabled(false);
        }
        else
        {
            startButtonsSetEnabled(true);
        }
        if(gv.randomTimeEnabled){
            ui->timerSlider->setEnabled(false);
            ui->wallpapers_slider_time->setEnabled(false);
            ui->interval_label->setEnabled(false);
        }
        else
        {
            ui->timerSlider->setEnabled(true);
            ui->wallpapers_slider_time->setEnabled(true);
            ui->interval_label->setEnabled(true);
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

    ui->page_2_potd->setChecked(true);
    ui->stackedWidget->setCurrentIndex(2);
    ui->page_2_potd->raise();
    ui->sep2->raise();
    ui->sep3->raise();
    if(!potdPageLoaded_){
        loadPotdPage();
    }
    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_3_clock_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==3){
        return;
    }

    ui->page_3_clock->setChecked(true);
    ui->stackedWidget->setCurrentIndex(3);
    ui->page_3_clock->raise();
    ui->sep3->raise();
    ui->sep4->raise();

    if(!wallpaperClocksPageLoaded_){
        loadWallpaperClocksPage();
    }
    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
}

void MainWindow::on_page_4_web_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==4){
        return;
    }

    ui->page_4_web->setChecked(true);
    ui->stackedWidget->setCurrentIndex(4);
    ui->page_4_web->raise();
    ui->sep4->raise();
    ui->sep5->raise();
    if(gv.previewImagesOnScreen){
        QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
    }
    if(!liveWebsitePageLoaded_){
        loadLiveWebsitePage();
    }
}

void MainWindow::on_page_5_other_clicked()
{
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==5){
        return;
    }

    ui->page_5_other->setChecked(true);
    ui->stackedWidget->setCurrentIndex(5);
    ui->page_5_other->raise();
    ui->sep5->raise();
    if(gv.previewImagesOnScreen){
        updateScreenLabel();
    }
}

void MainWindow::previousPage(){
    short currentPage = ui->stackedWidget->currentIndex()-1;
    if(currentPage<0){
        currentPage=5;
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
    if(currentPage>6){
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

void MainWindow::on_actionCopy_Name_triggered()
{
    QApplication::clipboard()->setText(Global::basenameOf(Global::currentBackgroundImage()));
}

void MainWindow::on_actionCopy_Path_triggered()
{
    QApplication::clipboard()->setText(Global::currentBackgroundImage());
}

void MainWindow::on_actionCopy_Image_triggered()
{
    QApplication::clipboard()->setImage(QImage(Global::currentBackgroundImage()), QClipboard::Clipboard);
}

void MainWindow::on_actionDelete_triggered()
{
    if(QMessageBox::question(0, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the current image?"))==QMessageBox::Yes)
    {
        if(!QFile::remove(Global::currentBackgroundImage())){
            QMessageBox::warning(0, tr("Error"), tr("There was a problem deleting the current image. Please make sure you have the permission to delete the image or that the image exists."));
        }
    }
}

void MainWindow::on_actionProperties_triggered()
{
    if(propertiesShown_){
        return;
    }
    QString imageFilename=Global::currentBackgroundImage();
    if(!QFile::exists(imageFilename) || QImage(imageFilename).isNull())
    {
        QMessageBox::warning(0, tr("Properties"), tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
        return;
    }

    propertiesShown_=true;
    properties_ = new Properties(imageFilename, false, 0, 0);
    properties_->setModal(true);
    properties_->setAttribute(Qt::WA_DeleteOnClose);
    connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
    properties_->show();
}

void MainWindow::on_actionOpen_Folder_triggered()
{
    Global::openFolderOf();
}

void MainWindow::on_actionQuit_Ctrl_Q_triggered()
{
    actionsOnClose();
    Global::debug("See ya next time!");
    qApp->exit(0);
}

void MainWindow::on_actionContents_triggered()
{
    Global::openUrl(HELP_URL);
}

void MainWindow::on_actionDonate_triggered()
{
    Global::openUrl(DONATE_URL);
}

void MainWindow::on_actionDownload_triggered()
{
    Global::openUrl("http://melloristudio.com/wallch/1000-HD-Wallpapers");
}

void MainWindow::on_actionReport_A_Bug_triggered()
{
    Global::openUrl("https://bugs.launchpad.net/wallpaper-changer/+filebug");
}

void MainWindow::on_actionGet_Help_Online_triggered()
{
    Global::openUrl("https://answers.launchpad.net/wallpaper-changer/+addquestion");
}

void MainWindow::on_actionWhat_is_my_screen_resolution_triggered()
{
    QMessageBox msgBox;msgBox.setWindowTitle(tr("What is my Screen Resolution"));
    msgBox.setText("<b>"+tr("Your screen resolution is")+" <span style=\" font-size:20pt;\">"+
                   QString::number(gv.screenWidth)+"</span><span style=\" font-size:15pt;\">x</span><span style=\" font-size:20pt;\">"+
                   QString::number(gv.screenHeight)+"</span>.</b><br>"+tr("The number above represents your screen/monitor resolution (In width and height)."));
    msgBox.setIconPixmap(QIcon(":/icons/Pictures/monitor320x200.png").pixmap(QSize(100,100)));
    msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
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
    statistics_->setWindowFlags(Qt::Window);
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
    history_->setWindowFlags(Qt::Window);
    connect(history_, SIGNAL(destroyed()), this, SLOT(historyDestroyed()));
    history_->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    history_->show();
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
    about_->setWindowFlags(Qt::Window);
    connect(about_, SIGNAL(destroyed()), this, SLOT(aboutDestroyed()));
    about_->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    about_->show();
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
    preferences_->setWindowFlags(Qt::Window);
    connect(preferences_, SIGNAL(destroyed()), this, SLOT(preferencesDestroyed()));
    connect(preferences_, SIGNAL(changePathsToIcons()), this, SLOT(changePathsToIcons()));
    connect(preferences_, SIGNAL(changeIconsToPaths()), this, SLOT(changeIconsToPaths()));
#ifdef ON_LINUX
    connect(preferences_, SIGNAL(bindKeySignal(QString)), this, SLOT(bindKey(const QString&)));
    connect(preferences_, SIGNAL(unbindKeySignal(QString)), this, SLOT(unbindKey(const QString&)));
#endif //#ifdef ON_LINUX
    connect(preferences_, SIGNAL(changeThemeTo(QString)), this, SLOT(changeCurrentThemeTo(const QString&)));
    connect(preferences_, SIGNAL(changeRandomTime(short)), this, SLOT(random_time_changed(short)));
    connect(preferences_, SIGNAL(refreshCustomIntervals()), this, SLOT(refreshIntervals()));
    connect(preferences_, SIGNAL(tvPreviewChanged(bool)), this, SLOT(tvPreview(bool)));
#ifdef ON_LINUX
    connect(preferences_, SIGNAL(unityProgressbarChanged(bool)), this, SLOT(unityProgressbarSetEnabled(bool)));
#endif //#ifdef ON_LINUX
    preferences_->show();
}

void MainWindow::preferencesDestroyed(){
    gv.preferencesDialogShown=false;
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
    if(propertiesShown_ || !ui->listWidget->currentItem()->isSelected() || ui->stackedWidget->currentIndex()!=0){
        return;
    }
    QString currentImage;
    if(gv.iconMode){
        if(ui->listWidget->currentItem()->statusTip().isEmpty()){
            currentImage=ui->listWidget->currentItem()->toolTip();
        }
        else{
            currentImage=ui->listWidget->currentItem()->statusTip();
        }
    }
    else{
        currentImage=ui->listWidget->currentItem()->text();
    }
    if(QImage(currentImage).isNull())
    {
        QMessageBox::warning(this, tr("Properties"), tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
        return;
    }
    else
    {
        propertiesShown_=true;
        properties_ = new Properties(currentImage, true, ui->listWidget->currentRow(), this);

        properties_->setModal(true);
        properties_->setAttribute(Qt::WA_DeleteOnClose);
        properties_->setWindowFlags(Qt::Window);
        connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
        connect(properties_, SIGNAL(requestNext(int)), this, SLOT(sendPropertiesNext(int)));
        connect(properties_, SIGNAL(requestPrevious(int)), this, SLOT(sendPropertiesPrevious(int)));
        connect(properties_, SIGNAL(newAverageColor(QString)), this, SLOT(setAverageColor(QString)));
        connect(this, SIGNAL(givePropertiesRequest(QString, int)), properties_, SLOT(updateEntries(QString, int)));
        properties_->show();
    }
}

void MainWindow::propertiesDestroyed(){
    propertiesShown_=false;
}

void MainWindow::sendPropertiesNext(int current){
    int listCount=ui->listWidget->count();
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
    int listCount=ui->listWidget->count();
    if(current==0){
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
    gv.potdIncludeDescription=checked;
    ui->edit_potd->setEnabled(checked);
    settings->setValue("potd_include_description", checked);
    settings->sync();

    restartPotdIfRunningAfterSettingChange();
}

void MainWindow::on_edit_potd_clicked()
{
    potdPreviewShown_=true;
    potdPreview_ = new PotdPreview(this);
    potdPreview_->setModal(true);
    potdPreview_->setAttribute(Qt::WA_DeleteOnClose);
    potdPreview_->setWindowFlags(Qt::Window);
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
    settings->setValue("current_page" , page);
}

void MainWindow::update_website_settings()
{
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
}

void MainWindow::on_donateButton_clicked()
{
    Global::openUrl(DONATE_URL);
}

void MainWindow::on_melloristudio_link_label_linkActivated(const QString &link)
{
    Global::openUrl(link);
}
