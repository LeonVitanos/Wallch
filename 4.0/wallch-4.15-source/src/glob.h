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

#ifdef ON_LINUX
#include <libappindicator/app-indicator.h>
#include "unity/unity/unity.h"
#endif //#ifdef ON_LINUX

//editing this is enough for everything else (like --version or About dialog)
#define APP_VERSION 4.15

#define HELP_URL "http://melloristudio.com/wallch/help"
#define DONATE_URL "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Z34FXUH6M4G9S"

#ifdef ON_LINUX
#define APP_DESKTOP_NAME "wallch-nautilus.desktop"
#endif //#ifdef ON_LINUX

#define AMBIANCE_THEME_STYLESHEET "QPushButton:checked:!hover {color: white;background-image: url(:/icons/Pictures/ambiance_checked.png);font: 10pt \"Ubuntu\";border: 0px;}QPushButton:checked:hover {color: white;background-image: url(:/icons/Pictures/ambiance_checked_and_hovered.png);font: 10pt \"Ubuntu\";border: 0px; }QPushButton {color: white;background-image: url(:/icons/Pictures/ambiance_not_checked.png);font: 10pt \"Ubuntu\";border: 0px;}"
#define RADIANCE_THEME_STYLESHEET "QPushButton:checked:!hover {color: #4c4c4c;background-image: url(:/icons/Pictures/radiance_checked.png);font: 10pt \"Ubuntu\";border: 0px;}QPushButton:checked:hover {color: #4c4c4c;background-image: url(:/icons/Pictures/radiance_checked_and_hovered.png);font: 10pt \"Ubuntu\";border: 0px; }QPushButton {color: #4c4c4c;background-image: url(:/icons/Pictures/radiance_not_checked.png);font: 10pt \"Ubuntu\";border: 0px;}"
#define AMBIANCE_SEPARATOR QPixmap(":/icons/Pictures/ambiance_separator.png")
#define RADIANCE_SEPARATOR QPixmap(":/icons/Pictures/radiance_seperator.png")

#ifdef ON_LINUX

#define INDICATOR_AMBIANCE_NORMAL QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_normal.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_LEFT QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_left.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_RIGHT QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_right.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_PAUSE QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_pause.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_EARTH QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_earth.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_POTD QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_potd.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_CLOCKS QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_clocks.png")).toLocal8Bit().data()
#define INDICATOR_AMBIANCE_WEBSITE QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_ambiance_website.png")).toLocal8Bit().data()

#define INDICATOR_RADIANCE_NORMAL QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_normal.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_LEFT QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_left.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_RIGHT QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_right.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_PAUSE QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_pause.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_EARTH QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_earth.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_POTD QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_potd.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_CLOCKS QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_clocks.png")).toLocal8Bit().data()
#define INDICATOR_RADIANCE_WEBSITE QString(QString::fromStdString(PREFIX)+QString("/share/wallch/files/indicator_radiance_website.png")).toLocal8Bit().data()

#define INDICATOR_SCROLL_INTERVAL 400

#define INDICATOR_DESCRIPTION "Wallch Indicator"

#endif //#ifdef ON_LINUX

#define POTD_ONLINE_URL "https://dl.dropboxusercontent.com/u/257493884/potd"
#define LIVEARTH_ONLINE_URL "http://dl.dropboxusercontent.com/u/257493884/le"

#define POTD_ONLINE_URL_B "http://melloristudio.com/wallch/potd"
#define LIVEARTH_ONLINE_URL_B "http://melloristudio.com/wallch/le"

#define LE_IMAGE "liveEarth"
#define POTD_IMAGE "potd"
#define WC_IMAGE "wallpaperClock"
#define LW_IMAGE "liveWebsite"

#define LW_PREVIEW_IMAGE "liveWebsitePreview.png"
#define POTD_PREVIEW_IMAGE "previewPotd.jpg"

#define NULL_IMAGE "empty.png"

#define HISTORY_SETTINGS "wallch", "History"

#define DEFAULT_SLIDER_DELAY 900
#define WEBSITE_TIMEOUT 90
#define RESEARCH_FOLDERS_TIMEOUT 1500

#define LEAST_WALLPAPERS_FOR_START 2

#define LIVEARTH_INTERVAL 1800

#define INTERVAL_INDEPENDENCE_DEFAULT_VALUE "p-1.0000:00:00:00:00:00"

#define IMAGE_FILTERS QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.gif" << "*.GIF" << "*.bmp" << "*.BMP" << "*.svg" << "*.SVG"

#define DEFAULT_INTERVALS_IN_SECONDS QList<int>() << 10 << 30 << 60 << 180 << 300 << 600 << 900 << 1200 << 1800 << 2700 << 3600 << 7200 << 10800 << 14400 << 21600 << 43200 << 86400

#define MENU_POPUP_POS QPoint(QCursor::pos()) + QPoint(2, 0)

#define PREVIOUS_PICTURES_LIMIT 30

#define BYTES_PER_MiB 1048576.0
#define BYTES_PER_KiB 1024.0

#define MAX_IMAGE_CACHE 12 // Should be about ~5000 images
#define CACHE_IMAGES_PER_MiB 420 //average, tested using 1034 HD images.
#define CACHED_IMAGES_SIZE 80, 80

#define AUTOSTART_DIR "/.config/autostart/"
#define BOOT_DESKTOP_FILE "wallch.desktop"

#ifdef ON_WIN32

#include "notification.h"

#endif

extern QSettings *settings;

#ifdef ON_LINUX

typedef enum {
    //please note that the row of these must match the row of the corresponding combobox in Preferences Dialog.
    UnityGnome, Gnome, XFCE, LXDE, Mate
} DesktopEnvironment;

typedef enum {
    NoneColor, SolidColor, HorizontalColor, VerticalColor
} ColoringType;

#endif

struct GlobalVar {
    QString homePath;
    QString wallchHomePath;
    QString currentTheme;
    QString currentDeDefaultWallpapersPath;
    QString currentOSName;
    bool preferencesDialogShown;
    bool independentIntervalEnabled;
    bool useShortcutNext;
    bool randomTimeEnabled;
    bool randomImagesEnabled;
    bool firstTimeout;
    bool processPaused;
#ifdef ON_LINUX
    bool unityProgressbarEnabled;
    DesktopEnvironment currentDE;
    QStringList unacceptedDesktopValues;
#endif //#ifdef ON_LINUX
    bool saveHistory;
    int randomTimeFrom;
    int randomTimeTo;
    bool doNotToggleRadiobuttonFallback;
    bool previewImagesOnScreen;
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
    int currentImageCacheSize;
    QString websiteWebpageToLoad;
    QString defaultPicturesLocation;
    QString potdDescriptionFont;
    QString potdDescriptionColor;
    QString potdDescriptionBackgroundColor;
    QStringList defaultIntervals;

    QStringList websiteExtraUsernames;
    QStringList websiteExtraPasswords;
    QString defaultWallpaperClock;
    QString nextShortcut;
    QString potdOnlineUrl;
    QString potdOnlineUrlB;
    QString liveEarthOnlineUrl;
    QString liveEarthOnlineUrlB;
    QString onlineLinkForHistory;
    QList<int> customIntervalsInSeconds;
    QRect websiteCropArea;
    QString websiteLoginUsername;
    QString websiteLoginPasswd;
    QString websiteFinalPageToLoad;
    bool websiteRedirect;
    QDateTime runningTimeOfProcess;
    QDateTime timeToFinishProcessInterval;   

#ifdef ON_LINUX
    //unity launcher shortcuts
    UnityLauncherEntry *unityLauncherEntry;
    DbusmenuMenuitem *unityStopAction, *unityNextAction, *unityPreviousAction, *unityPauseAction;
    //indicator
    AppIndicator *applicationIndicator;
    GtkWidget *wallpapersRadiobutton, *liveEarthRadiobutton, *potdRadiobutton, *wallpaperClocksRadiobutton,
                    *liveWebsiteRadiobutton, *iAmNotThere, *wallpapersJustChange, *wallpapersPlayPause, *wallpapersNext, *wallpapersPrevious;
#endif //#ifdef ON_LINUX

    //variable initialization

    GlobalVar() : homePath(QDir::homePath()), currentTheme("ambiance"), preferencesDialogShown(false), independentIntervalEnabled(true),
        useShortcutNext(false), randomTimeEnabled(false), randomImagesEnabled(false), firstTimeout(false), processPaused(false),
#ifdef ON_LINUX
        unityProgressbarEnabled(false), currentDE(UnityGnome), unacceptedDesktopValues(QStringList() << "" << "default.desktop" << "X-Cinnamon" << "default"),
#endif
        saveHistory(true), randomTimeFrom(300), randomTimeTo(1200), doNotToggleRadiobuttonFallback(false), previewImagesOnScreen(true), amPmEnabled(false),
        mainwindowLoaded(false), setAverageColor(false), websiteLoginEnabled(false), websiteCropEnabled(false), wallpapersChangedCurrentSession(0), timeLaunched(QDateTime::currentDateTime()),
        showNotification(false), wallpaperClocksRunning(false), liveWebsiteRunning(false), potdRunning(false), liveEarthRunning(false), wallpapersRunning(false),
        iconMode(true), rotateImages(false), potdIncludeDescription(true), potdDescriptionBottom(true), wallpaperClocksHourImages(0), refreshhourinterval(0), websiteWaitAfterFinishSeconds(3),
        websiteLoadImages(true), websiteJavaEnabled(false), websiteJavascriptCanReadClipboard(false), websiteJavascriptEnabled(true), websiteSimpleAuthEnabled(false),
        websiteInterval(6), screenHeight(0), screenWidth(0), potdDescriptionLeftMargin(100), potdDescriptionRightMargin(0), potdDescriptionBottomTopMargin(0), appStartTime(QDateTime::currentDateTime()), currentImageCacheSize(0),
        websiteWebpageToLoad("http://google.com"), defaultPicturesLocation(QStandardPaths::displayName(QStandardPaths::PicturesLocation)), potdDescriptionFont("Ubuntu"),
        defaultIntervals(QStringList() << "10 "+QObject::tr("seconds") << "30 "+QObject::tr("seconds") << "1 "+QObject::tr("minute") << "3 "+QObject::tr("minutes") << "5 "+QObject::tr("minutes") << "10 "+QObject::tr("minutes") << "15 "+QObject::tr("minutes") << "20 "+QObject::tr("minutes") << "30 "+QObject::tr("minutes") << "45 "+QObject::tr("minutes") << "1 "+QObject::tr("hour") << "2 "+QObject::tr("hours") << "3 "+QObject::tr("hours") << "4 "+QObject::tr("hours") << "6 "+QObject::tr("hours") << "12 "+QObject::tr("hours") << "1 "+QObject::tr("day"))
        {}
};

extern struct GlobalVar gv;

typedef struct
{
    QStringList images;
    int imageCount;
    QList<int> imagesSizeList;
    int size;
} CacheEntry;

class Global: public QObject
{
    Q_OBJECT

public:
    Global(bool internetOperation);
    ~Global();
    static void saveHistory(const QString &image, short feature);
    static void rotateImg(const QString &filename, short rotation_type, bool show_messagebox);
    static void readClockIni(const QString &path);
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
#ifdef ON_LINUX
    static void setUnityProgressBarEnabled(bool state);
    static void setUnityProgressbarValue(float percent);
    static QString gsettingsGet(const QString &schema, const QString &key);
    static QString getPrimaryColor();
    static QString getSecondaryColor();
    static void setPrimaryColor(const QString &colorName);
    static void setSecondaryColor(const QString &colorName);
    static ColoringType getColoringType();
    static QString getPcManFmValue(const QString &key);
    static void gsettingsSet(const QString &schema, const QString &key, const QString &value);
    static void changeIndicatorIcon(const QString &icon);
    static void changeIndicatorSelection(const QString &status);
    static void showWallpapersIndicatorControls(bool show, bool pauseText);
#endif //#ifdef ON_LINUX
    static int getSecondsTillHour(const QString &hour);
    static bool addToCacheSize  (qint64 size);
    static QString originalToCacheName(QString image_path);
    static QString getOutputOfCommand(QString command, QStringList parameters);
    static void openUrl(const QString &url);
    static QString monthInEnglish(short month);
    static void updateStartup();
    static bool createDesktopFile(const QString &path, const QString &command, const QString &comment);
    void livearth();
    void potd();
    CacheEntry getCacheStatus();
    bool connectedToInternet();
    void fixCacheSize(QString curWallpapersFolder);
    void abortDownload();

private:
#ifdef ON_WIN32
    Notification *notification_;
#endif
    QNetworkAccessManager *fileDownloader_;
    QNetworkReply *currentNetworkRequest_;
    bool internetOperation_;
    bool internetChecked_;
    bool connectedToInternet_;
    bool alreadyTriedAlternativeLinkToDropbox_;
    QDate potdDescriptionDate_;
    QString potdDescriptionFilename_;
    void downloadTextFileContainingImage(const QString &url);
    void disconnectFileDownloader();
    QString cacheToOriginalName(QString cacheName);
    qint64 cacheGetSizeOfCachedName(const QString &cacheName);
    bool isSubfolder(QString &subfolder, QString &parentFolder);
    QString cacheExcludeSize(const QString &cacheName);
    void downloadOnlineImage(const QString &onlineLink);
    void tryDownloadingImagesFromAlternativeLink();
    void downloadPotdDescription();
    void replaceSpecialHtml(QString &html);
    void writePotdDescription(const QString &description);
    void onlineRequestComplete(const QString &filename);
    static QString searchForFileInDir(QString folder, QString file);
    static void setPcManFmValue(const QString &key, const QString &value);

private Q_SLOTS:
    void readFileContainingImage(QNetworkReply *reply);
    void saveImage(QNetworkReply *reply);
    void internetConnectionReplyFinished(QNetworkReply* reply);
    void getPotdDescription(QNetworkReply* reply);
#ifdef ON_WIN32
    void notificationDestroyed();
#endif

Q_SIGNALS:
    void onlineRequestFailed();
    void onlineImageRequestReady(QString image);
    void updateNotification(QString message, QString image);
    void averageColorChanged();
};
#endif // GLOB_H
