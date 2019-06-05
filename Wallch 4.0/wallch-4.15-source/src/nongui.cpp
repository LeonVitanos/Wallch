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

#include "nongui.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QDesktopWidget>

#include <iostream>
#include <getopt.h>

#ifdef ON_WIN32

#include <cmath>
#include <Windows.h>

#endif //#ifdef ON_WIN32

#define ARG_NOT_REQ 0
#define ARG_REQ 1
#define ARG_OPT 2

#define UPDATE_STATISTICS_SECONDS_INTERVAL 30000
#define CHECK_INTERNET_INTERVAL 10000

bool nonGUI::alreadyRuns(){
    alreadyRunsMem_ = new QSharedMemory("Wallch Memory", this);
    if(alreadyRunsMem_->attach(QSharedMemory::ReadOnly)){
        alreadyRunsMem_->detach();
        return true;
    }
    if(alreadyRunsMem_->create(1)){
        return false;
    }
    return false;
}

void nonGUI::getDelay(){
    totalSeconds_=settings->value("delay", DEFAULT_SLIDER_DELAY).toInt();
}

void nonGUI::dirChanged(){
    if(researchFoldersTimerMain_.isActive()){
        researchFoldersTimerMain_.stop();
    }
    researchFoldersTimerMain_.start(RESEARCH_FOLDERS_TIMEOUT);
}

void nonGUI::resetWatchFolders(bool justDelete){
    if(watchFoldersMain_){
        delete watchFoldersMain_;
    }
    if(!justDelete){
        watchFoldersMain_ = new QFileSystemWatcher(this);
        connect(watchFoldersMain_, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged()));
    }
}

void nonGUI::researchDirs(){
    if(researchFoldersTimerMain_.isActive()){
        researchFoldersTimerMain_.stop();
    }

    resetWatchFolders(false);

    if(!getPicturesLocation(false)){
        return;
    }

    if(gv.randomImagesEnabled){
        wallpaperManager_->setRandomMode(true);
    }
    else
    {
        wallpaperManager_->startOver();
    }
}

void nonGUI::getFilesFromFolder(const QString &path){
    //this function is for adding files of a monitored folder
    QDir directoryContainingPictures(path, QString(""), QDir::Name, QDir::Files);
    Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
        wallpaperManager_->addWallpaper(path+"/"+pic);
    }
}

void nonGUI::potdSetSameImage(){
    Global::debug("Picture Of The Day should already be in your hard drive.");
    QString filename=Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
    if(filename.isEmpty()){
        Global::error("Wallch will now attemp to download again the Picture Of The Day.");
        settings->setValue("last_day_potd_was_set", "");
        settings->setValue("previous_img_url", "retry");
        settings->sync();
        globalParser_->potd();
    }
    WallpaperManager::setBackground(filename, true, true, 3);
}

void nonGUI::readPictures(const QString &folder){
    /*
     * This function returns a QStringList with all the needed pictures
     * for the process from a folder(it reads it recursively)...
     * Used ONLY with --change
     */
    Global::debug("Wallch is reading pictures from your folder: '"+folder+"'");
    //getting all the subfolders of the parent 'file' folder
    QDir directoryContainingPictures(folder, QString(""), QDir::Name, QDir::Files);
    //looping between the folders and adding each of their images.
    Q_FOREACH(QString currentFolder, Global::listFolders(folder, true, true)){
        directoryContainingPictures.setPath(currentFolder);
        Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
            wallpaperManager_->addWallpaper(currentFolder+"/"+pic);
        }
    }

    if(wallpaperManager_->wallpapersCount() == 0){
        globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
        Global::error("Wallch has not enough pictures to continue.");
        doAction("--stop");
    }
}

bool nonGUI::getPicturesLocation(bool init){

    if(!startedWithJustChange_ && init){
        resetWatchFolders(false);
        connect(&researchFoldersTimerMain_, SIGNAL(timeout()), this, SLOT(researchDirs()));
    }

    QString parentFolder=getCurrentWallpapersFolder();
    if(!QDir(parentFolder).exists()){
        globalParser_->desktopNotify(tr("Folder doesn't exist for the process to continue."), false, "info");
        return false;
    }

    wallpaperManager_->clearWallpapers();

    Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
        if(!startedWithJustChange_){
            //no need to monitor anything if this instance is for changing the picture just once.
            if(!watchFoldersMain_){
                resetWatchFolders(false);
            }
            watchFoldersMain_->addPath(curFolder);
        }
        getFilesFromFolder(curFolder);
    }
    if(gv.randomImagesEnabled){
        wallpaperManager_->setRandomMode(true);
    }

    if(startedWithJustChange_){
        if(wallpaperManager_->wallpapersCount() == 0){
            globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
            Global::error("Wallch has not enough pictures to continue.");
            doAction("--stop");
            return false;
        }
    }
    else
    {
        if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START){
            globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
            Global::error("Wallch has not enough pictures to continue.");
            doAction("--stop");
            return false;
        }
    }
    return true;
}

void showUsage(short exitCode){
    Global::debug("Wallch " + QString::number(APP_VERSION, 'f', 2) + "\n"\
                  "Usage: wallch [OPTION]\n\n"\
                  "Wallch options:\n"\
                  "-h/--help                Show this help.\n"\
                  "--change                 Change desktop background once by picking randomly an image from the list\n"\
                  "--change[=DIRECTORY]     Change desktop background once to the wallpaper at [=DIRECTORY]\n"\
                  "--start                  Starts changing pictures from the last used list.\n"\
                  "--earth                  Starts live earth, updating every 30 minutes.\n"\
                  "--potd                   Starts picture of the day, updating once a day.\n"\
                  "--clock                  Starts wallpaper clocks.\n"\
                  "--website                Starts live website.\n"\
                  "--stop                   Stops the current process, if it is active.\n"\
                  "--next                   Proceeds to the next image, if available.\n"\
                  "--previous               Proceeds to the previous image, if available.\n"\
                  "--pause                  Pauses the current process, if the wallpaper changing process is active.\n"\
                  "--quit                   Quits any running instance of wallch.\n "\
                  "--version                Shows current version.");
    exit(exitCode);
}

#ifdef ON_LINUX
static void stop_fake_callback_main(DbusmenuMenuitem *, guint, gpointer){
    nongui.doAction("--stop");
}

static void pause_fake_callback_main(DbusmenuMenuitem *, guint, gpointer){
    nongui.doAction("--pause");
}

static void next_fake_callback_main(DbusmenuMenuitem *, guint, gpointer){
    nongui.doAction("--next");
}

static void previous_fake_callback_main(DbusmenuMenuitem *, guint, gpointer){
    nongui.doAction("--previous");
}

static void key_action_next(const char *, void *){
    nongui.doAction("--next");
}

void nonGUI::setupUnityShortcuts(){
    unityMenu_ = dbusmenu_menuitem_new();
    gv.unityStopAction = dbusmenu_menuitem_new();
    gv.unityNextAction = dbusmenu_menuitem_new();
    gv.unityPreviousAction = dbusmenu_menuitem_new();
    gv.unityPauseAction = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_LABEL, "Stop");
    dbusmenu_menuitem_property_set(gv.unityNextAction, DBUSMENU_MENUITEM_PROP_LABEL, "Next");
    dbusmenu_menuitem_property_set(gv.unityPreviousAction, DBUSMENU_MENUITEM_PROP_LABEL, "Previous");
    dbusmenu_menuitem_property_set(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_LABEL, "Pause");

    dbusmenu_menuitem_child_append (unityMenu_, gv.unityStopAction);
    dbusmenu_menuitem_child_append (unityMenu_, gv.unityNextAction);
    dbusmenu_menuitem_child_append (unityMenu_, gv.unityPreviousAction);
    dbusmenu_menuitem_child_append (unityMenu_, gv.unityPauseAction);


    g_signal_connect (gv.unityStopAction, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(&stop_fake_callback_main), (gpointer)this);
    g_signal_connect (gv.unityNextAction, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(&next_fake_callback_main), (gpointer)this);
    g_signal_connect (gv.unityPreviousAction, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(&previous_fake_callback_main), (gpointer)this);
    g_signal_connect (gv.unityPauseAction, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK(&pause_fake_callback_main), (gpointer)this);

    if(!gv.unityLauncherEntry){
        gv.unityLauncherEntry = unity_launcher_entry_get_for_desktop_id(APP_DESKTOP_NAME);
    }

    unity_launcher_entry_set_quicklist(gv.unityLauncherEntry, unityMenu_);

    dbusmenu_menuitem_property_set_bool(unityMenu_, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
}

void nonGUI::setUnityShortcutsState(bool stopState, bool pauseState, bool nextState, bool previousState){
    if(gv.unityStopAction){
        dbusmenu_menuitem_property_set_bool(gv.unityStopAction, DBUSMENU_MENUITEM_PROP_VISIBLE, stopState && !gv.wallpapersRunning);
        dbusmenu_menuitem_property_set_bool(gv.unityPauseAction, DBUSMENU_MENUITEM_PROP_VISIBLE, pauseState);
        dbusmenu_menuitem_property_set_bool(gv.unityNextAction, DBUSMENU_MENUITEM_PROP_VISIBLE, nextState);
        dbusmenu_menuitem_property_set_bool(gv.unityPreviousAction, DBUSMENU_MENUITEM_PROP_VISIBLE, previousState);
    }
}
#endif //#ifdef ON_LINUX

void nonGUI::waitForInternetConnection(){
    Global::debug("Checking for internet connection...");
    if (globalParser_->connectedToInternet()){
#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome){
            setupUnityShortcuts();
            setUnityShortcutsState(true, false, false, false);
        }
#endif
        if(gv.potdRunning){
            continueWithPotd(false);
        }
        else if(gv.liveEarthRunning){
            secondsLeft_=LIVEARTH_INTERVAL;
            continueWithLiveEarth();
        }
        else if(gv.liveWebsiteRunning){
            continueWithWebsite();
        }
    }
    else
    {
        Global::error("No internet connection, trying again in "+QString::number(CHECK_INTERNET_INTERVAL/1000)+" secs...");
    }
}

void nonGUI::actionsOnWallpaperChange(){
    if(gv.wallpapersRunning)
    {
        changeWallpaperNow();
        if(gv.randomTimeEnabled){
            srand(time(0));
            totalSeconds_=secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
        }
        else if(secondsLeft_<=0)
        {
            secondsLeft_=totalSeconds_;
        }
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(secondsLeft_, 0);
        }
    }
    else if(gv.liveEarthRunning)
    {
        globalParser_->livearth();
        secondsLeft_=totalSeconds_;
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(secondsLeft_, 1);
        }
    }
    else if(gv.liveWebsiteRunning)
    {
        websiteSnapshot_->start();
        secondsLeft_=totalSeconds_;
        if(gv.independentIntervalEnabled){
            Global::saveSecondsLeftNow(secondsLeft_, 2);
        }
    }
    else if(gv.wallpaperClocksRunning)
    {
        QString currentWallpaperClock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, wallpaperClocksMinutesChecked_, wallpaperClocksHourChecked_, wallpaperClocksAmPmChecked_, wallpaperClocksDayOfWeekChecked_, wallpaperClocksDayOfMonthChecked_ , wallpaperClocksMonthChecked_ );
        if(gv.setAverageColor){
            Global::setAverageColor(currentWallpaperClock);
        }
        secondsLeft_=totalSeconds_=wallpaperClocksTotalSeconds_;
    }
}

void nonGUI::updateSeconds(){
    /*
     * This function runs once a second and reduces
     * the timeout_count, which, once 0, will update
     * the desktop background...
     */
    gv.runningTimeOfProcess = QDateTime::currentDateTime();
    if(secondsLeft_<=0){
        actionsOnWallpaperChange();

        gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsLeft_);
    }
    else
    {
        if(secondsLeft_!=gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval))
        {
            int secs_to_changing_time = gv.runningTimeOfProcess.secsTo(gv.timeToFinishProcessInterval);
            if(secs_to_changing_time<0)
            {
                actionsOnWallpaperChange();

                gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsLeft_);
            }
            else if (!(gv.wallpaperClocksRunning&& (secondsLeft_-secs_to_changing_time<-1 || secondsLeft_-secs_to_changing_time>1))){
                secondsLeft_=secs_to_changing_time;
            }
        }
    }
#ifdef ON_LINUX
    if(gv.unityProgressbarEnabled && gv.currentDE==UnityGnome){
        Global::setUnityProgressbarValue((float) secondsLeft_/totalSeconds_);
    }
#endif
    secondsLeft_--;
}

void nonGUI::checkPicOfDay(){
    if(Global::timeNowToString()=="00:00"){
        if(justUpdatedPotd_)
        {
            return;
        }
        QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
        QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
            justUpdatedPotd_=true;
            globalParser_->potd();
        }
        else
        {
            //the day is the same, setting the same image.
            potdSetSameImage();
        }
    }
    else
    {
        justUpdatedPotd_=false;
        //time has yet to be reached!
        if(this->isSingleShot()){
            if(this->isActive()){
                this->stop();
            }
            //back to normal
            this->start(59500);
        }
    }
#ifdef ON_LINUX
    updatePotdProgressMain();
#endif
}

#ifdef ON_LINUX
void nonGUI::updatePotdProgressMain(){
    if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
        Global::setUnityProgressbarValue((float) (Global::getSecondsTillHour("00:00")/86400.0));
    }
}
#endif //#ifdef ON_LINUX

void nonGUI::connectToUpdateSecondsSlot(){
    if(gv.potdRunning){
        connect(this, SIGNAL(timeout()), this, SLOT(checkPicOfDay()));
    }
    else
    {
        connect(this, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    }
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
        Global::setUnityProgressBarEnabled(true);
    }
#endif
}

void nonGUI::disconnectFromSlot(){
    disconnect(this, 0, this, 0);
}

void nonGUI::connectToCheckInternet(){
    connect(this, SIGNAL(timeout()), this, SLOT(waitForInternetConnection()));
}

void nonGUI::continueWithLiveEarth(){
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();
    totalSeconds_=LIVEARTH_INTERVAL;
    if(!gv.firstTimeout){
        globalParser_->livearth();
    }
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(totalSeconds_, 1);
    }
    this->start(1000);
}

void nonGUI::continueWithWebsite(){
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();
    //getting the required values from the settings...
    QDesktopWidget wid;
    gv.screenWidth=wid.width();
    gv.screenHeight=wid.height();

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

    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));

    websiteSnapshot_->setParameters(QUrl(gv.websiteWebpageToLoad), gv.screenWidth, gv.screenHeight);
    websiteSnapshot_->setWaitAfterFinish(gv.websiteWaitAfterFinishSeconds);
    websiteSnapshot_->setJavascriptConfig(gv.websiteJavascriptEnabled, gv.websiteJavascriptCanReadClipboard);
    websiteSnapshot_->setJavaEnabled(gv.websiteJavaEnabled);
    websiteSnapshot_->setLoadImagesEnabled(gv.websiteLoadImages);
    websiteSnapshot_->setCrop(gv.websiteCropEnabled, gv.websiteCropArea);

    if(gv.websiteLoginEnabled){
        if(!gv.websiteRedirect || gv.websiteFinalPageToLoad.isEmpty()){
            gv.websiteFinalPageToLoad=gv.websiteWebpageToLoad;
        }
        if(gv.websiteSimpleAuthEnabled){
            websiteSnapshot_->setSimpleAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
        }
        else
        {
            if(gv.websiteExtraUsernames.count()>0 || gv.websiteExtraPasswords.count()>0)
            {
                websiteSnapshot_->setComplexAuthenticationWithPossibleFields(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad, gv.websiteExtraUsernames, gv.websiteExtraPasswords, true);
            }
            else
            {
                websiteSnapshot_->setComplexAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
            }
        }
    }
    else
    {
        websiteSnapshot_->disableAuthentication();
    }

    websiteSnapshot_->setTimeout(WEBSITE_TIMEOUT);

    totalSeconds_=Global::websiteSliderValueToSeconds(gv.websiteInterval);
    if(!gv.firstTimeout){
        websiteSnapshot_->start();
    }
    if(!secondsLeft_){
        secondsLeft_=totalSeconds_;
    }
    Global::resetSleepProtection(secondsLeft_);
    if(gv.independentIntervalEnabled){
        Global::saveSecondsLeftNow(secondsLeft_, 2);
    }
#ifdef ON_LINUX
    Global::changeIndicatorSelection("website");
#endif //#ifdef ON_LINUX
    this->start(1000);
}

bool nonGUI::continueWithClock(){
    gv.defaultWallpaperClock=settings->value("default_wallpaper_clock","").toString();
    if(gv.defaultWallpaperClock.isEmpty() || gv.defaultWallpaperClock=="None" )
    {
        globalParser_->desktopNotify(tr("No default wallpaper clock has been set."), false, "info");
        Global::error("No default wallpaper clock has been set. Make sure that after installing the wallpaper clock you have clicked at set as default.");
        return false;
    }
    else if(!QFile::exists(gv.defaultWallpaperClock))
    {
        globalParser_->desktopNotify(tr("The default wallpaper clock does not exist"), false, "info");
        Global::error("Default wallpaper clock does not exist. Make sure the wallpaper clock is installed properly.");
        return false;
    }
    Global::readClockIni(gv.defaultWallpaperClock);
    wallpaperClocksMonthChecked_=settings->value("month",true).toBool();
    wallpaperClocksDayOfMonthChecked_=settings->value("day_of_month",true).toBool();
    wallpaperClocksDayOfWeekChecked_=settings->value("day_of_week",true).toBool();
    wallpaperClocksAmPmChecked_=settings->value("am_pm",true).toBool();
    wallpaperClocksHourChecked_=settings->value("hour",true).toBool();
    wallpaperClocksMinutesChecked_=settings->value("minute",true).toBool();

    QString wall_clock = globalParser_->wallpaperClockNow(gv.defaultWallpaperClock, wallpaperClocksMinutesChecked_, wallpaperClocksHourChecked_, wallpaperClocksAmPmChecked_, wallpaperClocksDayOfWeekChecked_, wallpaperClocksDayOfMonthChecked_ , wallpaperClocksMonthChecked_ );
    if(gv.setAverageColor){
        Global::setAverageColor(wall_clock);
    }

    QTime time = QTime::currentTime();
    float seconds_left_for_min=time.secsTo(QTime(time.hour(),time.minute()+1,1));

    if(settings->value("minute",true).toBool()){
        wallpaperClocksTotalSeconds_=60;
    }
    else if (settings->value("hour",true).toBool()){
        wallpaperClocksTotalSeconds_=gv.refreshhourinterval;
    }
    else if(settings->value("am_pm",true).toBool() && gv.amPmEnabled){
        wallpaperClocksTotalSeconds_=43200;
    }
    else if(settings->value("day_of_week",true).toBool() || settings->value("day_of_month",true).toBool() || settings->value("month",true).toBool()){
        wallpaperClocksTotalSeconds_=86400;
    }
    this->connectToUpdateSecondsSlot();
    this->connectToServer();

    if(settings->value("minute",true).toBool())
    {
        secondsLeft_=seconds_left_for_min;
    }
    else if(settings->value("hour",true).toBool())
    {
        secondsLeft_=(((QString::number(time.minute()/gv.refreshhourinterval).toInt()+1)*gv.refreshhourinterval-time.minute()-1)*60+seconds_left_for_min);
    }
    else if(settings->value("am_pm",true).toBool() && gv.amPmEnabled)
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
    else if(settings->value("day_of_week",true).toBool() || settings->value("day_of_month",true).toBool())
    {
        float seconds_left_for_next_day=time.secsTo(QTime(23,59,59))+2;
        secondsLeft_=seconds_left_for_next_day;
    }
    else if(settings->value("month",true).toBool())
    {
        float seconds_left_for_next_day=time.secsTo(QTime(23,59,59))+2;
        secondsLeft_=seconds_left_for_next_day;
    }
    else
    {

        globalParser_->desktopNotify(tr("At least one option (month, hour etc) must be selected for wallpaper clock to start."), false, "info");
        Global::error("At least one option (month, hour etc) must be selected for wallpaper clock to start.");
        return false;
    }
    totalSeconds_=secondsLeft_;

    gv.runningTimeOfProcess=QDateTime::currentDateTime();
    gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(secondsLeft_);

    gv.potdRunning=gv.liveEarthRunning=gv.liveWebsiteRunning=gv.wallpapersRunning=false;
    gv.wallpaperClocksRunning=true;
    Global::updateStartup();
    if(this->isActive()){
        this->stop();
    }
    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome){
        if(gv.unityProgressbarEnabled){
            Global::setUnityProgressBarEnabled(true);
        }
    }
    Global::changeIndicatorSelection("clocks");
#endif //#ifdef ON_LINUX
    this->start(1000);
    return true;
}

void nonGUI::continueWithPotd(bool checkInternetConnection){
    if(checkInternetConnection){
        Global::debug("Checking for internet connection...");
        if (!globalParser_->connectedToInternet()){
            //will check every 3 secs if there's internet connection...
            this->disconnectFromSlot();
            this->connectToCheckInternet();
            this->start(CHECK_INTERNET_INTERVAL);
            return;
        }
    }

    this->disconnectFromSlot();
    this->connectToUpdateSecondsSlot();
    this->connectToServer();

    //yearmonthday contains the last time that picture of the day was changed...
    QString lastDaySet=settings->value("last_day_potd_was_set", "").toString();
    QString dateTimeNow = QDateTime::currentDateTime().toString("dd.MM.yyyy");
    if(settings->value("potd_preferences_have_changed", false).toBool() || dateTimeNow!=lastDaySet){
        //the previous time that potd changes was another day
        globalParser_->potd();
    }
    else
    {
        potdSetSameImage();
    }
#ifdef ON_LINUX
    updatePotdProgressMain();
    Global::changeIndicatorSelection("potd");
    gv.doNotToggleRadiobuttonFallback=false;
#endif //#ifdef ON_LINUX
    this->start(59500);
}

void nonGUI::connectToServer()
{
    localServer_ = new QLocalServer(this);

    if(!localServer_->listen(QString(SOCKET_SERVER_NAME))){
#ifdef ON_LINUX
        //if not in windows, remove the socket and retry...
        QLocalServer::removeServer(QString(SOCKET_SERVER_NAME));
        if(!localServer_->listen(QString(SOCKET_SERVER_NAME))){
            Global::error("Could not connect to the local socket server.");
            return;
        }
#else
        //if in windows, there doesn't seem to be a solution
        Global::error("Could not connect to the local socket server.");
        return;
#endif //#ifdef ON_LINUX

    }

    connect(localServer_, SIGNAL(newConnection()), this, SLOT(newSocketConnection()));
}

void nonGUI::newSocketConnection(){
    QLocalSocket *clientConnection = localServer_->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32)){
        qApp->processEvents(QEventLoop::AllEvents);
        clientConnection->waitForReadyRead();
    }

    connect(clientConnection, SIGNAL(disconnected()), clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_5_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString message;
    in >> message;
    doAction(message);
}

void nonGUI::messageServer(const QString &message, bool quitAfterwards){
    messageToSendToServer_=message;
    quitAfterMessage_=quitAfterwards;
    socket_ = new QLocalSocket(this);
    socket_->abort();
    connect(socket_, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket_, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socketError()));
    socket_->connectToServer(QString(SOCKET_SERVER_NAME));
}

void nonGUI::socketError(){
    Global::error("The message '"+messageToSendToServer_+"' could not be sent to Wallch.");

    if(quitAfterMessage_){
        Global::debug("Assuming that the previous Wallch instance crashed and starting a new one now. Sorry for this :)");
        connectToServer();

#ifdef ON_LINUX
        setupIndicator();
        if(gv.currentDE==UnityGnome){
            setupUnityShortcuts();
        }
#else
        setupTray();
#endif //#ifdef ON_LINUX
        mainWindowLaunched_=true;
        MainWindow *w = new MainWindow(globalParser_, websiteSnapshot_, wallpaperManager_, 0, 0);
        connectMainwindowWithExternalActions(w);
        w->show();
    }
}

void nonGUI::socketConnected(){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_2);
    out << messageToSendToServer_;
    out.device()->seek(0);
    socket_->write(block);
    socket_->flush();
    if(quitAfterMessage_){
        QTimer::singleShot(100, Qt::CoarseTimer, this, SLOT(quitNow()));
    }
}

void nonGUI::quitNow(){
    qApp->exit(0);
}

void nonGUI::changeWallpaperNow(){

    if(wallpaperManager_->wallpapersCount() < LEAST_WALLPAPERS_FOR_START && !startedWithJustChange_){
        Global::error("Could not get pictures or not enough pictures.");
        globalParser_->desktopNotify(tr("There are not enough valid pictures for the process to continue."), false, "info");
        return;
    }
    if(previousWasClicked_){
        previousWasClicked_=false;
        wallpaperManager_->setBackground(wallpaperManager_->getPreviousWallpaper(), true, true, 1);
    }
    else
    {
        QString image = wallpaperManager_->getNextWallpaper();

        wallpaperManager_->addToPreviousWallpapers(image);

        wallpaperManager_->setBackground(image, true, true, 1);
    }
}

#ifdef ON_LINUX
gboolean indicator_is_scrolled(AppIndicator *, gshort *, gushort direction, gpointer *){
    if(nongui.indicatorScrollTimer_->isActive()){
        return false; //you have to wait for the timeout to finish in order to do another change
    }
    if(gv.wallpapersRunning){
        switch(direction){
        case 0:
            nongui.doAction("--previous");
            break;
        case 1:
            nongui.doAction("--next");
            break;
        default:
            break;
        }
    }
    else
    {
#ifdef ON_LINUX
        Global::changeIndicatorIcon("right");
        nongui.indicatorScrollTimer_->start(INDICATOR_SCROLL_INTERVAL);
#endif
        nongui.doAction("--change");
    }
    return false;
}

void nonGUI::indicatorBackToNormal(){
    Global::changeIndicatorIcon("normal");
}

void show_app_callback(guint, gpointer *){
    nongui.doAction("--focus");
}

void copy_current_image_callback(guint, gpointer *){
    QApplication::clipboard()->setImage(QImage(WallpaperManager::currentBackgroundWallpaper()), QClipboard::Clipboard);
}

void copy_path_current_image_callback(guint, gpointer *){
    QApplication::clipboard()->setText(WallpaperManager::currentBackgroundWallpaper());
}

void open_current_image_callback(guint, gpointer *){
    if(!QDesktopServices::openUrl(QUrl("file:///"+WallpaperManager::currentBackgroundWallpaper()))){
        Global::error("I probably could not open "+WallpaperManager::currentBackgroundWallpaper());
    }
}

void open_folder_current_image_callback(guint, gpointer *){
    WallpaperManager::openFolderOf();
}

void delete_current_image_callback(guint, gpointer *){
    nongui.doAction("--delete-current");
}

void properties_current_image_callback(guint, gpointer *){
    nongui.doAction("--properties");
}

void wallpapers_pause_callback(guint, gpointer *){
    nongui.doAction("--pause");
}

void wallpapers_just_change_callback(guint, gpointer *){
    nongui.doAction("--change");
}

void wallpapers_next_callback(guint, gpointer *){
    nongui.doAction("--next");
}

void wallpapers_previous_callback(guint, gpointer *){
    nongui.doAction("--previous");
}

void start_pictures_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--start");
        return;
    }
}

void stop_everything_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--stop");
        return;
    }
}

void next_pictures_main(guint, gpointer *){
    nongui.doAction("--next");
}

void previous_pictures_mainf(guint, gpointer *){
    nongui.doAction("--previous");
}

void activatelivearth_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--earth");
        return;
    }
}

void deactivatelivearth_main(guint, gpointer *){
    nongui.doAction("--stop");
}

void activatephotoofday_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--potd");
        return;
    }
}

void deactivatephotoofday_main(guint, gpointer *){
    nongui.doAction("--stop");
}

void activatewallpaperclocks_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--clock");
        return;
    }
}

void deactivatewallpaperclocks_main(guint, gpointer *){
    nongui.doAction("--stop");
}

void activatelivewebsite_callback(GtkCheckMenuItem *checkmenuitem, gpointer){
    if(gtk_check_menu_item_get_active(checkmenuitem)){
        if(gv.doNotToggleRadiobuttonFallback){
            gv.doNotToggleRadiobuttonFallback=false;
            return;
        }
        nongui.doAction("--website");
        return;
    }
}

void deactivatelivewebsite_main(guint, gpointer *){
    nongui.doAction("--stop");
}

void indicator_quit_app_main(guint, gpointer *){
    nongui.doAction("--quit");
}

void indicator_preferences_main(guint, gpointer *){
    nongui.doAction("--preferences");
}

void indicator_about_main(guint, gpointer *){
    nongui.doAction("--about");
}

void nonGUI::setupIndicator(){
    gv.currentTheme=settings->value("theme", "autodetect").toString();

    if(gv.currentTheme=="autodetect"){
        if(Global::gsettingsGet("org.gnome.desktop.interface", "gtk-theme")=="Radiance"){
            gv.currentTheme="radiance";
        }
        else
        {
            gv.currentTheme="ambiance";
        }
    }

    if(gv.currentTheme=="ambiance"){
        gv.applicationIndicator = app_indicator_new(INDICATOR_DESCRIPTION, INDICATOR_AMBIANCE_NORMAL, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
        app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_AMBIANCE_NORMAL, INDICATOR_DESCRIPTION);
    }
    else
    {
        gv.applicationIndicator = app_indicator_new(INDICATOR_DESCRIPTION, INDICATOR_RADIANCE_NORMAL, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
        app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_RADIANCE_NORMAL, INDICATOR_DESCRIPTION);
    }

    //main menu
    GtkWidget* indicatormenu = gtk_menu_new();

    //show app option
    showAppOption_ = gtk_menu_item_new_with_label(tr("Show").toLocal8Bit().data());
    g_signal_connect(showAppOption_, "activate", G_CALLBACK(show_app_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), showAppOption_);

    //current background submenu
    GtkWidget *currentImageSubmenu_main = gtk_menu_new();
    GtkWidget *currentImageSubmenuItem_main = gtk_menu_item_new_with_label(tr("Current Background").toLocal8Bit().data());
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(currentImageSubmenuItem_main), currentImageSubmenu_main);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), currentImageSubmenuItem_main);

    openImageOfCurImageOption = gtk_menu_item_new_with_label(tr("Open Image").toLocal8Bit().data());
    g_signal_connect(openImageOfCurImageOption, "activate", G_CALLBACK(open_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), openImageOfCurImageOption);

    openPathOfCurImageOption_ = gtk_menu_item_new_with_label(tr("Open Folder").toLocal8Bit().data());
    g_signal_connect(openPathOfCurImageOption_, "activate", G_CALLBACK(open_folder_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), openPathOfCurImageOption_);

    copyImageOfCurImageOption_ = gtk_menu_item_new_with_label(tr("Copy Image").toLocal8Bit().data());
    g_signal_connect(copyImageOfCurImageOption_, "activate", G_CALLBACK(copy_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), copyImageOfCurImageOption_);

    copyPathOfCurImageOption_ = gtk_menu_item_new_with_label(tr("Copy Path").toLocal8Bit().data());
    g_signal_connect(copyPathOfCurImageOption_, "activate", G_CALLBACK(copy_path_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), copyPathOfCurImageOption_);

    deleteCurImageOption_ = gtk_menu_item_new_with_label(tr("Delete").toLocal8Bit().data());
    g_signal_connect(deleteCurImageOption_, "activate", G_CALLBACK(delete_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), deleteCurImageOption_);

    propertiesOfCurImageOption_ = gtk_menu_item_new_with_label(tr("Properties").toLocal8Bit().data());
    g_signal_connect(propertiesOfCurImageOption_, "activate", G_CALLBACK(properties_current_image_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(currentImageSubmenu_main), propertiesOfCurImageOption_);

    //separator
    gtk_menu_shell_append ( GTK_MENU_SHELL (indicatormenu), gtk_separator_menu_item_new());

    GSList *indicatorRadiobuttonsList = NULL;

    gv.wallpapersRadiobutton = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, tr("Wallpapers").toLocal8Bit().data());
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.wallpapersRadiobutton));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpapersRadiobutton);

    gv.wallpapersJustChange = gtk_menu_item_new_with_label(QString("   "+tr("Change wallpaper once")).toLocal8Bit().data());
    g_signal_connect(gv.wallpapersJustChange, "activate", G_CALLBACK(wallpapers_just_change_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpapersJustChange);

    gv.wallpapersPlayPause = gtk_menu_item_new_with_label(QString("   "+tr("Pause")).toLocal8Bit().data());
    g_signal_connect(gv.wallpapersPlayPause, "activate", G_CALLBACK(wallpapers_pause_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpapersPlayPause);

    gv.wallpapersNext = gtk_menu_item_new_with_label(QString("   "+tr("Next")).toLocal8Bit().data());
    g_signal_connect(gv.wallpapersNext, "activate", G_CALLBACK(wallpapers_next_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpapersNext);

    gv.wallpapersPrevious = gtk_menu_item_new_with_label(QString("   "+tr("Previous")).toLocal8Bit().data());
    g_signal_connect(gv.wallpapersPrevious, "activate", G_CALLBACK(wallpapers_previous_callback), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpapersPrevious);

    gv.liveEarthRadiobutton = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, tr("Live Earth").toLocal8Bit().data());
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.liveEarthRadiobutton));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.liveEarthRadiobutton);

    gv.potdRadiobutton = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, tr("Picture Of The Day").toLocal8Bit().data());
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.potdRadiobutton));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.potdRadiobutton);

    gv.wallpaperClocksRadiobutton = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, tr("Wallpaper Clocks").toLocal8Bit().data());
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.wallpaperClocksRadiobutton));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.wallpaperClocksRadiobutton);

    gv.liveWebsiteRadiobutton = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, tr("Live Website").toLocal8Bit().data());
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.liveWebsiteRadiobutton));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.liveWebsiteRadiobutton);

    gv.iAmNotThere = gtk_radio_menu_item_new_with_label(indicatorRadiobuttonsList, "null");
    indicatorRadiobuttonsList=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM (gv.iAmNotThere));
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), gv.iAmNotThere);

    /*
     * Decide what will be checked has to be done prior to connecting signals so as
     * not to emit any because of the set_active process
     */

    if(gv.wallpapersRunning){
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.wallpapersRadiobutton), TRUE);
    }
    else if(gv.liveEarthRunning){
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.liveEarthRadiobutton), TRUE);
    }
    else if(gv.potdRunning){
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.potdRadiobutton), TRUE);
    }
    else if(gv.wallpaperClocksRunning){
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.wallpaperClocksRadiobutton), TRUE);
    }
    else if(gv.liveWebsiteRunning){
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.liveWebsiteRadiobutton), TRUE);
    }
    else
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.iAmNotThere), TRUE);
    }

    //connecting radiobuttons with their functions

    //this is the only radiobutton that needs the activate signal rather than the toggled one, because we want it to react as "stop" as well
    g_signal_connect(gv.wallpapersRadiobutton, "activate", G_CALLBACK(start_pictures_callback), this);
    g_signal_connect(gv.liveEarthRadiobutton, "activate", G_CALLBACK(activatelivearth_callback), this);
    g_signal_connect(gv.potdRadiobutton, "activate", G_CALLBACK(activatephotoofday_callback), this);
    g_signal_connect(gv.wallpaperClocksRadiobutton, "activate", G_CALLBACK(activatewallpaperclocks_callback), this);
    g_signal_connect(gv.liveWebsiteRadiobutton, "activate", G_CALLBACK(activatelivewebsite_callback), this);

    //separator
    gtk_menu_shell_append ( GTK_MENU_SHELL (indicatormenu), gtk_separator_menu_item_new());

    //preferences option
    showPreferencesOption_ = gtk_menu_item_new_with_label(tr("Preferences").toLocal8Bit().data());
    g_signal_connect(showPreferencesOption_, "activate", G_CALLBACK(indicator_preferences_main), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), showPreferencesOption_);

    //about option
    showAboutOption_ = gtk_menu_item_new_with_label(tr("About").toLocal8Bit().data());
    g_signal_connect(showAboutOption_, "activate", G_CALLBACK(indicator_about_main), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), showAboutOption_);

    //quit option
    quitAppOption_ = gtk_menu_item_new_with_label(tr("Quit").toLocal8Bit().data());
    g_signal_connect(quitAppOption_, "activate", G_CALLBACK(indicator_quit_app_main), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatormenu), quitAppOption_);

    g_signal_connect(G_OBJECT(gv.applicationIndicator), "scroll-event", G_CALLBACK(&indicator_is_scrolled), (gpointer)this);

    app_indicator_set_status(gv.applicationIndicator, APP_INDICATOR_STATUS_ACTIVE);

    gtk_widget_show_all(indicatormenu);
    gtk_widget_hide(gv.iAmNotThere);
    gtk_widget_hide(gv.wallpapersJustChange);

    if(!gv.wallpapersRunning){
        Global::showWallpapersIndicatorControls(false, true);
    }

    app_indicator_set_menu(gv.applicationIndicator, GTK_MENU (indicatormenu));

    indicatorScrollTimer_ = new QTimer(this);
    indicatorScrollTimer_->setSingleShot(true);
    connect(indicatorScrollTimer_, SIGNAL(timeout()), this, SLOT(indicatorBackToNormal()));
}
#ifdef ON_LINUX
void nonGUI::unityProgressbarSetEnabled(bool enabled){
    if(this->isActive()){
        Global::setUnityProgressBarEnabled(enabled);
        if(enabled){
            Global::setUnityProgressbarValue(0);
        }
    }
}

#endif //#ifdef ON_LINUX
#else

//System tray icon Code
void nonGUI::setupTray()
{
    showWindowAction_ = new QAction(tr("Show"), this);
    connect(showWindowAction_, SIGNAL(triggered()), this, SLOT(trayActionShowWindow()));

    openCurrentImageAction_ = new QAction(tr("Open Image"), this);
    connect(openCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImage()));

    openCurrentImageFolderAction_ = new QAction(tr("Open Folder"), this);
    connect(openCurrentImageFolderAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImageFolder()));

    copyCurrentImageAction_ = new QAction(tr("Copy Image"), this);
    connect(copyCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionCopyImage()));

    copyCurrentImagePathAction_ = new QAction(tr("Copy Path"), this);
    connect(copyCurrentImagePathAction_, SIGNAL(triggered()), this, SLOT(trayActionCopyPath()));

    deleteCurrentImageAction_ = new QAction(tr("Delete"), this);
    connect(deleteCurrentImageAction_, SIGNAL(triggered()), this, SLOT(trayActionDeleteCurrentImage()));

    openCurrentImagePropertiesAction_ = new QAction(tr("Properties"), this);
    connect(openCurrentImagePropertiesAction_, SIGNAL(triggered()), this, SLOT(trayActionOpenCurrentImageProperties()));

    wallpapersAction_ = new QAction(tr("Wallpapers"), this);
    connect(wallpapersAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapers()));

    wallpapersOnceAction_ = new QAction("   "+tr("Change wallpaper once"), this);
    connect(wallpapersOnceAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersOnce()));

    wallpapersPauseAction_ = new QAction("   "+tr("Pause"), this);
    connect(wallpapersPauseAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersPause()));

    wallpapersNextAction_ = new QAction("   "+tr("Next"), this);
    connect(wallpapersNextAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersNext()));

    wallpapersPreviousAction_ = new QAction("   "+tr("Previous"), this);
    connect(wallpapersPreviousAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpapersPrevious()));

    liveEarthAction_ = new QAction(tr("Live Earth"), this);
    connect(liveEarthAction_, SIGNAL(triggered()), this, SLOT(trayActionLiveEarth()));

    pictureOfTheDayAction_ = new QAction(tr("Picture Of The Day"), this);
    connect(pictureOfTheDayAction_, SIGNAL(triggered()), this, SLOT(trayActionPictureOfTheDay()));

    wallpaperClocksAction_ = new QAction(tr("Wallpaper Clocks"), this);
    connect(wallpaperClocksAction_, SIGNAL(triggered()), this, SLOT(trayActionWallpaperClocks()));

    liveWebsiteAction_ = new QAction(tr("Live Website"), this);
    connect(liveWebsiteAction_, SIGNAL(triggered()), this, SLOT(trayActionLiveWebsite()));

    preferencesAction_ = new QAction(tr("Preferences"), this);
    connect(preferencesAction_, SIGNAL(triggered()), this, SLOT(trayActionPreferences()));

    aboutAction_ = new QAction(tr("About"), this);
    connect(aboutAction_, SIGNAL(triggered()), this, SLOT(trayActionAbout()));

    quitAction_ = new QAction(tr("Exit"), this);
    connect(quitAction_, SIGNAL(triggered()), this, SLOT(trayActionQuit()));

    doubleClick_ = new QTimer(this);
    connect(doubleClick_, SIGNAL(timeout()), this, SLOT(trayActionWallpapersNext()));
    doubleClick_->setSingleShot(true);

    trayIconMenu_ = new QMenu();
    createTray();

    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setContextMenu(trayIconMenu_);
    connect(trayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivatedActions(QSystemTrayIcon::ActivationReason)));
    trayIcon_->setIcon(QIcon(":/icons/Pictures/wallch.png"));
    trayIcon_->show();
}

void nonGUI::trayActivatedActions(QSystemTrayIcon::ActivationReason reason)
{
    if(reason==2) //double-click, show window
    {
        doubleClick_->stop();
        doAction("--focus");
    }
    else if(reason==3){//single-click, wait to see if user double clicks, else next image
        int double_click_time = GetDoubleClickTime();
        doubleClick_->start(double_click_time);
    }
    else if(reason==4) //middle-click, next image
    {
        doAction("--next");
    }
}

void nonGUI::createTray()
{
    trayIconMenu_->clear();
    trayIconMenu_->addAction(showWindowAction_);
    currentImageMenu_ = new QMenu("Current Image");
    currentImageMenu_->addAction(openCurrentImageAction_);
    currentImageMenu_->addAction(openCurrentImageFolderAction_);
    currentImageMenu_->addAction(copyCurrentImagePathAction_);
    currentImageMenu_->addAction(copyCurrentImageAction_);
    currentImageMenu_->addAction(deleteCurrentImageAction_);
    currentImageMenu_->addAction(openCurrentImagePropertiesAction_);
    trayIconMenu_->addMenu(currentImageMenu_);
    trayIconMenu_->addSeparator();
    trayIconMenu_->addAction(wallpapersAction_);
    if(!gv.wallpapersRunning)
        trayIconMenu_->addAction(wallpapersOnceAction_);
    if(gv.wallpapersRunning)
    {
        wallpapersAction_->setCheckable(true);
        wallpapersAction_->setChecked(true);
        if(gv.processPaused){
            wallpapersPauseAction_->setText("   "+tr("Start"));
            trayIconMenu_->addAction(wallpapersPauseAction_);
        }
        else{
            wallpapersPauseAction_->setText("   "+tr("Pause"));
            trayIconMenu_->addAction(wallpapersPauseAction_);
            trayIconMenu_->addAction(wallpapersNextAction_);
            trayIconMenu_->addAction(wallpapersPreviousAction_);
        }
    }
    else if(gv.liveEarthRunning)
    {
        liveEarthAction_->setCheckable(true);
        liveEarthAction_->setChecked(true);
    }
    else if(gv.potdRunning)
    {
        pictureOfTheDayAction_->setCheckable(true);
        pictureOfTheDayAction_->setChecked(true);
    }
    else if(gv.wallpaperClocksRunning)
    {
        wallpaperClocksAction_->setCheckable(true);
        wallpaperClocksAction_->setChecked(true);
    }
    else if(gv.liveWebsiteRunning)
    {
        liveWebsiteAction_->setCheckable(true);
        liveWebsiteAction_->setChecked(true);
    }
    else
    {
        uncheckRunningFeatureOnTray();
    }
    trayIconMenu_->addAction(liveEarthAction_);
    trayIconMenu_->addAction(pictureOfTheDayAction_);
    trayIconMenu_->addAction(wallpaperClocksAction_);
    trayIconMenu_->addAction(liveWebsiteAction_);
    QActionGroup* myGroup = new QActionGroup( this );
    myGroup->addAction(wallpapersAction_);
    myGroup->addAction(liveEarthAction_);
    myGroup->addAction(pictureOfTheDayAction_);
    myGroup->addAction(wallpaperClocksAction_);
    myGroup->addAction(liveWebsiteAction_);
    trayIconMenu_->addSeparator();
    trayIconMenu_->addAction(preferencesAction_);
    trayIconMenu_->addAction(aboutAction_);
    trayIconMenu_->addAction(quitAction_);
}

void nonGUI::trayActionShowWindow()
{
    doAction("--focus");
}

void nonGUI::trayActionCopyPath()
{
    QApplication::clipboard()->setText(WallpaperManager::currentBackgroundWallpaper());
}

void nonGUI::trayActionCopyImage()
{
    QApplication::clipboard()->setImage(QImage(WallpaperManager::currentBackgroundWallpaper()), QClipboard::Clipboard);
}

void nonGUI::trayActionOpenCurrentImage()
{
    if(!QDesktopServices::openUrl(QUrl("file:///"+WallpaperManager::currentBackgroundWallpaper()))){
        Global::error("I probably could not open "+WallpaperManager::currentBackgroundWallpaper());
    }
}

void nonGUI::trayActionOpenCurrentImageFolder()
{
    Global::openFolderOf("");
}

void nonGUI::trayActionDeleteCurrentImage()
{
    doAction("--delete-current");
}

void nonGUI::trayActionOpenCurrentImageProperties()
{
    doAction("--properties");
}

void nonGUI::trayActionWallpapers()
{
    if(!gv.wallpapersRunning){
        doAction("--start");
    }
    else{
        doAction("--stop");
    }

    createTray();
}

void nonGUI::trayActionWallpapersOnce()
{
    doAction("--change");
}

void nonGUI::trayActionWallpapersPause()
{
    doAction("--pause");
}

void nonGUI::trayActionWallpapersNext()
{
    doAction("--next");
}

void nonGUI::trayActionWallpapersPrevious()
{
    doAction("--previous");
}

void nonGUI::trayActionLiveEarth()
{
    doAction("--earth");
    createTray();
}

void nonGUI::trayActionPictureOfTheDay()
{
    doAction("--potd");
    createTray();
}

void nonGUI::trayActionWallpaperClocks()
{
    doAction("--clock");
    createTray();
}

void nonGUI::trayActionLiveWebsite()
{
    doAction("--website");
    createTray();
}

void nonGUI::trayActionPreferences()
{
    doAction("--preferences");
}

void nonGUI::trayActionAbout()
{
    doAction("--about");
}

void nonGUI::trayActionQuit()
{
    doAction("--quit");
}

void nonGUI::uncheckRunningFeatureOnTray()
{
    wallpapersAction_->setCheckable(false);
    liveEarthAction_->setCheckable(false);
    pictureOfTheDayAction_->setCheckable(false);
    wallpaperClocksAction_->setCheckable(false);
    liveWebsiteAction_->setCheckable(false);
}

//End of System tray icon code
#endif //#ifdef ON_LINUX

void nonGUI::propertiesDestroyed()
{
    propertiesShown_=false;
}

void nonGUI::preferencesDestroyed(){
    gv.preferencesDialogShown=false;
}

void nonGUI::doAction(const QString &message){
    /*
     * This function controls messages coming from external sources,
     * like from the indicator (or tray) and the unity launcher.
     */

    if(message=="--focus"){

        if(mainWindowLaunched_){
            Global::debug("Focusing to the Wallch window.");
            Q_EMIT signalFocus();
            return;
        }

        Global::debug("Loading the Wallch window.");

#ifdef ON_LINUX
        if(gv.useShortcutNext){
            keybinder_unbind(gv.nextShortcut.toLocal8Bit().data(), key_action_next);
        }
#endif

        //stop any process from nonGUI (they will continue to GUI)
        if(this->isActive()){
            this->stop();
        }

        resetWatchFolders(true);

        Global::debug("Focus was requested! Continuing the process to the Graphical Interface");

        if(websiteSnapshot_){
            disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));
        }

        mainWindowLaunched_=true;

        MainWindow *w;

        if(gv.wallpapersRunning){
            w = new MainWindow(globalParser_, websiteSnapshot_, wallpaperManager_, secondsLeft_, totalSeconds_);
        }
        else if(gv.liveWebsiteRunning || gv.wallpaperClocksRunning || gv.liveEarthRunning){
            w = new MainWindow(globalParser_, websiteSnapshot_, wallpaperManager_, secondsLeft_, 0);
        }
        else
        {
            w = new MainWindow(globalParser_, websiteSnapshot_, wallpaperManager_, 0, 0);
        }

        connectMainwindowWithExternalActions(w);
        w->show();
    }
    else if(message=="--earth"){
        if(mainWindowLaunched_){
            if(gv.liveEarthRunning){
                Global::debug("Stopping the Live Earth process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Live Earth process.");
                Q_EMIT signalActivateLivearth();
            }
            return;
        }

        if(gv.liveEarthRunning){
            Global::debug("Stopping the Live Earth process.");
            doAction("--stop");
            return;
        }

        Global::debug("Checking for internet connection...");
        if(!globalParser_->connectedToInternet()){
            Global::error("Could not connect to the internet. Live Earth is not enabled.");
            return;
        }

        Global::debug("Switching to Live Earth mode.");
        if(gv.liveWebsiteRunning){
            if(websiteSnapshot_->isLoading()){
                websiteSnapshot_->stop();
            }
        }
        if(gv.potdRunning){
            globalParser_->abortDownload();
        }

#ifdef ON_LINUX
        Global::showWallpapersIndicatorControls(false, true);
#endif

        gv.liveWebsiteRunning=gv.wallpaperClocksRunning=gv.potdRunning=gv.wallpapersRunning=false;
        gv.liveEarthRunning=true;
        Global::updateStartup();
        if(this->isActive()){
            this->stop();
        }
        this->disconnectFromSlot();
        this->connectToUpdateSecondsSlot();
        globalParser_->livearth();
        secondsLeft_=totalSeconds_=LIVEARTH_INTERVAL;
        Global::resetSleepProtection(secondsLeft_);
        this->start(1000);
#ifdef ON_LINUX
        Global::changeIndicatorSelection("earth");
        if(gv.currentDE==UnityGnome){
            if(gv.unityProgressbarEnabled){
                Global::setUnityProgressBarEnabled(true);
            }
            setUnityShortcutsState(true, false, false, false);
        }
#endif //#ifdef ON_LINUX
    }
    else if(message=="--potd"){
        if(mainWindowLaunched_){
            if(gv.potdRunning){
                Global::debug("Stopping the Picture Of The Day process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Picture Of The Day process.");
                Q_EMIT signalActivatePotd();
            }
            return;
        }

        if(gv.potdRunning){
            Global::debug("Stopping the Picture Of The Day process.");
            doAction("--stop");
            return;
        }

        Global::debug("Checking for internet connection...");
        if(!globalParser_->connectedToInternet()){
            Global::error("Could not connect to the internet. Picture Of The Day is not enabled.");
            return;
        }
        Global::debug("Switching to Picture Of The Day mode.");
        if(gv.liveWebsiteRunning){
            if(websiteSnapshot_->isLoading()){
                websiteSnapshot_->stop();
            }
        }
        if(gv.liveEarthRunning){
            globalParser_->abortDownload();
        }
#ifdef ON_LINUX
        Global::showWallpapersIndicatorControls(false, true);
#endif
        gv.liveEarthRunning=gv.liveWebsiteRunning=gv.wallpaperClocksRunning=gv.wallpapersRunning=false;
        gv.potdRunning=true;
        Global::updateStartup();
        if(this->isActive()){
            this->stop();
        }
        this->disconnectFromSlot();
        this->continueWithPotd(true);
#ifdef ON_LINUX
        Global::changeIndicatorSelection("potd");
        if(gv.currentDE==UnityGnome){
            setUnityShortcutsState(true, false, false, false);
        }
#endif //#ifdef ON_LINUX
    }
    else if(message=="--clock")
    {
        if(mainWindowLaunched_){
            if(gv.wallpaperClocksRunning){
                Global::debug("Stopping the Wallpaper Clocks process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Wallpaper Clocks process.");
                Q_EMIT signalActivateWallClocks();
            }
            return;
        }

        if(gv.wallpaperClocksRunning){
            Global::debug("Stopping the Wallpaper Clocks process.");
            doAction("--stop");
            return;
        }

        if(!this->continueWithClock()){
            Global::error("There was a problem with starting the process. Wallpaper Clocks is not enabled.");
            doAction("--stop");
            return;
        }
#ifdef ON_LINUX
        Global::showWallpapersIndicatorControls(false, true);
#endif
        gv.liveEarthRunning=gv.liveWebsiteRunning=gv.potdRunning=gv.wallpapersRunning=false;
        gv.wallpaperClocksRunning=true;
        Global::updateStartup();
        Global::debug("Switching to Wallpaper Clocks mode.");
        if(gv.liveWebsiteRunning){
            if(websiteSnapshot_->isLoading()){
                websiteSnapshot_->stop();
            }
        }

#ifdef ON_LINUX
        Global::changeIndicatorSelection("clocks");
        if(gv.currentDE==UnityGnome){
            setUnityShortcutsState(true, false, false, false);
        }
#endif //#ifdef ON_LINUX
    }
    else if(message=="--website"){
        if(mainWindowLaunched_){
            if(gv.liveWebsiteRunning){
                Global::debug("Stopping the Live Website process.");
                Q_EMIT closeWhatsRunning();
            }
            else
            {
                Global::debug("Activating the Live Website process.");
                Q_EMIT signalActivateLiveWebsite();
            }
            return;
        }

        if(gv.liveWebsiteRunning){
            Global::debug("Stopping the Live Website process.");
            doAction("--stop");
            return;
        }
        if(websiteSnapshot_==NULL){
            websiteSnapshot_ = new WebsiteSnapshot();
        }

        Global::debug("Switching to Live Website mode.");
        if(gv.liveEarthRunning || gv.potdRunning){
            globalParser_->abortDownload();
        }
#ifdef ON_LINUX
        Global::showWallpapersIndicatorControls(false, true);
#endif
        gv.potdRunning=gv.liveEarthRunning=gv.wallpaperClocksRunning=gv.wallpapersRunning=false;
        gv.liveWebsiteRunning=true;
        Global::updateStartup();
        if(this->isActive()){
            this->stop();
        }
        this->disconnectFromSlot();
        secondsLeft_=0;
        this->continueWithWebsite();
#ifdef ON_LINUX
        Global::changeIndicatorSelection("website");
        if(gv.currentDE==UnityGnome){
            if(gv.unityProgressbarEnabled){
                Global::setUnityProgressBarEnabled(true);
            }
            setUnityShortcutsState(true, false, false, false);
        }
#endif //#ifdef ON_LINUX
    }
    else if(message=="--start"){

        if(mainWindowLaunched_){
            if(!gv.wallpapersRunning || gv.processPaused){
                Global::debug("Activating the Wallpapers process.");
                Q_EMIT signalStart();
            }
            else
            {
                Global::debug("Stopping the Wallpapers process.");
                doAction("--stop");
            }
            return;
        }

        if(gv.wallpapersRunning){
            if(gv.processPaused){
                doAction("--pause"); //continue from the pause
            }
            else
            {
                Global::debug("Stopping the Wallpapers process.");
                doAction("--stop");
            }
        }
        else
        {
            if(startedWithLiveEarth_ || startedWithWebsite_ || startedWithPotd_ || startedWithClock_){
                startedWithNone_=startedWithLiveEarth_=startedWithWebsite_=startedWithPotd_=startedWithClock_=false;
                checkSettings(true);
                if(!getPicturesLocation(true)){
                    return;
                }
                if(gv.randomImagesEnabled){
                    wallpaperManager_->setRandomMode(true);
                }
            }
            Global::debug("Switching to Wallpapers mode.");
            gv.wallpaperClocksRunning=gv.potdRunning=gv.liveEarthRunning=gv.liveWebsiteRunning=false;
            gv.wallpapersRunning=true;
            Global::updateStartup();

            if(this->isActive()){
                this->stop();
            }
            this->disconnectFromSlot();
            this->connectToUpdateSecondsSlot();

            getDelay();
            secondsLeft_=0;
            Global::resetSleepProtection(secondsLeft_);
            this->start(1000);
#ifdef ON_LINUX
            if(gv.currentDE==UnityGnome){
                if(gv.unityProgressbarEnabled){
                    Global::setUnityProgressBarEnabled(true);
                }

                setUnityShortcutsState(true, true, true, true);
            }

            Global::changeIndicatorSelection("wallpapers");
            Global::showWallpapersIndicatorControls(true, true);
#endif //#ifdef ON_LINUX
        }
    }
    else if(message=="--change"){
        if(mainWindowLaunched_){
            Q_EMIT signalOnce();
            return;
        }

        QString parentFolder=getCurrentWallpapersFolder();
        if(!QDir(parentFolder).exists()){
            globalParser_->desktopNotify(tr("Folder doesn't exist for the process to continue."), false, "info");
            return;
        }

        wallpaperManager_->clearWallpapers();

        Q_FOREACH(QString curFolder, Global::listFolders(parentFolder, true, true)){
            getFilesFromFolder(curFolder);
        }

        if(gv.randomImagesEnabled){
            wallpaperManager_->setRandomMode(true);
        }

        wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
    }
    else if(message=="--pause"){

        if(mainWindowLaunched_){
            Q_EMIT signalPause();
            return;
        }

        if(!gv.wallpapersRunning && !gv.processPaused){
            Global::error("Cannot pause to any other process rather than Wallpapers!");
            return;
        }
        if(!gv.processPaused){
            Global::debug("Pausing the Wallpapers process.");
            this->stop();
            gv.processPaused=true;
#ifdef ON_LINUX
            if(gv.currentDE==UnityGnome){
                setUnityShortcutsState(true, false, false, false);
            }

            Global::changeIndicatorSelection("pause");
            Global::showWallpapersIndicatorControls(true, false);
#endif //#ifdef ON_LINUX
        }
        else
        {
            Global::resetSleepProtection(secondsLeft_);
            Global::debug("Continuing from the pause...");
            gv.processPaused=false;
#ifdef ON_LINUX
            Global::showWallpapersIndicatorControls(true, true);

            if(gv.currentDE==UnityGnome){
                setUnityShortcutsState(true, true, true, true);
            }
#endif
            this->start(1000);
        }
    }
    else if(message=="--stop"){
        if(mainWindowLaunched_){
#ifdef ON_LINUX
            if(gv.wallpapersRunning){
                Global::showWallpapersIndicatorControls(false, true);
            }
#endif
            Q_EMIT closeWhatsRunning();
            return;
        }

#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome && gv.unityProgressbarEnabled){
            Global::setUnityProgressBarEnabled(false);
        }
#endif

        if(this->isActive()){
            this->stop();
        }

        if(gv.wallpapersRunning)
        {
#ifdef ON_LINUX
            Global::showWallpapersIndicatorControls(false, true);
#endif

            wallpaperManager_->startOver();

            gv.processPaused=false;

            if(gv.randomTimeEnabled){
                srand(time(0));
                totalSeconds_=secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
            }
            else
            {
                secondsLeft_=totalSeconds_;
            }
            if(gv.independentIntervalEnabled){
                //resetting the configuration!
                Global::saveSecondsLeftNow(-1, 0);
            }
        }
        else if(gv.liveEarthRunning || gv.liveWebsiteRunning){
            secondsLeft_=totalSeconds_;
        }
        else if(gv.wallpaperClocksRunning){
            secondsLeft_=totalSeconds_=wallpaperClocksTotalSeconds_;
        }

        if(gv.liveEarthRunning || gv.potdRunning){
            globalParser_->abortDownload();
        }

        gv.wallpapersRunning=gv.liveEarthRunning=gv.potdRunning=gv.wallpaperClocksRunning=gv.liveWebsiteRunning=false;
        Global::updateStartup();
#ifdef ON_LINUX
        Global::changeIndicatorSelection("none");
        if(gv.currentDE==UnityGnome){
            setUnityShortcutsState(false, false, false, false);
        }
#endif

    }
    else if(message=="--next"){

        if(mainWindowLaunched_){
            Q_EMIT signalNext();
            return;
        }

        if(gv.wallpapersRunning){
            if(this->isActive()){
#ifdef ON_LINUX
                Global::changeIndicatorIcon("right");
                indicatorScrollTimer_->start(INDICATOR_SCROLL_INTERVAL);
#endif
                this->stop();
                secondsLeft_=0;
                updateSeconds();
                this->start(1000);
            }
            else
            {
                Global::error("Wallpapers mode is not running in order to use 'next'");
            }
        }
        else
        {
            Global::error("Υou can use 'next' only in Wallpapers mode.");
        }
    }
    else if(message=="--previous"){

        if(mainWindowLaunched_){
            Q_EMIT signalPrevious();
            return;
        }

        if(gv.wallpapersRunning){
            if(this->isActive()){
                if(gv.wallpapersRunning){
#ifdef ON_LINUX
                    Global::changeIndicatorIcon("left");
                    indicatorScrollTimer_->start(INDICATOR_SCROLL_INTERVAL);
#endif
                    this->stop();
                    secondsLeft_=0;
                    previousWasClicked_=true;
                    updateSeconds();
                    this->start(1000);
                }
            }
            else
            {
                Global::error("Wallpapers mode is not running in order to use 'previous'");
            }
        }
        else
        {
            Global::error("Υou can use 'previous' only in Wallpapers mode.");
        }
    }
    else if(message=="--preferences"){

        if(mainWindowLaunched_){
            Q_EMIT signalShowPreferences();
            return;
        }
        if(gv.preferencesDialogShown){
            return;
        }
        gv.preferencesDialogShown=true;
        preferences_ = new Preferences();
        preferences_->setModal(true);
        preferences_->setAttribute(Qt::WA_DeleteOnClose);
        connect(preferences_, SIGNAL(destroyed()), this, SLOT(preferencesDestroyed()));
#ifdef ON_LINUX
        connect(preferences_, SIGNAL(unityProgressbarChanged(bool)), this, SLOT(unityProgressbarSetEnabled(bool)));
#endif
        preferences_->show();
    }
    else if(message=="--properties"){
        if(propertiesShown_){
            return;
        }
        QString imageFilename=WallpaperManager::currentBackgroundWallpaper();
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
    else if(message=="--about"){

        if(mainWindowLaunched_){
            Q_EMIT signalShowAbout();
            return;
        }

        About ab(0);
        ab.exec();
    }
    else if(message=="--delete-current"){
        if(mainWindowLaunched_){
            Q_EMIT signalDeleteCurrent();
            return;
        }
        if(QMessageBox::question(0, QObject::tr("Confirm deletion"), QObject::tr("Are you sure you want to permanently delete the current image?"))==QMessageBox::Yes)
        {
            if(!QFile::remove(WallpaperManager::currentBackgroundWallpaper())){
                QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("There was a problem deleting the current image. Please make sure you have the permission to delete the image or that the image exists."));
            }
        }
    }
    else if(message=="--quit"){
        if(mainWindowLaunched_){
            Q_EMIT signalQuit();
            return;
        }
        Global::debug("See ya next time!");
        qApp->exit(0);
    }
    else if(message.startsWith("CLOCK:")){
        if(mainWindowLaunched_){
            Q_EMIT signalInstallWallClock(message.right(message.length()-6));
        }
        else{
            Global::error("Wallpapper clock installations only on GUI mode!");
        }
    }
    else if(message.startsWith("MONITOR:")){
        QString folder=message.right(message.length()-8);
        if(mainWindowLaunched_){
            Q_EMIT signalAddFolderForMonitor(folder);
            return;
        }

        //update the configuration to point to the new folder
        setDefaultFolderInSettings(folder);

        if(gv.wallpapersRunning){
            researchDirs();
        }
    }
    else{
        Global::error("This message is unknown to me.");
    }
}

QString nonGUI::getCurrentWallpapersFolder(){
    //read the default folder location
    short currentFolderIndex=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(currentFolderIndex);
    QString parentFolder = settings->value("item", gv.currentDeDefaultWallpapersPath).toString();
    settings->endArray();
    return parentFolder;
}

void nonGUI::setDefaultFolderInSettings(const QString &folder){
    bool already_in_settings=false;
    short count = settings->beginReadArray("pictures_locations");
    if(QDir(folder).exists()){
        for(short i=0; i<count; i++)
        {
            settings->setArrayIndex(i);
            if(Global::foldersAreSame(folder, settings->value("item").toString()))
            {
                settings->endArray();
                settings->setValue("currentFolder_index", i);
                already_in_settings=true;
                break;
            }
        }
        settings->sync();
    }
    else
    {
        settings->endArray();
        Global::error("The folder "+folder+" does not exist!");
        return;
    }
    if(!already_in_settings){
        //Add the folder to the index and point to it as default
        settings->endArray();
        settings->beginWriteArray("pictures_locations");
        settings->setArrayIndex(count);
        settings->setValue("item", folder);
        settings->endArray();
        settings->setValue("currentFolder_index", count);
        settings->sync();
    }
}

void nonGUI::setIndependentInterval(const QString &independentInterval){
    if(!independentInterval.contains(".")){
        return;
    }

    QStringList parts=independentInterval.split(".");
    if(parts.count()!=3){
        return;
    }
    if(parts.at(0)=="e"){
        //independence was referring to live earth!
        if(!startedWithLiveEarth_){
            return;
        }
    }
    else if(parts.at(0)=="w"){
        //independence was referring to live website!
        if(!startedWithWebsite_){
            return;
        }
    }
    else
    {
        //independence was referring to wallpapers
        if(startedWithWebsite_ || startedWithLiveEarth_){
            return;
        }
    }
    bool ok;
    int independence_change_seconds_left = parts.at(1).toInt(&ok);
    if(!ok){
        return;
    }
    if(independence_change_seconds_left==-1){
        return;
    }

    QDateTime time_then = QDateTime::fromString(parts.at(2), "yyyy:MM:dd:HH:mm:ss");
    if(time_then.isNull() || !time_then.isValid()){
        return;
    }

    time_then.addSecs(independence_change_seconds_left);

    int secs_diff = time_then.secsTo(QDateTime::currentDateTime());
    independence_change_seconds_left-=secs_diff;

    if(independence_change_seconds_left<0){
        independence_change_seconds_left=0;
    }
    if(independence_change_seconds_left>0){
        Global::debug("Interval independence is enabled ("+QString::number(independence_change_seconds_left)+" seconds)");
        secondsLeft_=independence_change_seconds_left;
    }
}

void nonGUI::checkSettings(bool allSettings){

    // The least checks for the --change argument
    gv.setAverageColor = settings->value("average_color", false).toBool();
    gv.showNotification=settings->value("notification", false).toBool();
    gv.saveHistory=settings->value("history", true).toBool();
#ifdef ON_LINUX
    gv.currentDE=static_cast<DesktopEnvironment>(settings->value("de", 0).toInt());
#endif

    if(allSettings){
        //checks all needed settings
        gv.useShortcutNext=settings->value("use_shortcut_next", false).toBool();

#ifdef ON_LINUX
        gv.unityProgressbarEnabled=settings->value("unity_progressbar_enabled", false).toBool();
#endif

        gv.potdIncludeDescription = settings->value("potd_include_description", true).toBool();
        gv.potdDescriptionBottom = settings->value("potd_description_bottom", true).toBool();
        gv.potdDescriptionFont = settings->value("potd_description_font", "Ubuntu").toString();
        gv.potdDescriptionColor = settings->value("potd_text_color", "#FFFFFF").toString();
        gv.potdDescriptionBackgroundColor = settings->value("potd_background_color", "#000000").toString();
#ifdef ON_LINUX
        gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (gv.currentDE==UnityGnome ? 60 : 0)).toInt();
#else
        gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (0)).toInt();
#endif
        gv.potdDescriptionRightMargin = settings->value("potd_description_right_margin", 0).toInt();
        gv.potdDescriptionBottomTopMargin = settings->value("potd_description_bottom_top_margin", 0).toInt();

        gv.randomTimeEnabled=settings->value("random_time_enabled", false).toBool();
        if(startedWithLiveEarth_ || startedWithWebsite_ || (!startedWithClock_ && !startedWithPotd_ && !gv.randomTimeEnabled)){
            gv.independentIntervalEnabled = settings->value("independent_interval_enabled", true).toBool();
            if(gv.independentIntervalEnabled){
                //check if there is an interval to follow
                setIndependentInterval(settings->value("seconds_left_interval_independence", INTERVAL_INDEPENDENCE_DEFAULT_VALUE).toString());
            }
        }
        else
        {
            gv.independentIntervalEnabled=false;
        }

        gv.randomImagesEnabled=settings->value("random_images_enabled", true).toBool();

        if(!gv.randomImagesEnabled){
            wallpaperManager_->setRandomMode(false);
        }

        gv.firstTimeout=settings->value("first_timeout", false).toBool();
        if(gv.randomTimeEnabled){
            gv.randomTimeFrom=settings->value("random_time_from", 300).toInt();
            gv.randomTimeTo=settings->value("random_time_to", 1200).toInt();
            if(gv.randomTimeFrom>gv.randomTimeTo-3){
                Global::error("The random time is misconfigured, please head to the Preferences dialog to configure it properly!");
                gv.randomTimeEnabled=false;
            }
            else
            {
                totalSeconds_=secondsLeft_=(rand()%(gv.randomTimeTo-gv.randomTimeFrom+1))+gv.randomTimeFrom;
            }
        }
        if(gv.randomImagesEnabled){
            srand(QDateTime::currentMSecsSinceEpoch());
        }
        gv.useShortcutNext=settings->value("use_shortcut_next", false).toBool();
#ifdef ON_LINUX
        if(gv.useShortcutNext){
            gv.nextShortcut=settings->value("shortcut", "").toString();
            keybinder_init();
            if(!keybinder_bind(gv.nextShortcut.toLocal8Bit().data(), key_action_next, NULL)){
                //key could not be binded!
                Global::error("I probably could not bind the key sequence: '"+gv.nextShortcut+"'");
            }
        }
#endif
        gv.potdOnlineUrl=settings->value("potd_online_url", POTD_ONLINE_URL).toString();
        gv.potdOnlineUrlB=settings->value("potd_online_urlB", POTD_ONLINE_URL_B).toString();
        gv.liveEarthOnlineUrl=settings->value("line_earth_online_url", LIVEARTH_ONLINE_URL).toString();
        gv.liveEarthOnlineUrlB=settings->value("live_earth_online_urlB", LIVEARTH_ONLINE_URL_B).toString();
    }
}

void nonGUI::connectMainwindowWithExternalActions(MainWindow *w){
    //connects MainWindow with indicator, unity and command line messages.
    QObject::connect(&nongui, SIGNAL(signalOnce()), w, SLOT(justChangeWallpaper()));
    QObject::connect(&nongui, SIGNAL(signalPause()), w, SLOT(on_startButton_clicked()));
    QObject::connect(&nongui, SIGNAL(signalPrevious()), w, SLOT(on_previous_Button_clicked()));
    QObject::connect(&nongui, SIGNAL(signalNext()), w, SLOT(on_next_Button_clicked()));
    QObject::connect(&nongui, SIGNAL(signalStart()), w, SLOT(on_startButton_clicked()));
    QObject::connect(&nongui, SIGNAL(signalActivateLivearth()), w, SLOT(on_activate_livearth_clicked()));
    QObject::connect(&nongui, SIGNAL(signalActivatePotd()), w, SLOT(on_activate_potd_clicked()));
    QObject::connect(&nongui, SIGNAL(signalActivateWallClocks()), w, SLOT(on_activate_clock_clicked()));
    QObject::connect(&nongui, SIGNAL(signalActivateLiveWebsite()), w, SLOT(on_activate_website_clicked()));
    QObject::connect(&nongui, SIGNAL(closeWhatsRunning()), w, SLOT(closeWhatsRunning()));
    QObject::connect(&nongui, SIGNAL(signalShowPreferences()), w, SLOT(on_action_Preferences_triggered()));
    QObject::connect(&nongui, SIGNAL(signalShowAbout()), w, SLOT(on_action_About_triggered()));
    QObject::connect(&nongui, SIGNAL(signalQuit()), w, SLOT(on_actionQuit_Ctrl_Q_triggered()));
    QObject::connect(&nongui, SIGNAL(signalDeleteCurrent()), w, SLOT(on_actionDelete_triggered()));
    QObject::connect(&nongui, SIGNAL(signalAddFolderForMonitor(const QString&)), w, SLOT(addFolderForMonitor(const QString&)));
    QObject::connect(&nongui, SIGNAL(signalInstallWallClock(QString)), w, SLOT(installWallpaperClock(const QString&)));
#ifdef ON_WIN32
    QObject::connect(&nongui, SIGNAL(signalFocus()), w, SLOT(strongShowApp()));
    QObject::connect(w, SIGNAL(signalUncheckRunningFeatureOnTray()), &nongui , SLOT(uncheckRunningFeatureOnTray()));
    QObject::connect(w, SIGNAL(signalRecreateTray()), &nongui , SLOT(createTray()));
#else
    QObject::connect(&nongui, SIGNAL(signalFocus()), w, SLOT(strongShowApp()));
#endif //#ifdef ON_WIN32
}

std::string nonGUI::percentToProgressbar(short percentage){
    short loop_count=floor((float)(percentage/10.0));
    std::string progressbar_string;
    for(short i=0;i<loop_count;i++)
        progressbar_string+='=';
    loop_count=10-loop_count;
    for(short i=0;i<loop_count;i++)
        progressbar_string+=' ';
    return '['+progressbar_string+"] "+QString::number(percentage).toStdString()+"%";
}

void nonGUI::clearCurrentLine(short previousOutputCount){
    std::cout << '\r';
    for(short j=0;j<previousOutputCount;j++)
        std::cout << " ";
}

void nonGUI::liveWebsiteImageReady(QImage *image, short errorCode){
    if(errorCode==0){
        //no error!

        Global::remove(gv.wallchHomePath+LW_IMAGE+"*");
        QString filename=gv.wallchHomePath+LW_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+".png";
        image->save(filename);
        delete image;

        WallpaperManager::setBackground(filename, true, true, 5);
        QFile::remove(gv.wallchHomePath+LW_PREVIEW_IMAGE);
        QFile(filename).link(gv.wallchHomePath+LW_PREVIEW_IMAGE);
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

void nonGUI::updateSecondsPassed()
{
    settings->setValue("seconds_passed", settings->value("seconds_passed", 0).toUInt()+UPDATE_STATISTICS_SECONDS_INTERVAL/1000);
}

void nonGUI::startStatisticsTimer(){
    updateSecondsPassed_ = new QTimer(this);
    connect(updateSecondsPassed_, SIGNAL(timeout()), this, SLOT(updateSecondsPassed()));
    updateSecondsPassed_->start(UPDATE_STATISTICS_SECONDS_INTERVAL);
}

void nonGUI::viralSettingsOperations(){
    if(QApplication::instance()){
        QTranslator *translator = new QTranslator(this);
        translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_"+settings->value("language_file", "en").toString()+".qm");
        QApplication::installTranslator(translator);
    }

    //checking for first run
    if(settings->value("first-run", true).toBool()){
        settings->setValue("first-run", false);
#ifdef ON_LINUX
        if(!QDir(gv.homePath+"/.config/Wallch/").removeRecursively()){
            Global::error("I probably could not remove previous Wallch configurations!");
        }

        //doing DE detection
        DesktopEnvironment setDe;
        if(QDir("/etc/xdg/lubuntu").exists()){
            setDe=LXDE;
        }
        else if(QFile::exists("/usr/bin/xfconf-query")){
            setDe=XFCE;
        }
        else if(QFile::exists("/usr/bin/mate-about")){
            setDe=Mate;
        }
        else if(QFile::exists("/usr/bin/unity"))
        {
            setDe=UnityGnome;
        }
        else
        {
            setDe=Gnome;
        }

        settings->setValue("de", setDe);

        gv.currentDE=setDe;

        //doing OS name detection
        QString osName;
        QFile osDetectionFile("/etc/os-release");
        if(osDetectionFile.exists() && osDetectionFile.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream in(&osDetectionFile);
            QString curLine;
            QString prettyName, name;
            while(!in.atEnd()){
                curLine=in.readLine();
                if(curLine.startsWith("PRETTY_NAME=")){
                    prettyName=curLine.right(curLine.count()-12);
                }
                else if(curLine.startsWith("NAME=")){
                    name=curLine.right(curLine.count()-5);
                }
            }
            if(!prettyName.isEmpty()){
                if(prettyName.contains(" ")){
                    prettyName=prettyName.split(" ")[0];
                }
                prettyName.replace("\"", "");
                osName=prettyName;
            }
            else if(!name.isEmpty()){
                osName=name;
            }
        }
        else
        {
            osDetectionFile.setFileName("/etc/lsb-release");
            if(osDetectionFile.exists() && osDetectionFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                QTextStream in(&osDetectionFile);
                QString currentLine;
                bool found=false;
                while(!in.atEnd()){
                    currentLine=in.readLine();
                    if(currentLine.startsWith("DISTRIB_ID=")){
                        found=true;
                        currentLine=currentLine.right(currentLine.count()-11);
                        break;
                    }
                }
                if(found && !currentLine.isEmpty()){
                    osName=currentLine;
                }
            }
        }
        if(!osName.isEmpty()){
            osDetectionFile.close();
        }

        settings->setValue("osname", osName);

#elif ON_WIN32
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);

        settings->setValue("windows_major_version", QString::number(osvi.dwMajorVersion));
        settings->setValue("windows_minor_version", QString::number(osvi.dwMinorVersion));

        if(!QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Mellori Studio/Wallch/").removeRecursively()){
            Global::error("I probably could not remove previous Wallch configurations!");
        }
#endif //#ifdef ON_LINUX

    }

#ifdef ON_LINUX
    else
    {
        //this is not a first-run
        gv.currentDE=static_cast<DesktopEnvironment>(settings->value("de", 0).toInt());
    }
#endif
    settings->setValue("times_launched", settings->value("times_launched", 0).toUInt()+1); //adding one to times launched
    settings->sync();

    //store the current os name and the current os default wallpapers path so we don't check everytime
#ifdef ON_WIN32
    gv.currentOSName="Windows";
    gv.currentDeDefaultWallpapersPath="C:\\Windows\\Web\\Wallpaper";
#else
    gv.currentOSName=settings->value("osname", "Ubuntu").toString();
    if(gv.currentDE==LXDE && gv.currentOSName=="Ubuntu"){
        gv.currentDeDefaultWallpapersPath="/usr/share/lubuntu/wallpapers";
    }
    else
    {
        gv.currentDeDefaultWallpapersPath="/usr/share/backgrounds";
    }
#endif

#ifdef ON_LINUX
    gv.wallchHomePath=gv.homePath+"/.wallch/";
    gv.cachePath=gv.homePath+"/.cache/wallch/thumbs/";
#elif ON_WIN32
    gv.wallchHomePath=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Mellori Studio/Wallch/";
    gv.cachePath=gv.wallchHomePath+"/.cache/thumbs/";
#endif
    if(!QDir(gv.wallchHomePath).exists()){
        QDir().mkpath(gv.wallchHomePath);
    }
    if(!QDir(gv.cachePath).exists()){
        QDir().mkpath(gv.cachePath);
    }
    Global::updateStartup();
}

int nonGUI::startProgram(int argc, char *argv[]){
    if(argc > 1){
        const struct option longopts[] =
        {
            {"version",   ARG_NOT_REQ,        0, 'v'},
            {"help",      ARG_NOT_REQ,        0, 'h'},
            {"change",     ARG_OPT,  0, 'c'},
            {0,0,0,0},
        };

        int index;
        int iarg=0;

        opterr=0; //turn off getopt error message

        while(iarg != -1)
        {
            iarg = getopt_long(argc, argv, "c:vh", longopts, &index);
            switch (iarg)
            {
            case 'h':
                showUsage(0);
                break;
            case 'v':
                Global::debug("Wallch - Wallpaper Changer, Version "+QString::number(APP_VERSION, 'f', 2));
                return 0;
                break;
            case 'c':
            {
                viralSettingsOperations();
                wallpaperManager_ = new WallpaperManager();

                /*
                 * Just change the current image. This argument always launches when called
                 * correctly (it doesn't matter whether wallch is already running or not)
                */
                startedWithJustChange_=true;
                if(optarg){
                    //probably the user has specified a folder for working with --change.
                    if(index!=2 || argc > 3){
                        showUsage(1);
                    }
                    QString directoryToReadFrom=QString(optarg);
                    if(QDir(directoryToReadFrom).exists()){
                        //It is a directory. Read its contents instead of the default directory.
                        readPictures(QDir(directoryToReadFrom).absolutePath());
                    }
                    else
                    {
                        Global::error(directoryToReadFrom+" - No such directory.");
                        return 1;
                    }
                }
                else
                {
                    //only --change was specified... Read the pictures of the default folder
                    if(!getPicturesLocation(true)){
                        return 1;
                    }
                }

                checkSettings(false);

                if(gv.setAverageColor){
                    //QImage::scaled() crashes without QApplication :(
                    QApplication app(argc, argv);
                    wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
                    app.exit(0);
                }
                else
                {
                    wallpaperManager_->setBackground(wallpaperManager_->randomButNotCurrentWallpaper(), true, true, 1);
                }
                return 0;
                break;
            }
            }
        }
        //second level argument searching!
        QApplication app(argc, argv);
        globalParser_ = new Global(true);
        wallpaperManager_ = new WallpaperManager();
        viralSettingsOperations();
        QApplication::setQuitOnLastWindowClosed(false);

        QStringList arguments=QCoreApplication::arguments();

        if(arguments.contains("--start")){
            if(alreadyRuns()){
                messageServer("--start", true);
                return app.exec();
            }

            bool gotPicLocation=getPicturesLocation(true);

            checkSettings(true);

            Global::resetSleepProtection(secondsLeft_);
            if(gotPicLocation){
                if(!gv.randomTimeEnabled){
                    getDelay();
                    Global::debug("Your Desktop Background will change every "+QString::number(totalSeconds_)+" seconds.");
                }
                else
                {
                    Global::debug("Your Desktop Background will change every ["+QString::number(gv.randomTimeFrom)+"-"+QString::number(gv.randomTimeTo)+"] seconds.");
                }

                if(wallpaperManager_->wallpapersCount()<LEAST_WALLPAPERS_FOR_START){
                    Global::error("Too few pictures for image changing. You need at least "+QString(LEAST_WALLPAPERS_FOR_START)+".");
                    globalParser_->desktopNotify(tr("You cannot change wallpapers if they are less than 2!"), false, "info");
                    return 1;
                }

                if(gv.randomImagesEnabled){
                    wallpaperManager_->setRandomMode(true);
                }

                gv.wallpapersRunning=true;
                Global::updateStartup();
                connectToUpdateSecondsSlot();
            }
            connectToServer();
#ifdef ON_LINUX
            setupIndicator();
            if(gv.currentDE==UnityGnome){
                setupUnityShortcuts();
                if(gotPicLocation){
                    setUnityShortcutsState(true, true, true, true);
                }
                else
                {
                    setUnityShortcutsState(false, false, false, false);
                }
            }
#else
            setupTray();
#endif
            if(gotPicLocation){
                this->start(1000);
            }

            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--earth")){
            if(argc > 2){
                Global::error("Argument --earth doesn't take any other options!");
                showUsage(1);
            }
            if(alreadyRuns()){
                messageServer("--earth", true);
                startStatisticsTimer();
                return app.exec();
            }
            startedWithLiveEarth_=gv.liveEarthRunning=true;
            Global::updateStartup();
            secondsLeft_=LIVEARTH_INTERVAL;
            checkSettings(true);
            Global::resetSleepProtection(secondsLeft_);

            connectToServer();

#ifdef ON_LINUX
            setupIndicator();
#else
            setupTray();
#endif

            Global::debug("Checking for internet connection...");
            if (!globalParser_->connectedToInternet()){
                Global::error("No internet connection, trying again in "+QString::number(CHECK_INTERNET_INTERVAL/1000)+" secs...");
                disconnectFromSlot();
                connectToCheckInternet();
                this->start(CHECK_INTERNET_INTERVAL);
            }
            else
            {
#ifdef ON_LINUX
                if(gv.currentDE==UnityGnome){
                    setupUnityShortcuts();
                    setUnityShortcutsState(true, false, false, false);
                }
#endif
                continueWithLiveEarth();
            }
            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--clock")){
            if(argc>2){
                Global::error("Argument --clock doesn't take any other options!");
                showUsage(1);
            }

            if(alreadyRuns()){
                messageServer("--clock", true);
                startStatisticsTimer();
                return app.exec();
            }
            startedWithClock_=true;
            checkSettings(true);

            connectToServer();

#ifdef ON_LINUX
            setupIndicator();
            if(gv.currentDE==UnityGnome){
                setupUnityShortcuts();
                setUnityShortcutsState(true, false, false, false);
            }
#else
            setupTray();
#endif

            continueWithClock();
            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--potd")){
            if(argc>2){
                Global::error("Argument --potd doesn't take any other options!");
                showUsage(1);
            }
            if(alreadyRuns()){
                messageServer("--potd", true);
                return app.exec();
            }
            startedWithPotd_=gv.potdRunning=true;
            Global::updateStartup();

            connectToServer();

#ifdef ON_LINUX
            setupIndicator();
#else
            setupTray();
#endif

            checkSettings(true);

            Global::debug("Checking for internet connection...");
            if (!globalParser_->connectedToInternet()){
                //will check every 3 secs if there's internet connection...
                disconnectFromSlot();
                connectToCheckInternet();
                this->start(CHECK_INTERNET_INTERVAL);
            }
            else
            {
#ifdef ON_LINUX
                if(gv.currentDE==UnityGnome){
                    setupUnityShortcuts();
                    setUnityShortcutsState(true, false, false, false);
                }
#endif
                continueWithPotd(false);
            }
            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--website")){
            if(argc>2){
                Global::error("Argument --website doesn't take any other options!");
                showUsage(1);
            }
            if(alreadyRuns()){
                messageServer("--website", true);
                startStatisticsTimer();
                return app.exec();
            }
            gv.liveWebsiteRunning=startedWithWebsite_=true;
            Global::updateStartup();
            secondsLeft_=0;
            checkSettings(true);
            Global::resetSleepProtection(secondsLeft_);
            connectToServer();
#ifdef ON_LINUX
            setupIndicator();
#else
            setupTray();
#endif
            websiteSnapshot_ = new WebsiteSnapshot();

#ifdef ON_LINUX
            if(gv.currentDE==UnityGnome){
                setupUnityShortcuts();
                setUnityShortcutsState(true, false, false, false);
            }
#endif //#ifdef ON_LINUX
            continueWithWebsite();
            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--quit") || arguments.contains("--stop") || arguments.contains("--next") || arguments.contains("--previous") || arguments.contains("--pause")){
            if(argc>2){
                showUsage(1);
            }

            if(alreadyRuns()){
                QString message;
                QStringList all = QStringList() << "--quit" << "--stop" << "--next" << "--previous" << "--pause";
                int count=all.count();
                for(int i=0;i<count;i++){
                    if(arguments.contains(all.at(i))){
                        message=all.at(i);
                        break;
                    }
                }
                messageServer(message, true);
            }
            else
            {
                Global::error("No instance seems to be running.");
                exit(1);
            }
        }
        else if(arguments.contains("--none")){
            if(arguments.indexOf("--none") != 1 || argc!=2){
                showUsage(1);
            }
            if(alreadyRuns()){
                Global::error("Wallch seems to already run! Exiting this instance!");
                return 0;
            }
            startedWithNone_=true;
            connectToServer();
#ifdef ON_LINUX
            setupIndicator();
#else
            setupTray();
#endif
            startStatisticsTimer();
            return app.exec();
        }
        else if(arguments.contains("--cache")){
            if(arguments.indexOf("--cache") != 1 || argc!=3){
                showUsage(1);
            }
            QString parent_folder=arguments.at(2);
            if(!QDir(parent_folder).exists()){
                Global::error(parent_folder+" - No such file or directory.");
                return 1;
            }
            if(!QDir(gv.cachePath).exists()){
                QDir().mkpath(gv.cachePath);
            }
            parent_folder=QDir(parent_folder).absolutePath();
            gv.currentImageCacheSize=Global(false).getCacheStatus().size;
            QDir directoryContainingPictures;
            directoryContainingPictures.setFilter(QDir::Files);
            QStringList allPictures;
            Global::debug("Initializing...");
            Q_FOREACH(QString currentFolder, Global::listFolders(parent_folder, true, true)){
                directoryContainingPictures.setPath(currentFolder);
                Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
                    allPictures << currentFolder+"/"+pic;
                }
            }
            int allPicturesCount=allPictures.count();
            QString original_image_path;
            QStringList bad_files;
            std::string to_output;
            for(int i=0;i<allPicturesCount;i++){
                clearCurrentLine(to_output.length());
                original_image_path=allPictures.at(i);
                to_output='\r'+percentToProgressbar(short((((float)(i+1)/allPicturesCount)) * 100))+" - "+Global::basenameOf(original_image_path).toLocal8Bit().data();
                std::cout << to_output << std::flush;
                QString cached_image_path=Global::originalToCacheName(original_image_path);
                if(QFile(gv.cachePath+cached_image_path).exists()){
                    continue; //already cached
                }
                QImage img(original_image_path);
                if(!img.isNull() && img.scaled(CACHED_IMAGES_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(gv.cachePath+cached_image_path)){
                    Global::addToCacheSize(QFile(gv.cachePath+cached_image_path).size());
                }
                else
                {
                    bad_files << original_image_path;
                }
            }
            clearCurrentLine(to_output.length());
            to_output='\r'+percentToProgressbar(100);
            std::cout << to_output << std::flush;
            std::cout << std::endl;
            if(Global::addToCacheSize(0)){
                //just to check if the cache size is way too large now
                Global::debug("The cache size is larger than it should be: current size (bytes): "
                              +QString::number(gv.currentImageCacheSize)+
                              ", max size (bytes): "
                              +QString::number(MAX_IMAGE_CACHE*BYTES_PER_MiB)+". "+
                              "Fixing the size by deleting some images... (use --fcache for skipping this)"
                              );
                Global(false).fixCacheSize(getCurrentWallpapersFolder());
            }
            if(bad_files.count()){
                Global::error("Please mention that I could not create cache for the following file(s): \n"+bad_files.join("\n"));
            }
            Global::debug("Done!");
            return 0;
        }
        else if(arguments.contains("--fcache")){
            if(arguments.indexOf("--fcache") != 1 || argc!=3){
                showUsage(1);
            }
            QString parent_folder=arguments.at(2);
            if(!QDir(parent_folder).exists()){
                Global::error(parent_folder+" - No such file or directory.");
                return 1;
            }
            if(!QDir(gv.cachePath).exists()){
                QDir().mkpath(gv.cachePath);
            }
            parent_folder=QDir(parent_folder).absolutePath();
            QDir directoryContainingPictures;
            directoryContainingPictures.setFilter(QDir::Files);
            QStringList allPictures;
            Global::debug("Initializing...");
            Q_FOREACH(QString currentFolder, Global::listFolders(parent_folder, true, true)){
                directoryContainingPictures.setPath(currentFolder);
                Q_FOREACH(QString pic, directoryContainingPictures.entryList(IMAGE_FILTERS)){
                    allPictures << currentFolder+"/"+pic;
                }
            }
            int allPicturesCount=allPictures.count();
            QString originalImagePath;
            QStringList bad_files;
            std::string to_output;
            for(int i=0;i<allPicturesCount;i++){
                clearCurrentLine(to_output.length());
                originalImagePath=allPictures.at(i);
                to_output='\r'+percentToProgressbar(short((((float)(i+1)/allPicturesCount)) * 100))+" - "+Global::basenameOf(originalImagePath).toLocal8Bit().data();
                std::cout << to_output << std::flush;
                QString cached_image_path=Global::originalToCacheName(originalImagePath);
                if(QFile(gv.cachePath+cached_image_path).exists()){
                    continue; //already cached
                }
                QImage img(originalImagePath);
                if(img.isNull() || !img.scaled(CACHED_IMAGES_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(gv.cachePath+cached_image_path)){
                    bad_files << originalImagePath;
                }
            }
            clearCurrentLine(to_output.length());
            to_output='\r'+percentToProgressbar(100);
            std::cout << to_output << std::flush;
            std::cout << std::endl;
            if(bad_files.count()){
                Global::error("Please mention that I could not create cache for the following file(s): \n"+bad_files.join("\n"));
            }
            Global::debug("Done!");
            return 0;
        }
        else
        {
            //there were arguments, but not for starting wallch hidden... continue...
            if(QDir(QDir().absoluteFilePath(arguments.at(1))).exists()){
                //there's a folder as the first argument
                if(argc > 2){
                    Global::error("There is more than a folder in the arguments. Wallch does not know what to do with the extra stuff. See -h/--help for more info");
                    return 1;
                }
                else
                {
                    QString currentFolder = QDir::cleanPath(QDir().absoluteFilePath(arguments.at(1)));
                    if(alreadyRuns()){
                        Global::debug("Folder '"+currentFolder+"' has been sent to the already running instance of Wallch for monitoring.");
                        messageServer("MONITOR:"+currentFolder, true);
                        startStatisticsTimer();
                        return app.exec();
                    }
                    else
                    {
                        //just set the folder as the default folder for monitoring
                        setDefaultFolderInSettings(currentFolder);
                        Global::debug("Folder '"+currentFolder+"' is now the default folder for Wallch monitoring.");
                        globalParser_->desktopNotify(tr("The default folder has changed."), false, "info");
                        return 0;
                    }
                }
            }
            else
            {
                //wcz files to be sent to the already running instance of Wallch.
                short returnResult=1;
                bool runsAlready=alreadyRuns(), settingsChecked=false;
                for(short i=1; i<argc; i++){
                    if(arguments.at(i).endsWith(".wcz")){
                        if(runsAlready){
                            QString currentClock = QDir::cleanPath(QDir().absoluteFilePath(arguments.at(i)));
                            Global::debug("The wallpaper clock '"+currentClock+"'' has been sent to the already running instance of Wallch for installation.");
                            messageServer("CLOCK:"+currentClock, false);
                            returnResult=0;
                        }
                        else
                        {
                            Global::error("Installation of wallpaper clocks is only allowed on the GUI interface. Please open Wallch in GUI mode and try again.");
                            break;
                        }
                    }
                    else
                    {
                        Global::debug("Attempting to set as background '"+QDir::cleanPath(QDir().absoluteFilePath(arguments.at(i)))+"'.");
                        if(!settingsChecked){
                            checkSettings(false);
                            settingsChecked=true;
                        }
                        WallpaperManager::setBackground(QDir::cleanPath(QDir().absoluteFilePath(arguments.at(i))), true, true, 1);
                    }
                }
                return returnResult;
            }
        }
    }
    else
    {
        //we have no arguments, proceed
        QApplication app(argc, argv);
        globalParser_ = new Global(true);
        viralSettingsOperations();
        QApplication::setQuitOnLastWindowClosed(false);
        if(alreadyRuns()){
            messageServer("--focus", true);
            startStatisticsTimer();
            return app.exec();
        }

        connectToServer();

#ifdef ON_LINUX
        setupIndicator();
        if(gv.currentDE==UnityGnome){
            setupUnityShortcuts();
        }
#else
        setupTray();
#endif
        mainWindowLaunched_=true;
        MainWindow w(globalParser_, websiteSnapshot_, wallpaperManager_, 0, 0);
        connectMainwindowWithExternalActions(&w);
        w.show();
        startStatisticsTimer();
        return app.exec();
    }
    return 0;
}
