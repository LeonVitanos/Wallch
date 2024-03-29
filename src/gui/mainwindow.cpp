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
#include <QRadioButton>
#include <QFileDialog>
#include <QNetworkReply>
#include <QScreen>
#include <QScrollBar>
#include <QStandardItem>
#include <QColorDialog>
#include <QShortcut>

#include "mainwindow.h"
#include "math.h"
#include "desktopenvironment.h"

MainWindow *mainWindowInstance;

MainWindow::MainWindow(QSharedMemory *attachedMemory, Global *globalParser, ImageFetcher *imageFetcher,
                       WebsiteSnapshot *websiteSnapshot, WallpaperManager *wallpaperManager,
                       TimerManager *timerManager, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWindowInstance = this;

    attachedMemory_ = attachedMemory;

    initializePrivateVariables(globalParser, imageFetcher);
    wallpaperManager_ = (wallpaperManager == NULL) ? new WallpaperManager() : wallpaperManager;
    timerManager_ = (timerManager == NULL) ? new TimerManager() : timerManager;
    websiteSnapshot_ = websiteSnapshot;

    getScreenResolution(QGuiApplication::primaryScreen()->geometry());
    getScreenAvailableResolution(QGuiApplication::primaryScreen()->availableGeometry());
    imagePreviewResizeFactorX_ = SCREEN_LABEL_SIZE_X*2.0/(gv.screenAvailableWidth*1.0);
    imagePreviewResizeFactorY_ = SCREEN_LABEL_SIZE_Y*2.0/(gv.screenAvailableHeight*1.0);

    btn_group = new QButtonGroup(this);
    btn_group->addButton(ui->page_0_wallpapers, 0);
    btn_group->addButton(ui->page_1_earth, 1);
    btn_group->addButton(ui->page_2_potd, 2);
    btn_group->addButton(ui->page_3_clock, 3);
    btn_group->addButton(ui->page_4_web, 4);
    btn_group->addButton(ui->page_5_other, 5);

    setupMenu();
    connectSignalSlots();
    setupTimers();
    setupKeyboardShortcuts();
    changeCurrentTheme();
    applySettings();
    continueAlreadyRunningFeature();
    setAcceptDrops(true);

    currentShading = ColorManager::getColoringType();

    // Restore window's position and size
    if(settings->value("x").isValid()){
        this->move(QPoint(settings->value("x").toInt(), settings->value("y").toInt()));
        this->resize(QSize(settings->value("width").toInt(), settings->value("height").toInt()));
    }
    else
        this->move(QPoint(gv.screenAvailableWidth/2, gv.screenAvailableHeight/2) - this->rect().center());

    //processing request (loading gif)
    processingOnlineRequest_ = false;
    ui->process_request_label->hide();
    processingRequestGif_ = new QMovie(this);
    processingRequestGif_->setFileName(":/images/process_request.gif");
    ui->process_request_label->setMovie(processingRequestGif_);

    //for manually searching for files or re-selecting a picture after a folder contents have changed
    match_ = new QRegularExpression();
    match_->setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    QTimer::singleShot(10, this, SLOT(setButtonColor()));
    QTimer::singleShot(20, this, SLOT(findAvailableWallpaperStyles()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    this->hide();
    event->ignore();
}

void MainWindow::actionsOnClose()
{
    settings->setValue("x", this->x());
    settings->setValue("y", this->y());
    settings->setValue("width", this->width());
    settings->setValue("height", this->height());
    settings->sync();

    if(loadedPages_[0])
        savePicturesLocations();

    appAboutToClose_ = true;
    this->hide();
}

void MainWindow::resizeEvent(QResizeEvent *e){
    if(!gv.mainwindowLoaded)
        return;

    if(ui->stackedWidget->currentIndex() == 0)
        launchTimerToUpdateIcons();

    ui->screen_label->update();
    ui->screen_label_transition->update();
    ui->screen_label_text->update();

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
    QPoint showPoint = globalCoords - QPoint((menuWidth-buttonWidth), 0);
    QRect settingsMenuRect(showPoint.x(), showPoint.y(), menuWidth, menuHeight);

    if(availableGeometry_.contains(settingsMenuRect, true))
        return showPoint;
    else
    {
        QRect intersection = availableGeometry_.intersected(settingsMenuRect);

        if(intersection.width() < menuWidth){ // Width doesn't fit
            if(intersection.x() == 0) //it doesn't fit to the left, move to the right
                showPoint.setX(0);
            else //it doesn't fit to the right, move to the left
                showPoint.setX(availableGeometry_.width()-menuWidth);
        }

        if(intersection.height() < menuHeight){ //height doesn't fit
            if((intersection.y()-buttonHeight-availableGeometry_.y()) > menuHeight) //it fits to the top just fine
                showPoint.setY(globalCoords.y()-buttonHeight-menuHeight);
            else //it doesn't fit all above, fit it to the left/right
            {
                showPoint.setY(availableGeometry_.y()+availableGeometry_.height()-menuHeight);
                if(intersection.x() - buttonWidth >= 0) //fit to left
                    showPoint.setX(intersection.x()-buttonWidth);
                else //fit to right
                    showPoint.setX(globalCoords.x()+buttonWidth);
            }
        }
    }

    return showPoint;
}

void MainWindow::hideOrShow()
{
    if (this->isHidden())
        showNormal();
    else
        this->hide();
}

void MainWindow::connectSignalSlots(){
    connect(imageFetcher_, SIGNAL(fail()), this, SLOT(onlineRequestFailed()));
    connect(imageFetcher_, SIGNAL(success(QString)), this, SLOT(onlineImageRequestReady(QString)));
    connect(cacheManager_, SIGNAL(requestCurrentFolders()), this, SLOT(beginFixCacheForFolders()));
    connect(ui->website, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->website_slider, SIGNAL(valueChanged(int)), this, SLOT(update_website_settings()));
    connect(ui->website_crop_checkbox, SIGNAL(clicked()), this, SLOT(update_website_settings()));
    connect(ui->add_login_details, SIGNAL(clicked()), this, SLOT(update_website_settings()));
    connect(ui->username, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->password, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->final_webpage, SIGNAL(textChanged(QString)), this, SLOT(update_website_settings()));
    connect(ui->wallpapersList->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(launchTimerToUpdateIcons()));
    connect(scaleWatcher_, SIGNAL(finished()), this, SLOT(setPreviewImage()));
    connect(settingsMenu_, SIGNAL(aboutToHide()), this, SLOT(unhoverMenuButton()));
    connect(ui->days_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->hours_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->minutes_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(ui->seconds_spinBox, SIGNAL(valueChanged(int)), this, SLOT(timeSpinboxChanged()));
    connect(btn_group, SIGNAL(buttonClicked(int)), this, SLOT(page_button_clicked(int)));
    connect(wallpaperManager_, SIGNAL(updateImageStyle()), this, SLOT(updateImageStyleCombo()));
    connect(fileManager_, SIGNAL(prepareToSearchFolders()), this, SLOT(prepareToSearchFolders()));
    connect(fileManager_, SIGNAL(monitoredFoldersChanged()), this, SLOT(monitoredFoldersUpdated()));
    connect(fileManager_, SIGNAL(currentFolderDoesNotExist()), this, SLOT(currentFolderDoesNotExist()));
    connect(fileManager_, SIGNAL(addFilesToWallpapers(QString)), this, SLOT(addFilesToWallpapers(QString)));
    connect(QGuiApplication::primaryScreen(), SIGNAL(geometryChanged(QRect)), this, SLOT(getScreenResolution(QRect)));
    connect(QGuiApplication::primaryScreen(), SIGNAL(availableGeometryChanged(QRect)), this, SLOT(getScreenAvailableResolution(QRect)));

#ifdef Q_OS_LINUX
    dconf = new QProcess(this);
    connect(dconf, SIGNAL(readyReadStandardOutput()), this, SLOT(dconfChanges()));
    connect(dconf , SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(dconfChanges()));
    dconf->start("dconf watch /");
#endif
}

void MainWindow::setupTimers(){
    //Timer to updates progressbar and launch the changing process
    updateSecondsTimer_ = new QTimer(this);
    updateSecondsTimer_->setTimerType(Qt::VeryCoarseTimer);
    connect(updateSecondsTimer_, SIGNAL(timeout()), this, SLOT(updateSeconds()));

    //Timer that updates the configuration for timing once it is changed
    updateCheckTime_ = new QTimer(this);
    connect(updateCheckTime_, SIGNAL(timeout()), this, SLOT(updateTiming()));

    //Timer that update the icons of the visible items
    iconUpdater_ = new QTimer(this);
    connect(iconUpdater_, SIGNAL(timeout()), this, SLOT(updateVisibleIcons()));
    iconUpdater_->setSingleShot(true);

    //Timer to hide progressbar
    hideProgress_ = new QTimer(this);
    connect(hideProgress_, SIGNAL(timeout()), this, SLOT(hideTimeForNext()));
    hideProgress_->setSingleShot(true);
#ifdef Q_OS_LINUX
    //Timer to check battery status after laptop is unplugged
    batteryStatusChecker_ = new QTimer(this);
    connect(batteryStatusChecker_, SIGNAL(timeout()), this, SLOT(checkBatteryStatus()));
#endif
}

void MainWindow::setupKeyboardShortcuts(){
    (void) new QShortcut(Qt::Key_Escape, this, SLOT(escapePressed()));
    (void) new QShortcut(Qt::Key_Delete, this, SLOT(deletePressed()));
    (void) new QShortcut(Qt::ALT + Qt::Key_Return, this, SLOT(showProperties()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(showHideSearchBox()));
    (void) new QShortcut(Qt::Key_Return, this, SLOT(enterPressed()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousPage()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextPage()));
    (void) new QShortcut(Qt::ALT + Qt::Key_F4, this, SLOT(escapePressed()));
}

#ifdef Q_OS_LINUX
void MainWindow::dconfChanges(){
    QByteArray output = dconf->readAllStandardOutput();
    if(output.contains("theme"))
        changeCurrentTheme();
    else if(output.contains("picture-options"))
        updateImageStyleCombo();
    else if(output.contains("primary-color"))
    {
        qDebug() << "primary-color";
    }
    else if(output.contains("secondary-color"))
    {
        qDebug() << "econdary-color";
    }
    else if(output.contains("color-shading-type"))
        colorManager_->changeCurrentShading();
}
#endif

void MainWindow::changeCurrentTheme(){
    int theme = colorManager_->getCurrentTheme();

    QFile file(QString(":/themes/") + (theme == 0 ? "ambiance" : "radiance") + ".qss");
    if(!file.open(QIODevice::ReadOnly)){
        Global::error("Could not load the current theme file.");
        return;
    }

    qApp->setStyleSheet(QString::fromLatin1(file.readAll()));
    file.close();

    Q_FOREACH(QLabel *separator, menuSeparators_){
        separator->show();
        separator->setPixmap(theme == 0 ? AMBIANCE_SEPARATOR : RADIANCE_SEPARATOR);
    }
}

void MainWindow::initializePrivateVariables(Global *globalParser, ImageFetcher *imageFetcher){
    globalParser_ = (globalParser_ == NULL) ? new Global() : globalParser;
    imageFetcher_ = (imageFetcher == NULL) ? new ImageFetcher() : imageFetcher;
    cacheManager_ = new CacheManager();
    opacityEffect_ = new QGraphicsOpacityEffect(this);
    opacityEffect2_ = new QGraphicsOpacityEffect(this);
    scaleWatcher_ = new QFutureWatcher<QImage>(this);
    fileManager_ = new FileManager();
    dialogHelper_ = new DialogHelper();

    increaseOpacityAnimation = new QPropertyAnimation();
    increaseOpacityAnimation->setTargetObject(opacityEffect_);
    increaseOpacityAnimation->setPropertyName("opacity");
    increaseOpacityAnimation->setDuration(IMAGE_TRANSITION_SPEED);
    increaseOpacityAnimation->setEndValue(1);
    increaseOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);

    decreaseOpacityAnimation = new QPropertyAnimation();
    decreaseOpacityAnimation->setTargetObject(opacityEffect2_);
    decreaseOpacityAnimation->setPropertyName("opacity");
    decreaseOpacityAnimation->setDuration(IMAGE_TRANSITION_SPEED);
    decreaseOpacityAnimation->setEndValue(0);
    decreaseOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

void MainWindow::setupMenu()
{
    //We decided this is better than having a menubar
    settingsMenu_ = new QMenu(this);
    currentBgMenu_ = new QMenu(this);
    settingsMenu_->addAction(tr("Preferences"), this, SLOT(on_action_Preferences_triggered()), QKeySequence(tr("Ctrl+P")));
    settingsMenu_->addSeparator();
    currentBgMenu_->setTitle(tr("Current Background"));
    currentBgMenu_->addAction(tr("Open Image"), this, wallpaperManager_->openCurrentBackgroundImage);
    currentBgMenu_->addAction(tr("Open Folder"), this, wallpaperManager_->openCurrentBackgroundFolder);
    currentBgMenu_->addAction(tr("Copy Image"), this, wallpaperManager_->copyCurrentBackgroundImage);
    currentBgMenu_->addAction(tr("Copy Path"), this, wallpaperManager_->copyCurrentBackgroundPath);
    currentBgMenu_->addAction(tr("Delete"), this, wallpaperManager_->deleteCurrentBackgroundImage);
    currentBgMenu_->addAction(tr("Properties"), dialogHelper_, SLOT(showPropertiesDialog()));
    settingsMenu_->addMenu(currentBgMenu_);
    settingsMenu_->addSeparator();
    settingsMenu_->addAction(tr("History"), this, SLOT(on_actionHistory_triggered()), QKeySequence(tr("Ctrl+H")));
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
    int maxCacheIndex = settings->value("max_cache_size", 2).toInt();
    if(maxCacheIndex < 0 || maxCacheIndex >= cacheManager_->maxCacheIndexes.size())
        maxCacheIndex = 2;

    cacheManager_->setMaxCache(cacheManager_->maxCacheIndexes.value(maxCacheIndex).second);

    if(gv.iconMode){
        ui->wallpapersList->setIconSize(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->wallpapersList->setUniformItemSizes(true);
    }
    else
        ui->wallpapersList->setViewMode(QListView::ListMode);

    timerManager_->secondsInWallpapersSlider_ = settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();

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

    if(gv.randomImagesEnabled)
        srand(time(0));

    ui->shuffle_images_checkbox->setChecked(gv.randomImagesEnabled);
}

void MainWindow::continueAlreadyRunningFeature()
{   
    if(gv.wallpapersRunning)
    {
        loadWallpapersPage();

        if(gv.processPaused){
            actAsStart_=true;
            ui->startButton->setText(tr("&Start"));
            ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
            animateProgressbarOpacity(1);
            timerManager_->findSeconds(true);
            timerManager_->secondsRemaining_ += 1;
            globalParser_->resetSleepProtection(timerManager_->secondsRemaining_);
            updateSeconds();
            stopButtonsSetEnabled(true);
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
            timerManager_->findSeconds(true);
            startWasJustClicked_=true;
            startUpdateSeconds();
            stopButtonsSetEnabled(true);
            previousAndNextButtonsSetEnabled(true);
        }
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if(gv.liveEarthRunning)
    {
        ui->activate_livearth->setEnabled(false);
        ui->deactivate_livearth->setEnabled(true);
        startUpdateSeconds();
        page_button_clicked(1);
        animateProgressbarOpacity(1);
    }
    else if(gv.potdRunning)
    {
        ui->deactivate_potd->setEnabled(true);
        ui->activate_potd->setEnabled(false);
        page_button_clicked(2);
        startPotd(false);
    }
    else if(gv.liveWebsiteRunning)
    {
        ui->deactivate_website->setEnabled(true);
        ui->activate_website->setEnabled(false);
        startUpdateSeconds();
        page_button_clicked(4);
        animateProgressbarOpacity(1);
    }
    else
    {   //nothing's running, so just open the application..

        stopButtonsSetEnabled(false);
        previousAndNextButtonsSetEnabled(false);

        hideTimeForNext();
        page_button_clicked(settings->value("current_page", 0).toInt());

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
    wallpaperManager_->setBackground(image, true, gv.potdRunning||gv.liveEarthRunning, (gv.potdRunning ? 3 : 2));
    if(gv.setAverageColor){
        setButtonColor();
    }
    imageTransition(image);
    processRequestStop();
}

void MainWindow::escapePressed(){
    if(ui->stackedWidget->currentIndex() == 0 && ui->search_box->hasFocus() && searchIsOn_)
        on_search_close_clicked();
    else
        this->hide();
}

void MainWindow::startUpdateSeconds(){
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }

    updateSeconds();
    updateSecondsTimer_->start(1000);
    if(ui->timeForNext->isHidden()){
        ui->timeForNext->show();
    }
}

void MainWindow::updatePotdProgress(){
    int secsToPotd = globalParser_->getSecondsTillHour("00:00");

    if(secsToPotd<=3600)
        ui->timeForNext->setFormat(timerManager_->secondsToHms(secsToPotd));
    else
        ui->timeForNext->setFormat(timerManager_->secondsToHm(secsToPotd));

    setProgressbarsValue(short(((float) (secsToPotd/86400.0)) * 100));
}

void MainWindow::setProgressbarsValue(short value){
    ui->timeForNext->setValue(value);
}

void MainWindow::imageTransition(const QString &filename /*=QString()*/)
{
    if(!gv.previewImagesOnScreen || !gv.mainwindowLoaded){
        return;
    }

    if(scaleWatcher_->isRunning()){
        scaleWatcher_->cancel();
    }

    scaleWatcher_->setFuture(QtConcurrent::run(this, &MainWindow::scaleWallpapersPreview, filename));
}

QImage MainWindow::scaleWallpapersPreview(QString filename){
    QImageReader reader(filename);
    reader.setDecideFormatFromContent(true);
    reader.setScaledSize(QSize(reader.size().width()*imagePreviewResizeFactorX_, reader.size().height()*imagePreviewResizeFactorY_));
    return reader.read();
}

void MainWindow::animateScreenLabel(bool onlyHide){
    if(!gv.previewImagesOnScreen || !gv.mainwindowLoaded)
        return;

    if(ui->screen_label->pixmap(Qt::ReturnByValue).isNull())
        ui->screen_label_transition->clear();
    else{
        ui->screen_label_transition->setPixmap(ui->screen_label->pixmap(Qt::ReturnByValue));

        opacityEffect2_->setOpacity(true);
        ui->screen_label_transition->setGraphicsEffect(opacityEffect2_);
        decreaseOpacityAnimation->setStartValue(opacityEffect2_->opacity());
        decreaseOpacityAnimation->start();
    }

    if(onlyHide)
        ui->screen_label->clear();
    else{
        opacityEffect_->setOpacity(false);
        ui->screen_label->setGraphicsEffect(opacityEffect_);
        increaseOpacityAnimation->setStartValue(opacityEffect_->opacity());
        increaseOpacityAnimation->start();
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

void MainWindow::setPreviewImage(){
    if(!gv.previewImagesOnScreen || scaleWatcher_->isCanceled())
        return;

    animateScreenLabel(false);

    QImage image = scaleWatcher_->result();

    if(!image.isNull()){
#ifdef Q_OS_LINUX
        DesktopStyle desktopStyle = qvariant_cast<DesktopStyle>(ui->image_style_combo->currentData());

        if(image.format() == QImage::Format_Indexed8){
            //painting on QImage::Format_Indexed8 is not supported
            image = WallpaperManager::indexed8ToARGB32(image);
        }
        QImage colorsGradientsImage(75, 75, QImage::Format_RGB32);

        //bring me some food right meow!
        bool previewColorsAndGradientsMeow=true;

        if(desktopStyle == Tile || desktopStyle == Stretch || desktopStyle == Span){
            //the preview of the background color(s) is meaningless for the current styling
            previewColorsAndGradientsMeow = false;
        }
        if(previewColorsAndGradientsMeow || desktopStyle == NoneStyle){
            if(desktopStyle == NoneStyle)
                image = QImage();

            QString primaryColor;
            if(gv.setAverageColor) //TODO: Shouldn't primary color been already be the average color?
                primaryColor = WallpaperManager::getAverageColorOf(image).name();
            else
                primaryColor = ColorManager::getPrimaryColor();

            colorsGradientsImage.fill(primaryColor);

            if(currentShading != ColoringType::Solid)
                colorsGradientsImage = colorManager_->createVerticalHorizontalImage(75, 75);
        }
        QImage originalImage = image;
        int vScreenWidth= int((float)imagePreviewResizeFactorX_*gv.screenAvailableWidth);
        int vScreenHeight=int((float)imagePreviewResizeFactorY_*gv.screenAvailableHeight);

        //making the preview exactly like it will be shown at the desktop
        switch(desktopStyle){
        //Many thanks to http://askubuntu.com/questions/226816/background-setting-cropping-options
        case Tile: //TODO: Wrong preview on small pictures. The image starts from the center of the screen.
        {
            //Tile (or "wallpaper" picture-option)
            /*
             *  How it works (in Gnome):
             *  We have to repeat the image in case its width or height is less than the screen's
             *  If only one of the width or height are less then the screen's, then the one that
             *  is greater or equal is centered
             */
            if(image.width() < vScreenWidth || image.height() < vScreenHeight){
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
                    //timesY equals 1, this means the image's height is greater or equal to gv.screenAvailableHeight. So, the image must be centered on the height.
                    short yPos = 0-(originalImage.height()-vScreenHeight)/2.0;
                    for(short i=0; i<timesX; i++){
                        painter.drawPixmap(originalImage.width()*i, yPos, QPixmap::fromImage(originalImage));
                    }
                }
                else
                {
                    //timesX equals 1, this means the image's width is greater or equal to gv.screenAvailableWidth. So, the image must be centered on the width.
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
                if(currentDE == DE::Gnome){
                    //both dimensions are bigger than the screen's. Center both dimensions.

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
            image = QImage(vScreenWidth, vScreenHeight, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);

            painter.drawPixmap((vScreenWidth-originalImage.width())/2, (vScreenHeight-originalImage.height())/2, QPixmap::fromImage(originalImage));
            painter.end();
            break;
        }
        case Scale:
        {
            //Scale.
            /*
             *  How it works:
             *  It calculates the min of vScreenWidth/imageWidth and vScreenHeight/imageHeight.
             *  It stretches the corresponding dimension to fit the screen.
             *  The other dimension is centered.
             */
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
        case Span:
            //TODO: Stretch and Span seem to be the same. Confirm this
        default:
            break;
        }
        if(desktopStyle!=NoneStyle){
            if(image.width() > 2*SCREEN_LABEL_SIZE_X || image.height() > 2*SCREEN_LABEL_SIZE_Y)
                image = QImage(image.scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation)
                               .scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            else
                image = QImage(image.scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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
        image = QImage(image.scaled(2*SCREEN_LABEL_SIZE_X, 2*SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::FastTransformation)
                       .scaled(SCREEN_LABEL_SIZE_X, SCREEN_LABEL_SIZE_Y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
#endif
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
            imageTransition(filename);
        }
    }
}

void MainWindow::deChanged(){
    addingImageStylesNow = true;
    ui->image_style_combo->clear();
    findAvailableWallpaperStyles();
    addingImageStylesNow = false;
}

void MainWindow::findAvailableWallpaperStyles(){
#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        ui->image_style_combo->addItem(tr("Color"), NoneStyle);
        ui->image_style_combo->addItem(tr("Tile"), Tile);
        ui->image_style_combo->addItem(tr("Zoom"), Zoom);
        ui->image_style_combo->addItem(tr("Center"), Center);
        ui->image_style_combo->addItem(tr("Scale"), Scale);
        ui->image_style_combo->addItem(tr("Fill"), Stretch);
        ui->image_style_combo->addItem(tr("Span"), Span);
    }
    else if(currentDE == DE::XFCE){
        ui->image_style_combo->addItem(tr("None"), NoneStyle);
        ui->image_style_combo->addItem(tr("Centered"), Center);
        ui->image_style_combo->addItem(tr("Tiled"), Tile);
        ui->image_style_combo->addItem(tr("Stretched"), Stretch);
        ui->image_style_combo->addItem(tr("Scaled"), Scale);
        ui->image_style_combo->addItem(tr("Zoomed"), Zoom);
    }
    else if(currentDE == DE::LXDE){
        ui->image_style_combo->addItem(tr("Empty"), NoneStyle);
        ui->image_style_combo->addItem(tr("Fill"), Stretch);
        ui->image_style_combo->addItem(tr("Fit"), Scale);
        ui->image_style_combo->addItem(tr("Center"), Center);
        ui->image_style_combo->addItem(tr("Tile"), Tile);
    }
#else
# ifdef Q_OS_WIN
    ui->image_style_combo->addItem(tr("Color"), NoneStyle);
    ui->image_style_combo->addItem(tr("Tile"), Tile);
    ui->image_style_combo->addItem(tr("Center"), Center);
    ui->image_style_combo->addItem(tr("Stretch"), Stretch);

    // Windows 7 and above
    if(DesktopEnvironment::getOSproductVersion()>=6.1)
    {
        //Scale,Zoom
        ui->image_style_combo->addItem(tr("Fit"), Scale);
        ui->image_style_combo->addItem(tr("Fill"), Zoom);
        // Windows 8 and above
        if(DesktopEnvironment::getOSproductVersion()>=6.2)
            ui->image_style_combo->addItem(tr("Span"), Span);
    }
# else
#  ifdef Q_OS_MAC
    ui->image_style_combo->addItem(tr("Color"), NoneStyle);
    ui->image_style_combo->addItem(tr("Fill Screen"), Zoom);
    ui->image_style_combo->addItem(tr("Fit to Screen"), Scale);
    ui->image_style_combo->addItem(tr("Stretch to Fill Screendfdfgdfg"), Stretch);
    ui->image_style_combo->addItem(tr("Centre"), Center);
    ui->image_style_combo->addItem(tr("Tile"), Tile);
#  endif
# endif
#endif

    updateImageStyleCombo();
    currentBgMenu_->setEnabled(ui->image_style_combo->currentIndex()!=0);

    updateScreenLabel();

    gv.mainwindowLoaded=true;
}

void MainWindow::updateImageStyleCombo(){
    short index = wallpaperManager_->getCurrentFit();

    if(index<ui->image_style_combo->count() && index>=0 && index!=ui->image_style_combo->currentIndex())
        ui->image_style_combo->setCurrentIndex(index);
}

void MainWindow::setButtonColor(){
    QImage image(40, 19, QImage::Format_RGB32);

    if(currentShading == ColoringType::Solid)
        image.fill(ColorManager::getPrimaryColor());
    else
        image = colorManager_->createVerticalHorizontalImage(40, 19);

    ui->set_desktop_color->setIcon(QIcon(QPixmap::fromImage(image)));
}

void MainWindow::on_image_style_combo_currentIndexChanged(int index)
{
    if(!gv.mainwindowLoaded || addingImageStylesNow)
        return;

    wallpaperManager_->setCurrentFit(index);
    currentBgMenu_->setEnabled(index!=0);

    if(ui->screen_label_text->text().isEmpty())
        updateScreenLabel();
}

void MainWindow::actionsOnWallpaperChange(){
    if(gv.wallpapersRunning)
    {
        if(!fileManager_->currentFolderExists())
            return;

        timerManager_->findSeconds(true);

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
        globalParser_->saveSecondsLeftNow(timerManager_->secondsRemaining_, 0);
    }
    else if(gv.liveWebsiteRunning){
        ui->timeout_text_label->show();
        ui->website_timeout_label->show();
        processRequestStart();
        //websiteSnapshot_->start();
        timerManager_->secondsRemaining_=globalParser_->websiteSliderValueToSeconds(ui->website_slider->value());
        globalParser_->saveSecondsLeftNow(timerManager_->secondsRemaining_, 2);
    }
    else if(gv.liveEarthRunning){
        processRequestStart();
        imageFetcher_->setFetchType(FetchType::LE);
        imageFetcher_->fetch();
        timerManager_->secondsRemaining_=1800;
        globalParser_->saveSecondsLeftNow(timerManager_->secondsRemaining_, 1);
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
            //The previous time was today, so the picture must be still there,
            //so re-set it as background, no need to download anything!
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

#ifdef Q_OS_LINUX
    if(gv.pauseOnBattery){
        if(globalParser_->runsOnBattery()){
            if(!manuallyStartedOnBattery_){
                bool array[4] = {gv.wallpapersRunning, gv.liveEarthRunning, gv.potdRunning, gv.liveWebsiteRunning};

                for(int i=0;i<4;i++){
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
        if(timerManager_->secondsRemaining_<=0){
            actionsOnWallpaperChange();
            gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(timerManager_->secondsRemaining_);
        }

        //in case the computer went to hibernate/suspend take actions
        int secsToFinishInterval=gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
        if(timerManager_->secondsRemaining_<(secsToFinishInterval-1) || timerManager_->secondsRemaining_>(secsToFinishInterval+1))
        {
            int secondsToChange = gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
            if(secondsToChange<=0)
            {
                //the wallpaper should've already changed!
                actionsOnWallpaperChange();
                gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(timerManager_->secondsRemaining_);
            }
            else if (!(timerManager_->secondsRemaining_-secondsToChange<-1 || timerManager_->secondsRemaining_-secondsToChange>1)){
                //the time has yet to come, just update
                if(abs(timerManager_->secondsRemaining_-secondsToChange)>1){
                    timerManager_->secondsRemaining_=secondsToChange;
                }
            }
        }

        ui->timeForNext->setFormat(timerManager_->secondsToHms(timerManager_->secondsRemaining_));

        if(gv.wallpapersRunning)
        {
            if(timerManager_->totalSeconds_==0)
                setProgressbarsValue(100);
            else
                setProgressbarsValue((timerManager_->secondsRemaining_*100)/(timerManager_->totalSeconds_));
        }
        else if(gv.liveWebsiteRunning){
            int totalSecs=globalParser_->websiteSliderValueToSeconds(ui->website_slider->value());
            if(totalSecs==0){
                setProgressbarsValue(100);
            }
            else
            {
                setProgressbarsValue((timerManager_->secondsRemaining_*100)/totalSecs);
            }
            if(processingOnlineRequest_ && timePassedForLiveWebsiteRequest_<=WEBSITE_TIMEOUT){
                ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT+1-(timePassedForLiveWebsiteRequest_++))+" "+tr("seconds")+"...");
            }
        }
        else if(gv.liveEarthRunning){
            setProgressbarsValue(timerManager_->secondsRemaining_*100/1800);
        }

        timerManager_->secondsRemaining_--;
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
    ui->website_interval_slider_label->setText(timerManager_->secondsToMh(globalParser_->websiteSliderValueToSeconds(value)));
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
    else if(gv.liveWebsiteRunning){
        on_deactivate_website_clicked();
    }
}

void MainWindow::setAverageColor(const QString &image){
    //sets the desktop background color, updates mainwindow's primary color box
    globalParser_->setAverageColor(image);
    setButtonColor();
}

QString MainWindow::fixBasenameSize(const QString &basename){
    if(basename.length()>35){
        return basename.left(32)+"...";
    }
    return basename;
}

void MainWindow::updateScreenLabel()
{
    if(!gv.previewImagesOnScreen || !gv.mainwindowLoaded)
        return;

    ui->screen_label_info->clear();
    ui->screen_label_text->clear();

    switch(ui->stackedWidget->currentIndex()){
    case 0:
    {
        if(wallpaperManager_->wallpapersCount()==0 || ui->wallpapersList->selectedItems().count()==0)
            changeTextOfScreenLabelTo(tr("Select an image to preview")); //TODO: Stays on top sometimes
        else
            on_wallpapersList_itemSelectionChanged();
        break;
    }
    case 1:
    {
        QString filename=globalParser_->getFilename(gv.wallchHomePath+"liveEarth*");
        if(!filename.isEmpty())
            imageTransition(filename);
        else
            changeTextOfScreenLabelTo(tr("Preview not available"));
        break;
    }
    case 2:
    {
        QString filename=globalParser_->getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
        if(!filename.isEmpty())
            imageTransition(filename);
        else
            changeTextOfScreenLabelTo(tr("Preview not available"));
        break;
    }
    case 4:
    {
        if(QFile::exists(gv.wallchHomePath+LW_PREVIEW_IMAGE))
            imageTransition(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        else
            changeTextOfScreenLabelTo(tr("Preview not available"));
        break;
    }
    }
}

void MainWindow::changeTextOfScreenLabelTo(const QString &text)
{
    if(!gv.previewImagesOnScreen)
        return;

    animateScreenLabel(true);

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

//Wallpapers code

void MainWindow::justChangeWallpaper(){
    loadWallpapersPage();

    wallpaperManager_->setRandomWallpaperAsBackground();
}

void MainWindow::on_startButton_clicked(){
    loadWallpapersPage();

    if (!ui->startButton->isEnabled()){
        globalParser_->desktopNotify(tr("Not enough pictures to start chaning wallpapers."), false, "info");
        globalParser_->error("Too few images for wallpaper feature to start. Make sure there are at least 2 pictures at the selected folder.");
        return;
    }

    stopEverythingThatsRunning(1);
    startPauseWallpaperChangingProcess();
}

void MainWindow::startPauseWallpaperChangingProcess(){
    if(!fileManager_->currentFolderExists()){
        return;
    }
    if (actAsStart_){

        actAsStart_=false; //the next time act like pause is pressed
        gv.wallpapersRunning=true;
        SettingsManager::updateStartup();
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
                    if(ui->wallpapersList->currentItem()->isSelected())
                        wallpaperManager_->setRandomMode(true, -1, ui->wallpapersList->currentRow());
                }
                else
                    wallpaperManager_->setRandomMode(true);
            }
            else
                wallpaperManager_->setRandomMode(false);
        }

        if(gv.processPaused){
            //If the process was paused, then we need to continue from where it is left, not from the next second
            timerManager_->secondsRemaining_+=1;
        }

        globalParser_->resetSleepProtection(timerManager_->secondsRemaining_);

        if(!gv.processPaused)
            //if the process wasn't paused, then the progressbar is hidden. Add an animation so as to show it
            animateProgressbarOpacity(1);

        gv.processPaused=false;

        Q_EMIT signalRecreateTray();

        if(gv.pauseOnBattery){
            if(globalParser_->runsOnBattery()){
                //manually started
#ifdef Q_OS_LINUX
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
        page_button_clicked(0);
    }
    else
    {
        actAsStart_=true; //<- the next time act like start is pressed
        gv.processPaused=true;
        stoppedBecauseOnBattery_=false;
        ui->startButton->setText(tr("&Start"));
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
        Q_EMIT signalRecreateTray();
        updateSecondsTimer_->stop();
        ui->timeForNext->setFormat(ui->timeForNext->format()+" - Paused.");
        previousAndNextButtonsSetEnabled(false);
        if(wallpaperManager_->wallpapersCount() != 0){
            ui->shuffle_images_checkbox->setEnabled(true);
        }
        if(gv.independentIntervalEnabled){
            globalParser_->saveSecondsLeftNow(timerManager_->secondsRemaining_, 0);
        }
    }
}

void MainWindow::on_stopButton_clicked(){
    if (!ui->stopButton->isEnabled())
        return;

    if(wallpaperManager_->wallpapersCount()!=0)
        ui->shuffle_images_checkbox->setEnabled(true);

    stoppedBecauseOnBattery_=false;
    wallpaperManager_->startOver();
    actAsStart_=true;
    ui->startButton->setText(tr("&Start"));
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/media-playback-start.png")));
    animateProgressbarOpacity(0);
    gv.processPaused=false;
    firstRandomImageIsntRandom_=false;
    timerManager_->secondsRemaining_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    previousAndNextButtonsSetEnabled(false);

    startButtonsSetEnabled(wallpaperManager_->wallpapersCount() >= LEAST_WALLPAPERS_FOR_START);

    stopButtonsSetEnabled(false);
    gv.wallpapersRunning=false;
    SettingsManager::updateStartup();
#ifdef Q_OS_LINUX
    if(gv.pauseOnBattery){
        if(batteryStatusChecker_->isActive()){
            batteryStatusChecker_->stop();
        }
    }
#endif
    Q_EMIT signalRecreateTray();
    if(gv.independentIntervalEnabled){
        globalParser_->saveSecondsLeftNow(-1, 0);
    }
}

void MainWindow::on_next_Button_clicked()
{
    if (!ui->next_Button->isEnabled() || !fileManager_->currentFolderExists())
        return;

    updateSecondsTimer_->stop();
    timerManager_->secondsRemaining_=0;
    startUpdateSeconds();
}

void MainWindow::on_previous_Button_clicked()
{
    if(!ui->previous_Button->isEnabled() || !fileManager_->currentFolderExists())
        return;

    updateSecondsTimer_->stop();

    wallpaperManager_->setBackground(wallpaperManager_->getPreviousWallpaper(), true, true, 1);
    if(gv.setAverageColor)
        setButtonColor();

    if(gv.rotateImages && gv.iconMode)
        forceUpdateIconOf(wallpaperManager_->currentWallpaperIndex()-2);

    timerManager_->findSeconds(true);

    globalParser_->resetSleepProtection(timerManager_->secondsRemaining_);
    startUpdateSeconds();
}

void MainWindow::beginFixCacheForFolders(){
    QtConcurrent::run(cacheManager_, &CacheManager::fixCacheSizeWithCurrentFoldersBeing, fileManager_->getCurrentWallpaperFolders());
}

void MainWindow::clearWallpapersList(){
    ui->wallpapersList->clear();
    wallpaperManager_->clearWallpapers();
}

void MainWindow::currentFolderDoesNotExist()
{
    int index=ui->pictures_location_comboBox->currentIndex();
    ui->pictures_location_comboBox->setItemText(index, fileManager_->currentSelectionIsASet() ? ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()+" (0)":globalParser_->basenameOf(ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()+" (0)" ));

    if(gv.wallpapersRunning)
        on_stopButton_clicked();

    if(fileManager_->currentSelectionIsASet())
    {
        QString listOfFolders;
        short foldersListCount = fileManager_->currentFolderList_.count();
        for(short i = 0;i < foldersListCount; i++)
            listOfFolders.append(fileManager_->currentFolderList_.at(i) + "\n");
        if(QMessageBox::question(this, tr("None of the below folders exist\n"), listOfFolders+
                                                                                     tr("They could be on a hard drive on this computer so check if that disk is properly inserted, or it could be on a network so check if you are connected to the Internet or your network, and then try again. Delete this set of folders from the list?"))==QMessageBox::Yes)
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
            ui->pictures_location_comboBox->removeItem(index);
            savePicturesLocations();
        }
        else
            ui->pictures_location_comboBox->setCurrentIndex(0);
    }
    else
    {
        if(QMessageBox::question(this, tr("Location is not available"), fileManager_->currentFolder_+" "+tr("referes to a location that is unavailable.")+" "+
                                                                             tr("It could be on a hard drive on this computer so check if that disk is properly inserted, or it could be on a network so check if you are connected to the Internet or your network, and then try again. Delete this folder from the list?"))==QMessageBox::Yes)
        {
            ui->pictures_location_comboBox->setCurrentIndex(0);
            ui->pictures_location_comboBox->removeItem(index);
            savePicturesLocations();
        }
        else
            ui->pictures_location_comboBox->setCurrentIndex(0);
    }
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

void MainWindow::addFolderForMonitor(const QString &folder){
    if(QDir(folder).exists()){
        short count = ui->pictures_location_comboBox->count();
        for(short i=0; i<count; i++)
        {
            if(fileManager_->foldersAreSame(folder, ui->pictures_location_comboBox->itemData(i, Qt::UserRole).toString()))
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
        if(ui->shuffle_images_checkbox->isChecked()) //from normal to random. The first random image must not be the current image
            wallpaperManager_->setRandomMode(true, wallpaperManager_->currentWallpaperIndex());
        else //from random to normal
            wallpaperManager_->convertRandomToNormal();
    }

    QString image=wallpaperManager_->getNextWallpaper();

    wallpaperManager_->addToPreviousWallpapers(image);

    wallpaperManager_->setBackground(image, true, true, 1);
    if(gv.setAverageColor)
        setButtonColor();

    if(gv.rotateImages && gv.iconMode)
        forceUpdateIconOf(wallpaperManager_->currentWallpaperIndex());
}

void MainWindow::searchFor(const QString &term){
    if(term.isEmpty())
        return;

    ui->wallpapersList->clearSelection();
    searchList_.clear();
    int listCount=wallpaperManager_->wallpapersCount();
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
    match_->setPattern(QRegularExpression::wildcardToRegularExpression("*"+term+"*"));
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
    if(currentSearchItemIndex >= wallpaperManager_->wallpapersCount()+1){
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

void MainWindow::on_timerSlider_valueChanged(int value)
{
    ui->wallpapers_slider_time->setText(timerManager_->secondsToMinutesHoursDays(timerManager_->defaultIntervals.at(value-1)));

    if(!gv.mainwindowLoaded)
        return;

    timerManager_->findSeconds(false);
    updateCheckTime_->start(3000);
    settings->setValue("delay", timerManager_->secondsInWallpapersSlider_);
    settings->setValue("timeSlider", ui->timerSlider->value());
    settings->sync();
}

void MainWindow::updateTiming(){
    if(updateCheckTime_->isActive())
        updateCheckTime_->stop();

    if(!loadedPages_[0])
        return;

    settings->setValue( "timeSlider", ui->timerSlider->value());
    settings->setValue( "delay", timerManager_->secondsInWallpapersSlider_ );
    settings->sync();
}

void MainWindow::checkBatteryStatus(){
    if(!globalParser_->runsOnBattery()){
#ifdef Q_OS_LINUX
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
}

void MainWindow::stopButtonsSetEnabled(bool enabled)
{
    ui->stopButton->setEnabled(enabled);
}

void MainWindow::previousAndNextButtonsSetEnabled(bool enabled){
    ui->next_Button->setEnabled(enabled); ui->previous_Button->setEnabled(enabled);
}

void MainWindow::addFilesToWallpapers(const QString path){
    QString cleanPath = path.endsWith('/') ? path.left(path.count()-1) : path;
    QStringList currrentDirectory = QDir (cleanPath, QString(""), QDir::Name, QDir::Files).entryList(IMAGE_FILTERS);

    Q_FOREACH(QString file, currrentDirectory){
        QString fullPath= cleanPath + '/' + file;
        QListWidgetItem *item = new QListWidgetItem;
        if(gv.iconMode){
            item->setIcon(imageLoading_);
            item->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
            item->setToolTip(fullPath);
        }
        else{
            item->setText(fullPath);
            item->setToolTip("");
            item->setStatusTip(fullPath);
        }
        ui->wallpapersList->addItem(item);

        wallpaperManager_->addWallpaper(fullPath);
    }
}

void MainWindow::changePathsToIcons()
{
    ui->wallpapersList->setIconSize(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
    ui->wallpapersList->setViewMode(QListView::IconMode);
    ui->wallpapersList->setUniformItemSizes(true);
    ui->wallpapersList->setDragEnabled(false);
    gv.iconMode=true;
    int listCount=wallpaperManager_->wallpapersCount();

    for(int i=0;i<listCount;i++){
        QString status=ui->wallpapersList->item(i)->statusTip();
        ui->wallpapersList->item(i)->setToolTip(status);
        ui->wallpapersList->item(i)->setStatusTip("");
        ui->wallpapersList->item(i)->setText("");
        ui->wallpapersList->item(i)->setSizeHint(WALLPAPERS_LIST_ICON_SIZE+WALLPAPERS_LIST_ITEMS_OFFSET);
        ui->wallpapersList->item(i)->setIcon(imageLoading_);
    }
    launchTimerToUpdateIcons();
    iconsPathsChanged();
}

void MainWindow::changeIconsToPaths(){
    ui->wallpapersList->setViewMode(QListView::ListMode);
    ui->wallpapersList->setUniformItemSizes(false);
    ui->wallpapersList->setIconSize(QSize(-1, -1));
    gv.iconMode=false;
    int file_count=wallpaperManager_->wallpapersCount();

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
    iconsPathsChanged();
}

void MainWindow::iconsPathsChanged(){
    if(ui->stackedWidget->currentIndex()==0){
        animateScreenLabel(true);
        updateScreenLabel();
    }
    clearSearchBox();
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
    if(!wallpaperManager_->wallpapersCount())
        return;

    currentlyUpdatingIcons_=true;
    QListWidgetItem* minItem=ui->wallpapersList->itemAt(0, 0);
    QListWidgetItem* maxItem=ui->wallpapersList->itemAt(ui->wallpapersList->width()-30, ui->wallpapersList->height()-5);

    int diff=30;
    while(!maxItem){
        diff+=5;
        maxItem=ui->wallpapersList->itemAt(ui->wallpapersList->width()-diff, ui->wallpapersList->height()-5);
        if(ui->wallpapersList->width()-diff<0){
            maxItem=ui->wallpapersList->item(wallpaperManager_->wallpapersCount()-1);
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
        if(wallpaperManager_->wallpapersCount() > 1){
            delete ui->wallpapersList->item(index);
            wallpaperManager_->getCurrentWallpapers().removeAt(index);
        }
        else
            clearWallpapersList();
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

    if(ui->wallpapersList->currentRow() == index && ui->stackedWidget->currentIndex() == 0)
        updateScreenLabel();
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

void MainWindow::clearSearchBox(){
    if(searchIsOn_){
        on_search_close_clicked();
    }
    ui->search_box->clear();
}

void MainWindow::on_pictures_location_comboBox_currentIndexChanged(int index)
{
    if(changingPicturesLocations_)
        return;

    clearSearchBox();

    fileManager_->resetWatchFolders();

    clearWallpapersList();

    settings->setValue("currentFolder_index", ui->pictures_location_comboBox->currentIndex());
    settings->sync();

    bool selectionIsOk = fileManager_->picturesLocationChanged();

    if(selectionIsOk)
    {
        startButtonsSetEnabled(wallpaperManager_->wallpapersCount() >= LEAST_WALLPAPERS_FOR_START);

        if(wallpaperManager_->wallpapersCount())
            ui->wallpapersList->setCurrentRow(0);

        if(gv.wallpapersRunning)
            processRunningResetPictures();
    }
    else
    {
        startButtonsSetEnabled(false);
        if(gv.wallpapersRunning)
            on_stopButton_clicked();

        if(gv.mainwindowLoaded)
        {
            if(index==0)
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your System's backgrounds folder."));
            else if(index==1)
                QMessageBox::warning(this, tr("Load system's pictures folder"), tr("We couldn't find your Pictures folder."));
            else
                currentFolderDoesNotExist();
        }
    }
    monitoredFoldersUpdated();
}

void MainWindow::processRunningResetPictures(){
    /*
     * This function handles the case when the parent folder for the wallpapers is changed
     */

    //folder has changed/updated
    if(ui->shuffle_images_checkbox->isChecked()){
        if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START)
            on_stopButton_clicked();
        else
        {
            //generate new random images
            timerManager_->secondsRemaining_=0;
            wallpaperManager_->startOver();
        }
    }
    else if(wallpaperManager_->wallpapersCount() >= LEAST_WALLPAPERS_FOR_START)
    {
        wallpaperManager_->startOver(); //start from the beginning of the new folder
        timerManager_->secondsRemaining_=0;
    }
    else
        on_stopButton_clicked();
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

void MainWindow::showHideSearchBox(){
    if(!ui->stackedWidget->currentIndex()==0)
        return;

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
    page_button_clicked(1);
    ui->activate_livearth->setEnabled(false);
    ui->deactivate_livearth->setEnabled(true);
    gv.liveEarthRunning=true;
    SettingsManager::updateStartup();
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

    timerManager_->secondsRemaining_=0;
    imageFetcher_->abort();
    gv.liveEarthRunning=false;
    SettingsManager::updateStartup();
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
    page_button_clicked(2);
    startPotd(true);
}

void MainWindow::startPotd(bool launchNow){
    ui->deactivate_potd->setEnabled(true);
    ui->activate_potd->setEnabled(false);
    gv.potdRunning=true;
    SettingsManager::updateStartup();
    justUpdatedPotd_=false;
    if(launchNow){
        actionsOnWallpaperChange();
    }
    animateProgressbarOpacity(1);

    Q_EMIT signalRecreateTray();
    startUpdateSeconds();
    updatePotdProgress();
}

void MainWindow::on_deactivate_potd_clicked()
{
    if(!ui->deactivate_potd->isEnabled()){
        return;
    }

    timerManager_->secondsRemaining_=0;
    processRequestStop();
    imageFetcher_->abort();
    gv.potdRunning=false;
    SettingsManager::updateStartup();
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    animateProgressbarOpacity(0);
    ui->activate_potd->setEnabled(true);
    ui->deactivate_potd->setEnabled(false);
    stoppedBecauseOnBattery_=false;

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

//Live Website Code
void MainWindow::on_activate_website_clicked()
{   
    stopEverythingThatsRunning(5);
    loadLiveWebsitePage();

    if(websiteSnapshot_==NULL || !ui->activate_website->isEnabled() || !websiteConfiguredCorrectly()){
        return;
    }

    gv.websiteWebpageToLoad=gv.onlineLinkForHistory=ui->website->text();
    gv.websiteInterval=ui->website_slider->value();

    page_button_clicked(4);

    ui->deactivate_website->setEnabled(true);
    ui->activate_website->setEnabled(false);
    ui->website_timeout_label->setText(QString::number(WEBSITE_TIMEOUT)+" "+tr("seconds")+"...");
    ui->timeout_text_label->show();
    ui->website_timeout_label->show();
    timePassedForLiveWebsiteRequest_=1;
    ui->live_website_login_widget->setEnabled(false);

    QApplication::processEvents(QEventLoop::AllEvents);

    gv.liveWebsiteRunning=true;
    SettingsManager::updateStartup();

    Q_EMIT signalRecreateTray();
    setProgressbarsValue(100);
    animateProgressbarOpacity(1);

    /*prepareWebsiteSnapshot();
    websiteSnapshot_->setCrop(ui->website_crop_checkbox->isChecked(), gv.websiteCropArea);
    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    startUpdateSeconds();*/
}

void MainWindow::on_deactivate_website_clicked()
{
    if(!ui->deactivate_website->isEnabled()){
        return;
    }

    stoppedBecauseOnBattery_=false;
    gv.liveWebsiteRunning=false;
    SettingsManager::updateStartup();
    timerManager_->secondsRemaining_=0;
    if(updateSecondsTimer_->isActive()){
        updateSecondsTimer_->stop();
    }
    if(gv.independentIntervalEnabled){
        globalParser_->saveSecondsLeftNow(-1, 2);
    }
    ui->live_website_login_widget->setEnabled(true);
    animateProgressbarOpacity(0);

    Q_EMIT signalUncheckRunningFeatureOnTray();
    /*disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageCreated(QImage*,short)));
    ui->deactivate_website->setEnabled(false); ui->activate_website->setEnabled(true);
    ui->timeout_text_label->hide();
    ui->website_timeout_label->hide();
    processRequestStop();
    websiteSnapshot_->stop();*/
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

        wallpaperManager_->setBackground(filename, true, true, 5);
        if(gv.setAverageColor)
            setButtonColor();

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
    /*websiteSnapshot_->setParameters(QUrl(ui->website->text()), gv.screenAvailableWidth, gv.screenAvailableHeight);

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

    websiteSnapshot_->setTimeout(WEBSITE_TIMEOUT);*/
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
    if(loadedPages_[0])
        return;

    if(wallpaperManager_==NULL)
        wallpaperManager_=new WallpaperManager();

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

    ui->pictures_location_comboBox->setItemText(0, DesktopEnvironment::getOSprettyName() + " " + tr("Desktop Backgrounds"));
    ui->pictures_location_comboBox->setItemData(0, DesktopEnvironment::getOSWallpaperPath(), Qt::UserRole);
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
        if(fileManager_->atLeastOneFolderFromTheSetExists()){
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
        fileManager_->currentFolder_=ui->pictures_location_comboBox->itemData(currentFolder, Qt::UserRole).toString();
        if(!QDir(fileManager_->currentFolder_).exists())
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

    ui->stackedWidget_2->setCurrentIndex(gv.typeOfInterval);

    ui->days_spinBox->setValue(settings->value("days_box", 0).toInt());
    ui->hours_spinBox->setValue(settings->value("hours_box", 0).toInt());
    ui->minutes_spinBox->setValue(settings->value("minutes_box", 30).toInt());
    ui->seconds_spinBox->setValue(settings->value("seconds_box", 0).toInt());

    ui->timerSlider->setValue(settings->value("timeSlider", 7).toInt());
    if(ui->timerSlider->value()==7){
        on_timerSlider_valueChanged(7);
    }
    launchTimerToUpdateIcons();

    openCloseSearch_ = new QPropertyAnimation(ui->search_widget, "maximumHeight");
    connect(openCloseSearch_, SIGNAL(finished()), this, SLOT(openCloseSearchAnimationFinished()));
    openCloseSearch_->setDuration(GENERAL_ANIMATION_SPEED);

    startButtonsSetEnabled(wallpaperManager_->wallpapersCount() >= LEAST_WALLPAPERS_FOR_START);
}

void MainWindow::loadLePage(){
    if(loadedPages_[1])
        return;

    ui->le_tag_checkbox->setChecked(gv.leEnableTag);
    ui->le_tag_button->setEnabled(gv.leEnableTag);
}

void MainWindow::loadPotdPage(){
    if(loadedPages_[2])
        return;

    ui->label_5->setText("<span style=\"font-style:italic;\">"+tr("Style")+"</span><span style=\" font-weight:600; font-style:italic;\"> "+tr("Scale")+"</span><span style=\" font-style:italic;\"> "+tr("is highly recommended for this feature")+"</span>");
    ui->include_description_checkBox->setChecked(gv.potdIncludeDescription);
    ui->edit_potd->setEnabled(gv.potdIncludeDescription);
}

void MainWindow::loadWallpaperClocksPage()
{
    if(loadedPages_[3])
        return;
}

void MainWindow::loadLiveWebsitePage(){
    if(loadedPages_[4])
        return;

    //add login details information
    openCloseAddLogin_ = new QPropertyAnimation(ui->live_website_login_widget, "maximumHeight");
    connect(openCloseAddLogin_, SIGNAL(finished()), this, SLOT(openCloseAddLoginAnimationFinished()));
    openCloseAddLogin_->setDuration(GENERAL_ANIMATION_SPEED);
    ui->add_login_details->setChecked(gv.websiteLoginEnabled);
    if(gv.websiteLoginEnabled)
        on_add_login_details_clicked(true);
    ui->username->setText(gv.websiteLoginUsername);
    ui->password->setText(gv.websiteLoginPasswd);

    ui->website->setText(gv.websiteWebpageToLoad);
    short old_website_slider_value=ui->website_slider->value();
    ui->website_slider->setValue(gv.websiteInterval);
    if(gv.websiteInterval == old_website_slider_value)
        on_website_slider_valueChanged(gv.websiteInterval);
    ui->website_crop_checkbox->setChecked(gv.websiteCropEnabled);
    ui->edit_crop->setEnabled(ui->website_crop_checkbox->isEnabled() && gv.websiteCropEnabled);
    ui->redirect_checkBox->setChecked(gv.websiteRedirect);
    ui->final_webpage->setEnabled(gv.websiteRedirect);
    ui->final_webpage->setText(gv.websiteFinalPageToLoad);
    ui->timeout_text_label->hide();
    ui->website_timeout_label->hide();

    if(gv.previewImagesOnScreen){
        ui->verticalLayout_11->addWidget(ui->timeout_text_label);
        ui->verticalLayout_11->addWidget(ui->website_timeout_label);
    }
    else {
        ui->horizontalLayout_23->addWidget(ui->timeout_text_label);
        ui->horizontalLayout_23->addWidget(ui->website_timeout_label);
    }

    changedWebPage_= initialWebPage_=ui->website->text();
    if(websiteSnapshot_==NULL)
        websiteSnapshot_ = new WebsiteSnapshot();
}

void MainWindow::loadMelloriPage()
{
    if(loadedPages_[5])
        return;
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

void MainWindow::page_button_clicked(int btn){
    if(gv.mainwindowLoaded && ui->stackedWidget->currentIndex()==btn)
        return;

    switch(btn){
    case 0:
    {
        loadWallpapersPage();
        launchTimerToUpdateIcons();
        on_timerSlider_valueChanged(ui->timerSlider->value());

        ui->sep1->raise();
        break;
    }
    case 1:
    {
        loadLePage();
        ui->sep1->raise();
        ui->sep2->raise();
        break;
    }
    case 2:
    {
        loadPotdPage();
        ui->sep2->raise();
        ui->sep3->raise();
        break;
    }
    case 3:
    {
        loadWallpaperClocksPage();
        ui->sep3->raise();
        ui->sep4->raise();
        break;
    }
    case 4:
    {
        loadLiveWebsitePage();
        ui->sep4->raise();
        ui->sep5->raise();
        break;
    }
    case 5:
    {
        loadMelloriPage();
        ui->sep5->raise();
        ui->sep6->raise();
        break;
    }
    }

    btn_group->button(btn)->setChecked(true);
    btn_group->button(btn)->raise();

    ui->stackedWidget->setCurrentIndex(btn);
    loadedPages_[btn]=true;
    QTimer::singleShot(10, this, SLOT(updateScreenLabel()));
}

void MainWindow::previousPage(){
    if(ui->stackedWidget->currentIndex()==0)
        page_button_clicked(5);
    else
        page_button_clicked(ui->stackedWidget->currentIndex()-1);
}

void MainWindow::nextPage(){
    if(ui->stackedWidget->currentIndex()==5)
        page_button_clicked(0);
    else
        page_button_clicked(ui->stackedWidget->currentIndex()+1);
}

//Actions code

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

void MainWindow::getScreenResolution (QRect geometry){
    gv.screenWidth = geometry.width();
    gv.screenHeight = geometry.height();
}

void MainWindow::getScreenAvailableResolution (QRect geometry){
    availableGeometry_ = geometry;
    gv.screenAvailableWidth = geometry.width();
    gv.screenAvailableHeight = geometry.height();
}

//Dialogs Code

void MainWindow::on_actionHistory_triggered()
{
    if(historyShown_){
        return;
    }
    historyShown_=true;
    history_ = new History (wallpaperManager_, this);
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
    connect(preferences_, SIGNAL(researchFolders()), fileManager_, SLOT(researchFolders()));
    connect(preferences_, SIGNAL(previewChanged()), this, SLOT(wait_preview_changed()));
    connect(preferences_, SIGNAL(intervalTypeChanged()), this, SLOT(intervalTypeChanged()));
    connect(preferences_, SIGNAL(changeTheme()), this, SLOT(changeCurrentTheme()));
    connect(preferences_, SIGNAL(maxCacheChanged(qint64)), cacheManager_, SLOT(setMaxCache(qint64)));
    connect(preferences_, SIGNAL(deManuallyChanged()), this, SLOT(deChanged()));

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
    colorsGradients_ = new ColorsGradients(wallpaperManager_, this);

    colorsGradients_->setModal(true);
    colorsGradients_->setAttribute(Qt::WA_DeleteOnClose);
    colorsGradients_->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    connect(colorsGradients_, SIGNAL(destroyed()), this, SLOT(colorsGradientsDestroyed()));
    connect(colorsGradients_, SIGNAL(updateTv()), this, SLOT(updateScreenLabel()));
    connect(colorsGradients_, SIGNAL(updateDesktopColor()), this, SLOT(setButtonColor()));
    connect(colorsGradients_, SIGNAL(updateImageStyle()), this, SLOT(updateImageStyleCombo()));
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

// Settings
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
    else if(page==4)
        ui->help_label->setText("Screenshot of a website");
    else
        ui->help_label->setText("Empty Page");

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

// Animations in case user shows/hides the preview screen
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
           bool array[4] = {gv.wallpapersRunning, gv.liveEarthRunning, gv.potdRunning, gv.liveWebsiteRunning};

           for(int i=0;i<4;i++){
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
}


void MainWindow::timeSpinboxChanged()
{
    int delay=ui->days_spinBox->value()*86400+ui->hours_spinBox->value()*3600+ui->minutes_spinBox->value()*60+ui->seconds_spinBox->value();
    if(delay==0)
    {
        delay=1;
        ui->seconds_spinBox->setValue(1);
    }
    timerManager_->findSeconds(false);
    updateCheckTime_->start(3000);

    settings->setValue("delay", delay);
    settings->sync();
}

void MainWindow::on_days_spinBox_valueChanged(int arg1)
{
    settings->setValue("days_box", arg1);
}


void MainWindow::on_hours_spinBox_valueChanged(int arg1)
{
    settings->setValue("hours_box", arg1);
}


void MainWindow::on_minutes_spinBox_valueChanged(int arg1)
{
    settings->setValue("minutes_box", arg1);
}


void MainWindow::on_seconds_spinBox_valueChanged(int arg1)
{
    settings->setValue("seconds_box", arg1);
}

// 'Wallpapers' ListWidget functions

void MainWindow::on_wallpapersList_customContextMenuRequested()
{
    if(!fileManager_->currentFolderExists())
        return;

    int selectedCount = ui->wallpapersList->selectedItems().count();

    if(selectedCount==0)
        return;

    listwidgetMenu_ = new QMenu(this);

    if (selectedCount<2){

        QAction *enterAction = new QAction(tr("Set this item as Background"), listwidgetMenu_);
        enterAction->setShortcut(QKeySequence("Enter"));
        connect(enterAction, SIGNAL(triggered()), this, SLOT(on_wallpapersList_itemDoubleClicked()));
        listwidgetMenu_->addAction(enterAction);

        if(wallpaperManager_->wallpapersCount()>2)
           listwidgetMenu_->addAction(tr("Start from this image"), this, SLOT(startWithThisImage()));

        listwidgetMenu_->addAction(tr("Open Image"), this, SLOT(openImage()));
        listwidgetMenu_->addAction(tr("Open folder"), this, SLOT(openImageFolder()));
        listwidgetMenu_->addAction(tr("Rotate Right"), this, SLOT(rotateRight()));
        listwidgetMenu_->addAction(tr("Rotate Left"), this, SLOT(rotateLeft()));
        listwidgetMenu_->addAction(tr("Copy path to clipboard"), this, SLOT(copyImagePath()));
        listwidgetMenu_->addAction(tr("Copy image to clipboard"), this, SLOT(copyImage()));

        QAction *findAction = new QAction(tr("Find an image by name"), listwidgetMenu_);
        findAction->setShortcut(QKeySequence("Ctrl+F"));
        connect(findAction, SIGNAL(triggered()), this, SLOT(showHideSearchBoxMenu()));
        listwidgetMenu_->addAction(findAction);

        QAction *deleteAction = new QAction(tr("Delete image from disk"), listwidgetMenu_);
        deleteAction->setShortcut(QKeySequence::Delete);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(removeImageFromDisk()));
        listwidgetMenu_->addAction(deleteAction);

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

void MainWindow::on_wallpapersList_itemDoubleClicked()
{
    if(!wallpaperManager_->wallpapersCount())
        return;

    int curRow = ui->wallpapersList->currentRow();
    if(curRow < 0)
        return;

    QString picture = getPathOfListItem(curRow);

    if(WallpaperManager::imageIsNull(picture))
        return;

    wallpaperManager_->addToPreviousWallpapers(picture);
    wallpaperManager_->setBackground(picture, true, true, 1);

    if(gv.setAverageColor)
        setButtonColor();

    if(gv.rotateImages && gv.iconMode)
        forceUpdateIconOf(curRow);

#ifdef Q_OS_LINUX
    if(currentDE == DE::LXDE){
        DesktopStyle desktopStyle = qvariant_cast<DesktopStyle>(ui->image_style_combo->currentData());
        if(desktopStyle == NoneStyle)
           ui->image_style_combo->setCurrentIndex(2);
    }
#endif
}

void MainWindow::on_wallpapersList_itemSelectionChanged()
{
    if(!gv.mainwindowLoaded || !gv.previewImagesOnScreen)
        return;

    if(ui->wallpapersList->selectedItems().count()==1){

        QString filename = getPathOfListItem();

        if(!processingOnlineRequest_)
           ui->screen_label_info->setText(fixBasenameSize(globalParser_->basenameOf(filename)));

        imageTransition(filename);
    }
}

void MainWindow::deletePressed(){
    if(ui->stackedWidget->currentIndex() == 0 && ui->wallpapersList->selectedItems().count() > 0){
        if(ui->wallpapersList->selectedItems().count() > 1)
           removeImagesFromDisk();
        else
           removeImageFromDisk();
    }
}

// File System Watcher
void MainWindow::prepareToSearchFolders(){
    if(gv.previewImagesOnScreen){
        //keep the current selected file (it will be selected again after the folder researching has finished)

        if(ui->wallpapersList->selectedItems().count()){
           if(gv.iconMode){
               if(ui->wallpapersList->selectedItems().at(0)->statusTip().isEmpty())
                   nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->toolTip();
               else
                   nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->statusTip();
           }
           else
               nameOfSelectionPriorFolderChange_=ui->wallpapersList->selectedItems().at(0)->text();
        }
    }
    clearWallpapersList();
    clearSearchBox();
}

void MainWindow::monitoredFoldersUpdated(){
    //TODO: Fix bug where deleting the current picture freezes app
    int itemCount =ui->wallpapersList->count();
    startButtonsSetEnabled(wallpaperManager_->wallpapersCount() >= LEAST_WALLPAPERS_FOR_START);

    if(gv.iconMode)
        launchTimerToUpdateIcons();

    switch(ui->pictures_location_comboBox->currentIndex()){
    case 0:
        ui->pictures_location_comboBox->setItemText(0, DesktopEnvironment::getOSprettyName() + " "  + tr("Desktop Backgrounds")
                                                           + " (" + QString::number(itemCount) + ")");
        break;
    case 1:
        ui->pictures_location_comboBox->setItemText(1,tr("My Pictures") + " (" + QString::number(itemCount) + ")");
        break;
    default:
        short index=ui->pictures_location_comboBox->currentIndex();
        if(fileManager_->currentSelectionIsASet())
           ui->pictures_location_comboBox->setItemText(index, ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString()
                                                                  + " (" + QString::number(itemCount) + ")");
        else
           ui->pictures_location_comboBox->setItemText(index, globalParser_->basenameOf(ui->pictures_location_comboBox->itemData(index, Qt::UserRole).toString())
                                                                  + " (" + QString::number(itemCount) + ")");
        break;
    }

    if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START){
        if(gv.wallpapersRunning)
           on_stopButton_clicked();
    }
    else
        startButtonsSetEnabled(true);

    if(gv.previewImagesOnScreen && ui->stackedWidget->currentIndex()==0){
        searchFor(nameOfSelectionPriorFolderChange_);
        ui->screen_label_info->clear();
        updateScreenLabel();
    }
}


// 'Wallpapers' ListWidget right-click menu functions

void MainWindow::openImage()
{
    if(!ui->wallpapersList->currentItem()->isSelected())
        return;

    Global::openUrl("file:///" + getPathOfListItem());
}

void MainWindow::openImageFolder(){
    if(!ui->wallpapersList->currentItem()->isSelected())
        return;

    FileManager::openFolderOf(getPathOfListItem());
}

void MainWindow::openImageFolderMassive(){
    FileManager::openMultipleFolders(ui->wallpapersList->selectedItems());
}

void MainWindow::startWithThisImage(){
    stopEverythingThatsRunning(0);

    if(ui->shuffle_images_checkbox->isChecked())
        firstRandomImageIsntRandom_=true;

    on_startButton_clicked();
}

void MainWindow::removeImageFromDisk(){
    if (QMessageBox::question(this, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the selected image?")) != QMessageBox::Yes)
        return;

    QString imageFilename = getPathOfListItem(ui->wallpapersList->currentRow());

    if(!QFile::remove(imageFilename))
        QMessageBox::warning(this, tr("Error!"), tr("Image deletion failed possibly because you don't have the permissions to delete the image or the image doesn't exist"));
    else //remove the cache of the image, in case it exists
        cacheManager_->removeCacheOf(imageFilename);

    if(wallpaperManager_->wallpapersCount() <= LEAST_WALLPAPERS_FOR_START)
        startButtonsSetEnabled(false);
}

void MainWindow::removeImagesFromDisk(){
    int selectedCount = ui->wallpapersList->selectedItems().count();

    if(QMessageBox::question(this, tr("Warning!"), tr("Image deletion.")+"<br><br>"+tr("This action is going to permanently delete")+" "+QString::number(selectedCount)+" "+tr("images. Are you sure?"))==QMessageBox::Yes){
        QStringList imagesThatFailedToDelete;

        for(int i=0;i<selectedCount;i++){
           QString currentImage;

           if(gv.iconMode){
               if(ui->wallpapersList->selectedItems().at(i)->statusTip().isEmpty())
                   currentImage = ui->wallpapersList->selectedItems().at(i)->toolTip();
               else
                   currentImage = ui->wallpapersList->selectedItems().at(i)->statusTip();
           }
           else
               currentImage = ui->wallpapersList->selectedItems().at(i)->text();

           if(!QFile::remove(currentImage))
               imagesThatFailedToDelete << currentImage;
           else //remove the cache of the image, in case it exists
               cacheManager_->removeCacheOf(currentImage);
        }

        if(imagesThatFailedToDelete.count() > 0)
           QMessageBox::warning(this, tr("Error!"), tr("There was a problem with the deletion of the following files:")+"\n"+imagesThatFailedToDelete.join("\n"));
    }
}

void MainWindow::rotateRight(){
    if(!ui->wallpapersList->currentItem()->isSelected())
        return;

    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull())
        return;

    globalParser_->rotateImg(path, 6, false);

    rotationCompleted(path);
}

void MainWindow::rotateLeft(){
    if(!ui->wallpapersList->currentItem()->isSelected())
        return;

    QString path = getPathOfListItem();

    if(!QFile::exists(path) || QImage(path).isNull())
        return;

    globalParser_->rotateImg(path, 8, false);

    rotationCompleted(path);
}

void MainWindow::rotationCompleted(QString &imagePath){
    if(gv.iconMode)
        forceUpdateIconOf(ui->wallpapersList->currentRow());
    else
        updateScreenLabel();

    if(imagePath == wallpaperManager_->currentBackgroundWallpaper())
        wallpaperManager_->setBackground(imagePath, false, false, 1);
}

void MainWindow::copyImagePath(){
    globalParser_->copyTextToClipboard(getPathOfListItem());
}

void MainWindow::copyImage(){
    globalParser_->copyImageToClipboard(getPathOfListItem());
}

void MainWindow::showHideSearchBoxMenu(){
    if(searchIsOn_){
        ui->search_box->setFocus();
        return;
    }

    showHideSearchBox();
}

void MainWindow::showProperties()
{
    if(!ui->wallpapersList->currentItem()->isSelected() || ui->stackedWidget->currentIndex()!=0)
        return;

    dialogHelper_->showPropertiesDialog(ui->wallpapersList->currentRow(), wallpaperManager_);
}
