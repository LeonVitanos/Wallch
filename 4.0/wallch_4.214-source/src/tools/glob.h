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

#ifndef GLOB_H
#define GLOB_H

#include <QNetworkReply>
#include <QDir>
#include <QTextStream>
#include <QTimer>
#include <QStringList>
#include <QDateTime>
#include <QRect>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QSettings>

#include "time.h"

#ifdef Q_OS_UNIX
#include "unity/unity/unity.h"
#endif //#ifdef Q_OS_UNIX

#define HELP_URL "http://melloristudio.com/wallch/help"
#define DONATE_URL "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Z34FXUH6M4G9S"

#ifdef Q_OS_UNIX
#define APP_DESKTOP_NAME "wallch-nautilus.desktop"
#endif //#ifdef Q_OS_UNIX

#define AMBIANCE_SEPARATOR QPixmap(":/themes/ambiance_separator.png")
#define RADIANCE_SEPARATOR QPixmap(":/themes/radiance_seperator.png")

#define POTD_ONLINE_URL "https://dl.dropboxusercontent.com/u/257493884/potd"
#define LIVEARTH_ONLINE_URL "https://dl.dropboxusercontent.com/u/257493884/le"

#define POTD_ONLINE_URL_B "http://melloristudio.com/wallch/potd"
#define LIVEARTH_ONLINE_URL_B "http://melloristudio.com/wallch/le"

#define LE_IMAGE "liveEarth"
#define POTD_IMAGE "potd"
#define WC_IMAGE "wallpaperClock"
#define LW_IMAGE "liveWebsite"

#define LW_PREVIEW_IMAGE "liveWebsitePreview.png"
#define POTD_PREVIEW_IMAGE "previewPotd.jpg"
#define LE_POINT_IMAGE "lePoint.jpg"

#define NULL_IMAGE "empty.png"

#define HISTORY_SETTINGS "wallch", "History"

#define DEFAULT_SLIDER_DELAY 900
#define WEBSITE_TIMEOUT 90
#define RESEARCH_FOLDERS_TIMEOUT 1500

#define LEAST_WALLPAPERS_FOR_START 2

#define LIVEARTH_INTERVAL 1800

#define INTERVAL_INDEPENDENCE_DEFAULT_VALUE "p-1.0000:00:00:00:00:00"

#define IMAGE_FILTERS QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.gif" << "*.GIF" << "*.bmp" << "*.BMP" << "*.svg" << "*.SVG"

#define MENU_POPUP_POS QPoint(QCursor::pos()) + QPoint(2, 0)

#define PREVIOUS_PICTURES_LIMIT 30

#define AUTOSTART_DIR "/.config/autostart/"
#define BOOT_DESKTOP_FILE "wallch.desktop"

#ifdef Q_OS_WIN

#include "notification.h"

#endif

extern QSettings *settings;

#ifdef Q_OS_UNIX

struct DesktopEnvironment {
  enum Value {
    UnityGnome, Gnome, XFCE, LXDE, Mate
  };
};

struct ColoringType {
  enum Value {
    NoneColor, SolidColor, HorizontalColor, VerticalColor
  };
};

#endif

struct GlobalVar {
    QString homePath;
    QString wallchHomePath;
    QString currentTheme;
    QString currentDeDefaultWallpapersPath;
    QString currentOSName;
    bool preferencesDialogShown;
    bool independentIntervalEnabled;
    int typeOfInterval;
    bool randomImagesEnabled;
    bool firstTimeout;
    bool symlinks;
    bool processPaused;
#ifdef Q_OS_UNIX
    bool unityProgressbarEnabled;
    DesktopEnvironment::Value currentDE;
    QStringList unacceptedDesktopValues;
#endif //#ifdef Q_OS_UNIX
    bool saveHistory;
    int randomTimeFrom;
    int randomTimeTo;
    bool doNotToggleRadiobuttonFallback;
    bool previewImagesOnScreen;
    bool pauseOnBattery;
    bool amPmEnabled;
    bool mainwindowLoaded;
    bool setAverageColor;
    bool websiteLoginEnabled;
    bool websiteCropEnabled;
    int wallpapersChangedCurrentSession;
    QDateTime timeLaunched;
    bool showNotification;
    bool wallpaperClocksRunning;
    bool liveWebsiteRunning;
    bool potdRunning;
    bool liveEarthRunning;
    bool wallpapersRunning;
    bool iconMode;
    bool rotateImages;
    bool potdIncludeDescription;
    bool leEnableTag;
    bool potdDescriptionBottom;
    short wallpaperClocksHourImages;
    short refreshhourinterval;
    short websiteWaitAfterFinishSeconds;
    bool websiteLoadImages;
    bool websiteJavaEnabled;
    bool websiteJavascriptCanReadClipboard;
    bool websiteJavascriptEnabled;
    bool websiteSimpleAuthEnabled;
    int websiteInterval;
    int screenHeight;
    int screenWidth;
    int potdDescriptionLeftMargin;
    int potdDescriptionRightMargin;
    int potdDescriptionBottomTopMargin;
    QString cachePath;
    QDateTime appStartTime;
    QString websiteWebpageToLoad;
    QString defaultPicturesLocation;
    QString potdDescriptionFont;
    QString potdDescriptionColor;
    QString potdDescriptionBackgroundColor;
    QList<int> defaultIntervals;

    QStringList websiteExtraUsernames;
    QStringList websiteExtraPasswords;
    QString defaultWallpaperClock;
    QString nextShortcut;
    QString potdOnlineUrl;
    QString potdOnlineUrlB;
    QString liveEarthOnlineUrl;
    QString liveEarthOnlineUrlB;
    QString onlineLinkForHistory;
    QRect websiteCropArea;
    QString websiteLoginUsername;
    QString websiteLoginPasswd;
    QString websiteFinalPageToLoad;
    bool websiteRedirect;
    QDateTime runningTimeOfProcess;
    QDateTime timeToFinishProcessInterval;   

#ifdef Q_OS_UNIX
    //unity launcher shortcuts
    UnityLauncherEntry *unityLauncherEntry;
    DbusmenuMenuitem *unityStopAction, *unityNextAction, *unityPreviousAction, *unityPauseAction;
#endif //#ifdef Q_OS_UNIX

    //variable initialization

    GlobalVar() : homePath(QDir::homePath()), currentTheme("ambiance"), preferencesDialogShown(false), independentIntervalEnabled(true),
        typeOfInterval(0), randomImagesEnabled(false), firstTimeout(false), symlinks(false), processPaused(false),
#ifdef Q_OS_UNIX
        unityProgressbarEnabled(false), currentDE(DesktopEnvironment::UnityGnome), unacceptedDesktopValues(QStringList() << "" << "default.desktop" << "X-Cinnamon" << "default"),
#endif
        saveHistory(true), randomTimeFrom(300), randomTimeTo(1200), doNotToggleRadiobuttonFallback(false), previewImagesOnScreen(true), pauseOnBattery(false), amPmEnabled(false),
        mainwindowLoaded(false), setAverageColor(false), websiteLoginEnabled(false), websiteCropEnabled(false), wallpapersChangedCurrentSession(0), timeLaunched(QDateTime::currentDateTime()),
        showNotification(false), wallpaperClocksRunning(false), liveWebsiteRunning(false), potdRunning(false), liveEarthRunning(false), wallpapersRunning(false),
        iconMode(true), rotateImages(false), potdIncludeDescription(true), leEnableTag(false), potdDescriptionBottom(true), wallpaperClocksHourImages(0), refreshhourinterval(0), websiteWaitAfterFinishSeconds(3),
        websiteLoadImages(true), websiteJavaEnabled(false), websiteJavascriptCanReadClipboard(false), websiteJavascriptEnabled(true), websiteSimpleAuthEnabled(false),
        websiteInterval(6), screenHeight(0), screenWidth(0), potdDescriptionLeftMargin(100), potdDescriptionRightMargin(0), potdDescriptionBottomTopMargin(0), appStartTime(QDateTime::currentDateTime()),
        websiteWebpageToLoad("http://google.com"), defaultPicturesLocation(homePath+"/"+QStandardPaths::displayName(QStandardPaths::PicturesLocation)), potdDescriptionFont("Ubuntu"),
        defaultIntervals(QList<int>() << 10 << 30 << 60 << 180 << 300 << 600 << 900 << 1200 << 1800 << 2700 << 3600 << 7200 << 10800 << 14400 << 21600 << 43200 << 86400)
        {}
};

extern struct GlobalVar gv;

class Global: public QObject
{
    Q_OBJECT

public:
    Global();
    ~Global();
    static void saveHistory(const QString &image, short feature);
    static void rotateImg(const QString &filename, short rotation_type, bool show_messagebox);
    static void readClockIni();
    void desktopNotify(const QString text, bool checkImage, const QString &image);
    QString wallpaperClockNow(const QString &path, bool minuteCheck, bool hourCheck, bool amPmCheck, bool dayWeekCheck, bool dayMonthCheck, bool monthCheck);
    static QString setAverageColor(const QString &image);
    static QStringList listFolders(const QString &parentFolder, bool recursively, bool includeParent);
    static void generateRandomImages(int imagesNumber, int firstOne);
    static QString basenameOf(const QString &path);
    static QString dirnameOf(const QString &path);
    static QString suffixOf(const QString &path);
    static QString timeNowToString();
    static QString secondsToMinutesHoursDays(int seconds);
    static int websiteSliderValueToSeconds(short value);
    static bool remove(const QString &files);
    static QString getFilename(const QString &file);
    static bool foldersAreSame(QString folder1, QString folder2);
    static void saveSecondsLeftNow(int secondsLeft, short forType);
    static QString base64Decode(const QString &string);
    static void rotateImageBasedOnExif(const QString &image);
    static short getExifRotation(const QString &filename);
    static void resetSleepProtection(int timeoutCount);
    static void addPreviousBackground(QStringList &previous_backgrounds, const QString &image);
    static void error(const QString &message);
    static void debug(const QString &message);
    static QString getPrimaryColor();
#ifdef Q_OS_UNIX
    static void setUnityProgressBarEnabled(bool state);
    static void setUnityProgressbarValue(float percent);
    static QString gsettingsGet(const QString &schema, const QString &key);
    static QString getSecondaryColor();
    static void setPrimaryColor(const QString &colorName);
    static void setSecondaryColor(const QString &colorName);
    static ColoringType::Value getColoringType();
    static QString getPcManFmValue(const QString &key);
    static void gsettingsSet(const QString &schema, const QString &key, const QString &value);
    static void changeIndicatorIcon(const QString &icon);
    static void changeIndicatorSelection(const QString &status);
    static void showWallpapersIndicatorControls(bool show, bool pauseText);
#endif //#ifdef Q_OS_UNIX
    static int getSecondsTillHour(const QString &hour);
    static QString getOutputOfCommand(QString command, QStringList parameters);
    static void openUrl(const QString &url);
    static QString monthInEnglish(short month);
    static void updateStartup();
    static bool createDesktopFile(const QString &path, const QString &command, const QString &comment);
    static bool isSubfolder(QString &subfolder, QString &parentFolder);
    static QPixmap roundedCorners(const QImage &image, const int radius);
    static short autodetectTheme();
    bool runsOnBattery();
    static void currentBackgroundExists();

private:
#ifdef Q_OS_WIN
    Notification *notification_ = NULL;
#endif
    static QString searchForFileInDir(QString folder, QString file);
    static void setPcManFmValue(const QString &key, const QString &value);

    #ifdef Q_OS_WIN
private Q_SLOTS:
    void notificationDestroyed();
#endif

Q_SIGNALS:
    void updateNotification(QString message, QString image);
};
#endif // GLOB_H
