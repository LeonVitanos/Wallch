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
#include <QClipboard>

#include "math.h"

#define HELP_URL "http://melloristudio.com/wallch/help"
#define DONATE_URL "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Z34FXUH6M4G9S"

#ifdef Q_OS_LINUX
    #define APP_DESKTOP_NAME "wallch-nautilus.desktop"
#endif

#define AMBIANCE_SEPARATOR QPixmap(":/themes/ambiance_separator.png")
#define RADIANCE_SEPARATOR QPixmap(":/themes/radiance_seperator.png")

#define POTD_ONLINE_URL "https://dl.dropboxusercontent.com/u/257493884/potd"
#define LIVEARTH_ONLINE_URL "https://dl.dropboxusercontent.com/u/257493884/le"

#define POTD_ONLINE_URL_B "http://melloristudio.com/wallch/potd"
#define LIVEARTH_ONLINE_URL_B "http://melloristudio.com/wallch/le"

#define COLOR_IMAGE "color.png"
#define LE_IMAGE "liveEarth"
#define POTD_IMAGE "potd"
#define LW_IMAGE "liveWebsite"

#define LW_PREVIEW_IMAGE "liveWebsitePreview.png"
#define POTD_PREVIEW_IMAGE "previewPotd.jpg"
#define LE_POINT_IMAGE "lePoint.jpg"

#define NULL_IMAGE "empty.png"

#define HISTORY_SETTINGS "wallch", "History"

#define DEFAULT_SLIDER_DELAY 900
#define WEBSITE_TIMEOUT 90
#define GENERAL_ANIMATION_DURATION 150

#define LEAST_WALLPAPERS_FOR_START 2

#define LIVEARTH_INTERVAL 1800

#define INTERVAL_INDEPENDENCE_DEFAULT_VALUE "p-1.0000:00:00:00:00:00"

//TODO: Add support for heic images (libheic)
#define IMAGE_FILTERS QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.gif" << "*.GIF" << "*.bmp" << "*.BMP" << "*.svg" << "*.SVG"

#define MENU_POPUP_POS QPoint(QCursor::pos()) + QPoint(2, 0)

#define PREVIOUS_PICTURES_LIMIT 30

#define AUTOSTART_DIR "/.config/autostart/"
#define BOOT_DESKTOP_FILE "wallch.desktop"

typedef enum {
    NoneStyle, Center, Tile, Stretch, Scale, Zoom, Span
} DesktopStyle;
Q_DECLARE_METATYPE(DesktopStyle);

#ifndef Q_OS_LINUX
    #include "notification.h"
#endif

struct GlobalVar {
    // 'Wallpapers' Feature
    bool wallpapersRunning = false;

    // 'Live Earth' Feature
    bool liveEarthRunning = false;
    QString liveEarthOnlineUrl;
    QString liveEarthOnlineUrlB;

    // 'POTD' Feature
    bool potdRunning = false;
    bool potdIncludeDescription = true;
    bool potdDescriptionBottom = true;
    int potdDescriptionLeftMargin = 100;
    int potdDescriptionRightMargin = 0;
    int potdDescriptionBottomTopMargin = 0;
    QString potdDescriptionFont = "Arial";
    QString potdDescriptionColor;
    QString potdDescriptionBackgroundColor;
    QString potdOnlineUrl;
    QString potdOnlineUrlB;

    // 'Website' Feature
    bool liveWebsiteRunning = false;
    short websiteWaitAfterFinishSeconds = 3;
    bool websiteLoadImages = true;
    bool websiteJavaEnabled = false;
    bool websiteJavascriptCanReadClipboard = false;
    bool websiteJavascriptEnabled = true;
    bool websiteSimpleAuthEnabled = false;
    bool websiteRedirect = false;
    bool websiteLoginEnabled = false;
    bool websiteCropEnabled = false;
    int websiteInterval = 6;
    QString websiteWebpageToLoad = "http://google.com";
    QRect websiteCropArea;
    QString websiteLoginUsername;
    QString websiteLoginPasswd;
    QString websiteFinalPageToLoad;

    QString homePath = QDir::homePath();
    QString wallchHomePath;
    QString currentDeDefaultWallpapersPath;
    QString currentOSName;
    bool preferencesDialogShown = false;
    bool independentIntervalEnabled = true;
    int typeOfInterval = 0;
    bool randomImagesEnabled = false;
    bool firstTimeout = false;
    bool symlinks = false;
    bool processPaused = false;
    bool saveHistory = true;
    int randomTimeFrom = 300;
    int randomTimeTo = 1200;
    bool doNotToggleRadiobuttonFallback = false;
    bool previewImagesOnScreen = true;
    bool pauseOnBattery = false;
    bool amPmEnabled = false;
    bool mainwindowLoaded = false;
    bool setAverageColor = false;
    int wallpapersChangedCurrentSession = 0;
    QDateTime timeLaunched = QDateTime::currentDateTime();
    bool showNotification = false;
    bool iconMode = true;
    bool rotateImages = false;
    bool leEnableTag = false;
    short refreshhourinterval = 0;
    int screenHeight = 0;
    int screenWidth = 0;
    int screenAvailableHeight = 0;
    int screenAvailableWidth = 0;
    QString cachePath;
    QDateTime appStartTime = QDateTime::currentDateTime();
    QString defaultPicturesLocation = homePath + "/" + QStandardPaths::displayName(QStandardPaths::PicturesLocation);
    QStringList websiteExtraUsernames;
    QStringList websiteExtraPasswords;
    QString nextShortcut;
    QString onlineLinkForHistory;
    QDateTime runningTimeOfProcess;
    QDateTime timeToFinishProcessInterval;

#ifdef Q_OS_LINUX
    QStringList unacceptedDesktopValues = QStringList() << "" << "default.desktop" << "X-Cinnamon" << "default";
#endif
};

extern struct GlobalVar gv;

class Global: public QObject
{
    Q_OBJECT

public:
    Global();
    ~Global();
    static void saveHistory(const QString &image, short feature);
    void desktopNotify(const QString text, bool checkImage, const QString &image);
    static QString setAverageColor(const QString &image);
    static QStringList listFolders(const QString &parentFolder, bool recursively, bool includeParent);
    static void generateRandomImages(int imagesNumber, int firstOne);
    static QString basenameOf(const QString &path);
    static QString dirnameOf(const QString &path);
    static QString suffixOf(const QString &path);
    static QString timeNowToString();
    static int websiteSliderValueToSeconds(short value);
    static bool remove(const QString &files);
    static QString getFilename(const QString &file);
    static void saveSecondsLeftNow(int secondsLeft, short forType);
    static QString base64Decode(const QString &string);
    static void resetSleepProtection(int timeoutCount);
    static void addPreviousBackground(QStringList &previous_backgrounds, const QString &image);
    static void error(const QString &message);
    static void debug(const QString &message);
#ifdef Q_OS_LINUX
    static void changeIndicatorIcon(const QString &icon);
    static void changeIndicatorSelection(const QString &status);
    static void showWallpapersIndicatorControls(bool show, bool pauseText);
#endif
        static int getSecondsTillHour(const QString &hour);
    static void openUrl(const QString &url);
    static QString monthInEnglish(short month);
    static bool isSubfolder(QString &subfolder, QString &parentFolder);
    static QPixmap roundedCorners(const QImage &image, const int radius);
    bool runsOnBattery();

    // Clipboard management
    static void copyImageToClipboard(const QString &image);
    static void copyTextToClipboard(const QString &text);

#ifndef Q_OS_LINUX
    private:
        Notification *notification_ = NULL;
    private Q_SLOTS:
        void notificationDestroyed();
#else
    static QString searchForFileInDir(QString folder, QString file);
#endif

Q_SIGNALS:
    void updateNotification(QString message, QString image);
};
#endif // GLOB_H
