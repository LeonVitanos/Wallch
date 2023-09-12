#include "settingsmanager.h"

SettingsManager::SettingsManager()
{

}

void SettingsManager::initializeSettings(){
    checkFirstRun();
    loadPaths();
    loadSettings();
}

void SettingsManager::checkFirstRun(){
    if(settings->value("first-run", true).toBool()){
        settings->setValue("first-run", false);
#ifdef Q_OS_LINUX
        if(!QDir(gv.homePath+"/.config/Wallch/").removeRecursively())
            Global::error("I probably could not remove previous Wallch configurations!");
#else
        if(!QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Wallch/").removeRecursively())
            Global::error("I probably could not remove previous Wallch configurations!");

# ifdef Q_OS_WIN
        QSettings settings2("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wallch.exe", QSettings::NativeFormat);
        settings2.setValue("Default", QDir::currentPath()+"/wallch.exe");
        settings2.setValue("Path", QDir::currentPath()+"/Wallch");
        settings2.sync();
# endif
#endif
    }

    settings->setValue("times_launched", settings->value("times_launched", 0).toUInt()+1);
}

void SettingsManager::loadPaths(){
#ifdef Q_OS_LINUX
    gv.wallchHomePath=gv.homePath+"/.wallch/";
    gv.cachePath=gv.homePath+"/.cache/wallch/thumbs/";
#else
    gv.wallchHomePath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/Wallch/";
    gv.cachePath = gv.wallchHomePath+"/.cache/thumbs/";
#endif

    if(!QDir(gv.wallchHomePath).exists())
        QDir().mkpath(gv.wallchHomePath);
    if(!QDir(gv.cachePath).exists())
        QDir().mkpath(gv.cachePath);
}

void SettingsManager::loadSettings(){
    // General
    gv.setAverageColor = settings->value("average_color", false).toBool();
    gv.showNotification=settings->value("notification", false).toBool();
    gv.saveHistory=settings->value("history", true).toBool();
    gv.symlinks=settings->value("symlinks", false).toBool();
    gv.pauseOnBattery = settings->value("pause_on_battery", false).toBool();
    gv.independentIntervalEnabled = settings->value("independent_interval_enabled", true).toBool();
    gv.rotateImages = settings->value("rotation", false).toBool();
    gv.firstTimeout = settings->value("first_timeout", false).toBool();
    gv.previewImagesOnScreen = settings->value("preview_images_on_screen", true).toBool();

    // 'Wallpapers' Feature
    gv.typeOfInterval=settings->value("typeOfIntervals", 0).toInt();
    gv.iconMode = settings->value("icon_style", true).toBool();
    gv.randomImagesEnabled = settings->value("random_images_enabled", true).toBool();

    // 'Live Earth' Feature
    gv.leEnableTag = settings->value("le_enable_tag", false).toBool();
    gv.liveEarthOnlineUrl=settings->value("line_earth_online_url", LIVEARTH_ONLINE_URL).toString();
    gv.liveEarthOnlineUrlB=settings->value("live_earth_online_urlB", LIVEARTH_ONLINE_URL_B).toString();

    // 'POTD' Settings
    gv.potdIncludeDescription = settings->value("potd_include_description", true).toBool();
    gv.potdDescriptionBottom = settings->value("potd_description_bottom", true).toBool();
    gv.potdDescriptionFont = settings->value("potd_description_font", "Arial").toString();
    gv.potdDescriptionColor = settings->value("potd_text_color", "#FFFFFF").toString();
    gv.potdDescriptionBackgroundColor = settings->value("potd_background_color", "#000000").toString();
    gv.potdDescriptionLeftMargin = settings->value("potd_description_left_margin", (0)).toInt();
    gv.potdDescriptionRightMargin = settings->value("potd_description_right_margin", 0).toInt();
    gv.potdDescriptionBottomTopMargin = settings->value("potd_description_bottom_top_margin", 0).toInt();
    gv.potdOnlineUrl=settings->value("potd_online_url", POTD_ONLINE_URL).toString();
    gv.potdOnlineUrlB=settings->value("potd_online_urlB", POTD_ONLINE_URL_B).toString();

    // 'Website' Feature
    gv.websiteWebpageToLoad=settings->value("website", "http://google.com").toString();
    gv.websiteInterval=settings->value("website_interval", 6).toInt();
    gv.websiteCropEnabled=settings->value("website_crop", false).toBool();
    gv.websiteCropArea=settings->value("website_crop_area", QRect(0, 0, gv.screenAvailableWidth, gv.screenAvailableHeight)).toRect();
    gv.websiteLoginEnabled=settings->value("website_login", false).toBool();
    gv.websiteLoginUsername=settings->value("website_username", "").toString();
    gv.websiteLoginPasswd=settings->value("website_password", "").toString();
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
}

void SettingsManager::updateStartup()
{
    if(!settings->value("Startup", true).toBool())
        return;

    QString arguments = gv.wallpapersRunning ? (settings->value("Once", false).toBool() ? "--change" : "--start") :
        gv.liveEarthRunning ? "--earth" :
        gv.potdRunning ? "--potd" :
        gv.liveWebsiteRunning ? "--website" :
        settings->value("start_hidden", false).toBool() ? "--none" : "";

#ifdef Q_OS_WIN
    QSettings windowsSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString wallch_exe_path = (QFile::exists("C:\\Program Files (x86)\\Wallch\\wallch.exe")?"C:\\Program Files (x86)\\Wallch\\wallch.exe":(QFile::exists("C:\\Program Files\\Wallch\\wallch.exe")?"C:\\Program Files\\Wallch\\wallch.exe":QDir::currentPath()+"/wallch.exe"));

    if(arguments.isEmpty())
        windowsSettings.remove("Wallch");
    else
        windowsSettings.setValue("Wallch", wallch_exe_path + " " + arguments);

    windowsSettings.sync();
#else
# ifdef Q_OS_UNIX
    if(!QDir(gv.homePath+AUTOSTART_DIR).exists()){
        if(!QDir().mkpath(gv.homePath+AUTOSTART_DIR)){
            Global::error("Failed to create ~"+QString(AUTOSTART_DIR)+" folder. Please check folder existence and permissions.");
        }
    }
    else
        Global::remove(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE);

    if(!arguments.isEmpty()){
        QString desktopFileComment = gv.wallpapersRunning ? (settings->value("Once", false).toBool() ? "Sets a random picture from the list as background" : "Start Changing Wallpapers") :
                                         gv.liveEarthRunning ? "Enable Live Earth" :
                                         gv.potdRunning ? "Enable Picture of the Day" :
                                         gv.liveWebsiteRunning ? "Enable Live Website" :
                                         settings->value("start_hidden", false).toBool() ? "Start Wallch hidden in tray" : "";

        QString desktopFileCommand = "/usr/bin/wallch " + arguments;

#  ifdef Q_OS_LINUX
        short defaultValue=3;
#  else
        short defaultValue=0;
#  endif

        if(settings->value("startup_timeout", defaultValue).toInt()!=0)
            desktopFileCommand="bash -c 'sleep "+QString::number(settings->value("startup_timeout", defaultValue).toInt())+" && "+desktopFileCommand+"'";

        createDesktopFile(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE, desktopFileCommand, desktopFileComment);
    }
# endif
#endif
}

void SettingsManager::setDefaultFolder(const QString &folder){
    if(!QDir(folder).exists()){
        Global::error("The folder "+folder+" does not exist!");
        return;
    }

    bool alreadyInSettings=false;
    short count = settings->beginReadArray("pictures_locations");

    for(short i=0; i<count; i++)
    {
        settings->setArrayIndex(i);
        if(FileManager::foldersAreSame(folder, settings->value("item").toString()))
        {
            settings->endArray();
            settings->setValue("currentFolder_index", i);
            alreadyInSettings=true;
            break;
        }
    }

    if(!alreadyInSettings){
        //Add the folder to the index and point to it as default
        settings->endArray();
        settings->beginWriteArray("pictures_locations");
        settings->setArrayIndex(count);
        settings->setValue("item", folder);
        settings->endArray();
        settings->setValue("currentFolder_index", count);
    }

    settings->sync();
}
