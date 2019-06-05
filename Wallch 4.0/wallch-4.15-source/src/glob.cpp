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

#include <QProcess>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QDateTime>
#include <QSettings>
#include <QDirIterator>
#include <QPixmap>
#include <QtConcurrent/QtConcurrentRun>
#include <QPainter>
#include <QNetworkAccessManager>
#include <QEvent>
#include <QDesktopServices>
#include <QTextDocument>

#include <iostream>
using namespace std;

#ifdef ON_LINUX

#include <libnotify/notify.h>
#include <gio/gio.h>
#include <libexif/exif-data.h>

#else

#include <windows.h>

#endif

#include "glob.h"
#include "mainwindow.h"

const QChar cacheFromChar='/'; //do NOT change
const QChar cacheToChar='^'; //change, but it CANNOT be the same as cacheAlreadyIncludedDelimiter
const QChar cacheAlreadyIncludedDelimiter='.'; //cannot be the same with the cacheToChar
const QChar cache_size_char='=';

QSettings *settings = new QSettings("wallch", "Settings");

Global::Global(bool internetOperation){
    if(internetOperation){
        fileDownloader_ = new QNetworkAccessManager(this);
        currentNetworkRequest_=NULL;
    }
    internetOperation_=internetOperation;
#ifdef WIN32
    notification_=NULL;
#endif
}

Global::~Global(){
    if(internetOperation_){
        delete fileDownloader_;
    }
}

#ifdef ON_LINUX
void Global::setUnityProgressBarEnabled(bool state){
    if(!gv.unityLauncherEntry){
        gv.unityLauncherEntry = unity_launcher_entry_get_for_desktop_id(APP_DESKTOP_NAME);
    }
    unity_launcher_entry_set_progress_visible(gv.unityLauncherEntry, state);
    setUnityProgressbarValue(0);
}

void Global::setUnityProgressbarValue(float percent){
    if(!gv.unityLauncherEntry){
        gv.unityLauncherEntry = unity_launcher_entry_get_for_desktop_id(APP_DESKTOP_NAME);
    }
    unity_launcher_entry_set_progress(gv.unityLauncherEntry, percent);
}
#endif //#ifdef ON_LINUX

void Global::potd(){
    //searching whether the link for current day is saved at settings
    settings->beginGroup("potd_links");
    potdDescriptionDate_=QDate::currentDate();
    QString curOnlineLink=settings->value(potdDescriptionDate_.toString("dd.MM.yyyy"), "").toString();
    settings->endGroup();
    if(curOnlineLink.isEmpty()){
        alreadyTriedAlternativeLinkToDropbox_=false;
        downloadTextFileContainingImage(gv.potdOnlineUrl);
    }
    else
    {
        Global::debug("The link is already available");
        downloadOnlineImage(curOnlineLink);
    }
}

void Global::tryDownloadingImagesFromAlternativeLink(){
    alreadyTriedAlternativeLinkToDropbox_=true;
    if(gv.potdRunning){
        downloadTextFileContainingImage(gv.potdOnlineUrlB);
    }
    else
    {
        downloadTextFileContainingImage(gv.liveEarthOnlineUrlB);
    }
}

void Global::readFileContainingImage(QNetworkReply *reply){
    currentNetworkRequest_->deleteLater();
    currentNetworkRequest_=NULL;
    if (reply->error()){
        if(!alreadyTriedAlternativeLinkToDropbox_){
            tryDownloadingImagesFromAlternativeLink();
        }
        else
        {
            Q_EMIT onlineRequestFailed();
            Global::error("Could not process your request: "+reply->errorString());
            reply->deleteLater();
        }
        return;
    }
    QString onlineLink(reply->readAll());

    reply->deleteLater();

    if(gv.potdRunning){
        //potd has multiple links in it
        QStringList linksDates=onlineLink.split(QRegExp("[ \n]"),QString::SkipEmptyParts);
        if(linksDates.count()!=6){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT onlineRequestFailed();
            }
            return;
        }

        if(!linksDates.at(1).startsWith("http")){
            //something's wrong!
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT onlineRequestFailed();
            }
            return;
        }
        settings->remove("potd_links");
        settings->beginGroup("potd_links");
        bool linkFound=false;
        QString curDate=QDate::currentDate().toString("dd.MM.yyyy");
        for(short i=0;i<6;i+=2){
            if(!linkFound){
                if(curDate==linksDates.at(i)){
                    linkFound=true;
                    onlineLink=linksDates.at(i+1);
                }
            }
            settings->setValue(linksDates.at(i), linksDates.at(i+1));
        }
        settings->endGroup();
        settings->sync();
        if(!linkFound){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                desktopNotify(tr("Today's Picture Of The Day is not available!"), false, "info");
                Q_EMIT onlineRequestFailed();
            }
            return;
        }
    }
    else if(gv.liveEarthRunning)
    {
        if(!onlineLink.startsWith("http:")){
            if(!alreadyTriedAlternativeLinkToDropbox_){
                tryDownloadingImagesFromAlternativeLink();
            }
            else
            {
                Global::error("Live Earth image download failed!");
                Q_EMIT onlineRequestFailed();
            }
            return;
        }
    }

    onlineLink.replace('\n', "");

    gv.onlineLinkForHistory=onlineLink;

    if(gv.liveEarthRunning){
        Global::remove(gv.wallchHomePath+LE_IMAGE+"*");
    }
    else if(gv.potdRunning){
        QString previous_img_URL=settings->value("previous_img_url", "").toString();
        if(previous_img_URL==onlineLink){
            //image has not yet changed, just skip it for the next day :D
            Global::debug("Wikipedia Picture of the day picture has yet to be updated. Setting the same image...");

            QString filename = Global::getFilename(gv.wallchHomePath+POTD_IMAGE+"*");
            if(!filename.isEmpty()){
                onlineRequestComplete(filename);
                return;
            }
            else
            {
                Global::debug("Redownloading the image...");
            }
        }
    }

    downloadOnlineImage(onlineLink);
}

void Global::downloadOnlineImage(const QString &onlineLink){
    disconnectFileDownloader();
    QObject::connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(saveImage(QNetworkReply*)));
    currentNetworkRequest_=fileDownloader_->get(QNetworkRequest(QUrl(onlineLink)));
}

void Global::disconnectFileDownloader(){
    QObject::disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(getPotdDescription(QNetworkReply*)));
    QObject::disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(saveImage(QNetworkReply*)));
    QObject::disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(readFileContainingImage(QNetworkReply*)));
    QObject::disconnect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(internetConnectionReplyFinished(QNetworkReply*)));
}

void Global::getPotdDescription(QNetworkReply *reply){
    currentNetworkRequest_->deleteLater();
    currentNetworkRequest_=NULL;
    if(reply->error()){
        //set potd without the description
        Global::error("Could not get picture of the day description. Setting image as is.");
        onlineRequestComplete(potdDescriptionFilename_);
        return;
    }
    QString potdDescription(reply->readAll());
    reply->deleteLater();

    if(potdDescription.contains("/wiki/File:"))
    {
        QRegExp filter("<a href=\"/wiki/File:(.+)</a></small>");
        int result = filter.indexIn(potdDescription);
        if(result != -1){
            potdDescription=filter.cap(1);
        }
    }
    else
    {
        Global::error("Could not get picture of the day description. Setting image as is.");
        onlineRequestComplete(potdDescriptionFilename_);
        return;
    }

    if(potdDescription.contains("<p>") && potdDescription.contains("</small>"))
    {
        QRegExp filter2("<p>(.+)</small>");
        int result2 = filter2.indexIn(potdDescription);
        if(result2 != -1){
            potdDescription=filter2.cap(1);
        }
    }
    else
    {
        Global::error("Could not get picture of the day description. Setting image as is.");
        onlineRequestComplete(potdDescriptionFilename_);
        return;
    }
    potdDescription.replace("/wiki", "http://en.wikipedia.org/wiki").remove(QRegExp("<[^>]*>")).replace("\n", " ");
    replaceSpecialHtml(potdDescription);
    QtConcurrent::run(this, &Global::writePotdDescription, potdDescription);
}

void Global::writePotdDescription(const QString &description){
    QImage image(potdDescriptionFilename_);
    QPainter drawer(&image);

    QFont font;
    font.setFamily(gv.potdDescriptionFont);
    font.setPixelSize(image.width()/55);

    drawer.setFont(font);

    QRect drawRect;
    drawRect=drawer.fontMetrics().boundingRect(QRect(0, 0, image.width()-(gv.potdDescriptionLeftMargin+gv.potdDescriptionRightMargin), image.height()), (Qt::TextWordWrap | Qt::AlignCenter), description);
    drawRect.setX(0);
    drawRect.setWidth(image.width());

    int newY;
    if(gv.potdDescriptionBottom){
        newY=image.height()-drawRect.height()-gv.potdDescriptionBottomTopMargin;
    }
    else
    {
        newY=gv.potdDescriptionBottomTopMargin;
    }
    int oldH=drawRect.height();
    drawRect.setY(newY);
    drawRect.setHeight(oldH);

    QColor backgroundColor=QColor(gv.potdDescriptionBackgroundColor);
    backgroundColor.setAlpha(176);

    drawer.fillRect(drawRect, QBrush(backgroundColor));

    drawRect.setX(gv.potdDescriptionLeftMargin);
    drawRect.setWidth(image.width()-(gv.potdDescriptionLeftMargin+gv.potdDescriptionRightMargin));

    drawer.setPen(QColor(gv.potdDescriptionColor));
    drawer.drawText(drawRect, (Qt::TextWordWrap | Qt::AlignCenter), description, &drawRect);

    drawer.end();
    image.save(potdDescriptionFilename_, "JPEG", 100);
    onlineRequestComplete(potdDescriptionFilename_);
}

void Global::onlineRequestComplete(const QString &filename){
    WallpaperManager::setBackground(filename, true, gv.potdRunning, (gv.potdRunning ? 3 : 2));
    if(gv.setAverageColor){
        Q_EMIT averageColorChanged();
    }
    if(gv.mainwindowLoaded){
        Q_EMIT onlineImageRequestReady(filename);
    }
}

void Global::replaceSpecialHtml(QString &html){
    QTextDocument textDocument;
    textDocument.setHtml(html);
    html=textDocument.toPlainText();
}

void Global::downloadTextFileContainingImage(const QString &url){
    //downloads the plain text file that contains the direct link to the image
    disconnectFileDownloader();
    QObject::connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(readFileContainingImage(QNetworkReply*)));
    currentNetworkRequest_=fileDownloader_->get(QNetworkRequest(QUrl(url)));
}

void Global::abortDownload(){
    if(currentNetworkRequest_!=NULL && !currentNetworkRequest_->isFinished()){
        currentNetworkRequest_->abort();
    }
}

void Global::saveImage(QNetworkReply *reply)
{
    currentNetworkRequest_->deleteLater();
    currentNetworkRequest_=NULL;
    if (reply->error()){
        Q_EMIT onlineRequestFailed();
        Global::error("Could not process your request: "+reply->errorString());
        reply->deleteLater();
        return;
    }

    QString filename;
    if(gv.liveEarthRunning){
        Global::remove(gv.wallchHomePath+LE_IMAGE+"*");
        filename=gv.wallchHomePath+LE_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+Global::suffixOf(reply->url().toString());
    }
    else if(gv.potdRunning){
        Global::remove(gv.wallchHomePath+POTD_IMAGE+"*");
        filename=gv.wallchHomePath+POTD_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+Global::suffixOf(reply->url().toString());
    }
    else
    {
        reply->deleteLater();
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)){
        Global::error("Could not save image at "+filename+" !");
        Q_EMIT onlineRequestFailed();
        reply->deleteLater();
        return;
    }
    file.write(reply->readAll());
    file.close();

    if(gv.potdRunning){
        settings->setValue("previous_img_url", reply->url().toString());
        settings->setValue("last_day_potd_was_set", QDateTime::currentDateTime().toString("dd.MM.yyyy"));
        settings->setValue("potd_preferences_have_changed", false);
        settings->sync();
    }

    reply->deleteLater();

    if(gv.potdRunning && gv.potdIncludeDescription){
        potdDescriptionFilename_=filename;
        downloadPotdDescription();
    }
    else
    {
        onlineRequestComplete(filename);
    }
}

void Global::downloadPotdDescription(){
    disconnectFileDownloader();
    connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(getPotdDescription(QNetworkReply*)));
    QString urlToGetDescription;
    if((potdDescriptionDate_.year()==2004 && (potdDescriptionDate_.month()==11 || potdDescriptionDate_.month()==12)) || potdDescriptionDate_.year()==2005 || potdDescriptionDate_.year()==2006){
        urlToGetDescription = "http://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day/"+  Global::monthInEnglish(potdDescriptionDate_.month()) + potdDescriptionDate_.toString("_d,_yyyy");
    }
    else{
        urlToGetDescription = "http://en.wikipedia.org/wiki/Template:POTD/"+potdDescriptionDate_.toString("yyyy-MM-dd");
    }

    currentNetworkRequest_=fileDownloader_->get(QNetworkRequest(QUrl(urlToGetDescription)));
}

QString Global::monthInEnglish(short month)
{
    /*
     * Do NOT replace this function with QDate::longMonthName(),
     * because it is system language independent (but this always
     * returns english month names).
     */
    switch(month){
    case 1: return "January"; break;
    case 2: return "February"; break;
    case 3: return "March"; break;
    case 4: return "April"; break;
    case 5: return "May"; break;
    case 6: return "June"; break;
    case 7: return "July"; break;
    case 8: return "August"; break;
    case 9: return "September"; break;
    case 10: return "October"; break;
    case 11: return "November"; break;
    case 12: return "December"; break;
    default: return "Error"; break;
    }
}

void Global::livearth(){
    alreadyTriedAlternativeLinkToDropbox_=false;
    downloadTextFileContainingImage(gv.liveEarthOnlineUrl);
}

#ifdef ON_LINUX

QString Global::getPrimaryColor(){
    QString primaryColor;
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome){
        primaryColor = Global::gsettingsGet("org.gnome.desktop.background", "primary-color");
    }
    else if(gv.currentDE==Mate){
        primaryColor = Global::gsettingsGet("org.mate.background", "primary-color");
    }
    else if(gv.currentDE==XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color1")){
                QStringList colors=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).split("\n");
                QList<int> rgbColors;
                Q_FOREACH(QString color, colors){
                    bool ok=false;
                    int currentColor=color.toInt(&ok);
                    if(!ok){
                        continue;
                    }
                    rgbColors.append(int((256.0*currentColor)/65535.0));
                }
                if(rgbColors.count()!=4){
                    continue;
                }
                QColor finalColor;
                finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
                primaryColor = finalColor.name();
                break;
            }
        }
    }
    else if(gv.currentDE==LXDE){
        primaryColor=getPcManFmValue("desktop_bg");
    }
    return primaryColor;
}

QString Global::getSecondaryColor(){
    QString secondaryColor;
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome){
        secondaryColor=Global::gsettingsGet("org.gnome.desktop.background", "secondary-color");
    }
    else if(gv.currentDE==Mate){
        secondaryColor=Global::gsettingsGet("org.mate.background", "secondary-color");
    }
    else if(gv.currentDE==XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color2")){
                QStringList colors=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).split("\n");
                QList<int> rgbColors;
                Q_FOREACH(QString color, colors){
                    bool ok=false;
                    int currentColor=color.toInt(&ok);
                    if(!ok){
                        continue;
                    }
                    rgbColors.append(int((256.0*currentColor)/65535.0));
                }
                if(rgbColors.count()!=4){
                    continue;
                }
                QColor finalColor;
                finalColor.setRgb(rgbColors.at(0), rgbColors.at(1), rgbColors.at(2));
                secondaryColor = finalColor.name();
                break;
            }
        }
    }
    return secondaryColor;
}

void Global::setPrimaryColor(const QString &colorName){
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome){
        Global::gsettingsSet("org.gnome.desktop.background", "primary-color", colorName);
    }
    else if(gv.currentDE==Mate)
    {
        Global::gsettingsSet("org.mate.background", "primary-color", colorName);
    }
    else if(gv.currentDE==XFCE){
        QStringList colorValues;
        QColor color=QColor(colorName);
        colorValues.append(QString::number(int((65535.0/256.0)*color.red())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.green())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.blue())));
        colorValues.append("65535");
        Q_FOREACH(QString entry, getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color1")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry
                                        << "-t" << "uint" << "-s" << colorValues.at(0)
                                        << "-t" << "uint" << "-s" << colorValues.at(1)
                                        << "-t" << "uint" << "-s" << colorValues.at(2)
                                        << "-t" << "uint" << "-s" << colorValues.at(3));
            }
        }
    }
    else if(gv.currentDE==LXDE){
        setPcManFmValue("desktop_bg", colorName);
        QStringList pids=getOutputOfCommand("pidof", QStringList() << "pcmanfm").replace("\n", "").split(" ");
        Q_FOREACH(QString pid, pids){
            QString output=getOutputOfCommand("ps", QStringList() << "-fp" << pid);
            if(output.contains("--desktop")){
                QProcess::startDetached("kill", QStringList() << "-9" << pid);
            }
        }
        if(QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists()){
            QProcess::startDetached("pcmanfm", QStringList() << "--desktop" << "-p" << "lubuntu");
        }
        else
        {
            QProcess::startDetached("pcmanfm", QStringList() << "--desktop");
        }
    }
}

void Global::setSecondaryColor(const QString &colorName){
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome){
        Global::gsettingsSet("org.gnome.desktop.background", "secondary-color", colorName);
    }
    else if(gv.currentDE==Mate){
        Global::gsettingsSet("org.mate.background", "secondary-color", colorName);
    }
    else if(gv.currentDE==XFCE){
        QStringList colorValues;
        QColor color=QColor(colorName);
        colorValues.append(QString::number(int((65535.0/256.0)*color.red())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.green())));
        colorValues.append(QString::number(int((65535.0/256.0)*color.blue())));
        colorValues.append("65535");
        Q_FOREACH(QString entry, getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color2")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry
                                        << "-t" << "uint" << "-s" << colorValues.at(0)
                                        << "-t" << "uint" << "-s" << colorValues.at(1)
                                        << "-t" << "uint" << "-s" << colorValues.at(2)
                                        << "-t" << "uint" << "-s" << colorValues.at(3));
            }
        }
    }
}

ColoringType Global::getColoringType(){
    ColoringType coloringType=SolidColor;
    if(gv.currentDE==Gnome || gv.currentDE==UnityGnome){
        QString colorStyle=Global::gsettingsGet("org.gnome.desktop.background", "color-shading-type");
        if(colorStyle.contains("solid")){
            coloringType=SolidColor;
        }
        else if(colorStyle.contains("vertical")){
            coloringType=VerticalColor;
        }
        else if(colorStyle.contains("horizontal")){
            coloringType=HorizontalColor;
        }
    }
    else if(gv.currentDE==Mate){
        QString colorStyle=Global::gsettingsGet("org.mate.background", "color-shading-type");
        if(colorStyle.contains("solid")){
            coloringType=SolidColor;
        }
        else if(colorStyle.contains("vertical")){
            coloringType=VerticalColor;
        }
        else if(colorStyle.contains("horizontal")){
            coloringType=HorizontalColor;
        }
    }
    else if(gv.currentDE==XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("color-style")){
                QString colorStyle=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).replace("\n", "");
                if(colorStyle=="0"){
                    coloringType=SolidColor;
                }
                else if(colorStyle=="1"){
                    coloringType=HorizontalColor;
                }
                else if(colorStyle=="2"){
                    coloringType=VerticalColor;
                }
                else if(colorStyle=="3"){
                    coloringType=NoneColor;
                }
            }
        }
    }
    else if(gv.currentDE==LXDE){
        coloringType=SolidColor;
    }

    return coloringType;
}

QString Global::searchForFileInDir(QString folder, QString file){
    if(!QDir(folder).exists()){
        return QString();
    }

    Q_FOREACH(QString path, Global::listFolders(folder, true, true)){

        if(QDir(path, QString(), QDir::Name, QDir::Files).entryList().contains(file)){
            return path+'/'+file;
        }
    }

    return QString();
}

QString Global::getPcManFmValue(const QString &key){
    QString result;
    if(QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists()){
        QSettings settings("pcmanfm", "lubuntu/pcmanfm");
        settings.beginGroup("desktop");
        result=settings.value(key, "").toString();
        settings.endGroup();
    }
    else
    {
        QSettings settings("pcmanfm", "default/pcmanfm");
        settings.beginGroup("desktop");
        result=settings.value(key, "").toString();
        settings.endGroup();
    }
    return result;
}

void Global::setPcManFmValue(const QString &key, const QString &value){
    if(QDir(gv.homePath+"/.config/pcmanfm/lubuntu").exists()){
        QSettings settings("pcmanfm", "lubuntu/pcmanfm");
        settings.beginGroup("desktop");
        settings.setValue(key, value);
        settings.endGroup();
        settings.sync();
    }
    else
    {
        QSettings settings("pcmanfm", "default/pcmanfm");
        settings.beginGroup("desktop");
        settings.setValue(key, value);
        settings.endGroup();
        settings.sync();
    }
}

void Global::rotateImageBasedOnExif(const QString &image)
{
    //getExifRotation() will return 0 if it has no data, in other case it will return 1-8;
    short currentExifRotation = getExifRotation(image);

    if(currentExifRotation > 1){
        Global::rotateImg(image, currentExifRotation, true); //if currentExifRotation==1, then the rotation is normal
    }
}

short Global::getExifRotation(const QString &filename){
    ExifData *data = exif_data_new_from_file(filename.toLocal8Bit().data());
    ExifByteOrder byte_order = exif_data_get_byte_order(data);
    ExifEntry *entry;

    short return_value=0;
    if(data){
        if ((entry = exif_content_get_entry( data->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION))){
            return_value = (short) exif_get_short(entry->data, byte_order);
        }
        exif_data_unref(data); //no need to exif_entry_unref(entry), there is no memory leak here.
    }
    return return_value;
}
#endif //#ifdef ON_LINUX

QString Global::setAverageColor(const QString &image){
    //sets the desktop's average color and returns the name of it.
    QString averageColor = QColor::fromRgb(QImage(image).scaled(1, 1, Qt::IgnoreAspectRatio, Qt::FastTransformation).pixel(0, 0)).name();
#ifdef ON_LINUX
    Global::setPrimaryColor(averageColor);
#endif //#ifdef ON_LINUX

    return averageColor;
}

void Global::saveHistory(const QString &image, short feature){
    QDateTime dateNow = QDateTime::currentDateTime();
    QSettings history_settings(HISTORY_SETTINGS);
    history_settings.beginGroup(QString::number(dateNow.date().year()));
    history_settings.beginGroup(QString::number(dateNow.date().month()));
    int size = history_settings.beginReadArray(dateNow.toString("dd"));
    history_settings.endArray();
    history_settings.beginWriteArray(dateNow.toString("dd"));
    history_settings.setArrayIndex(size);
    history_settings.setValue("time", dateNow.toString("hh:mm"));
    if(feature==1){
        history_settings.setValue("path", image);
    }
    else if(feature==2 || feature==3 || feature==5){
        history_settings.setValue("path", gv.onlineLinkForHistory);
    }
    else if(feature==4){
        history_settings.setValue("path", "");
    }
    history_settings.setValue("type", feature);
    history_settings.endArray();
    history_settings.endGroup();
    history_settings.endGroup();
}

void Global::desktopNotify(const QString text, bool checkImage, const QString &image){

    if(checkImage && !QFile::exists(image))
    {
        Global::error("Could not display notification");
        return;
    }

#ifdef ON_WIN32
    if(notification_==NULL)
    {
        notification_ = new Notification(text, image, 0);
        notification_->setAttribute(Qt::WA_DeleteOnClose);
        connect(notification_, SIGNAL(destroyed()), this, SLOT(notificationDestroyed()));
        connect(this, SIGNAL(updateNotification(QString,QString)), notification_, SLOT(setupNotification(QString,QString)));
        notification_->show();
    }
    else
    {
        Q_EMIT updateNotification(text, image);
    }
#elif ON_LINUX
    static NotifyNotification* notification=NULL;

    if (!notify_init ("update-notifications")){
        Global::error("Could not display notification");
        return;
    }

    gboolean successfullyCreated;
    GError* error = NULL;

    if(notification==NULL){

        notification = notify_notification_new ( "Wallch", text.toLocal8Bit().data(), image.toLocal8Bit().data());
        error = NULL;
        successfullyCreated = notify_notification_show (notification, &error);
        if (!successfullyCreated)
        {
            g_print ("That did not work ... \"%s\".\n", error->message);
            g_error_free (error);
        }
    }
    else
    {
        notify_notification_update (notification, "Wallch", text.toLocal8Bit().data(), image.toLocal8Bit().data());
        error = NULL;
        successfullyCreated = notify_notification_show (notification, &error);
        if (!successfullyCreated)
        {
            g_print ("That did not work ... \"%s\".\n", error->message);
            g_error_free (error);
        }
    }
#endif //#ifdef ON_LINUX
}

#ifdef ON_WIN32
void Global::notificationDestroyed(){
    notification_=NULL;
}
#endif

void Global::rotateImg(const QString &filename, short rotation_type, bool show_messagebox){
    /*
     * A function for rotating the image 'filename' based on its 'rotation_type'
     * Rotation_type is corresponding to the exif values of the image.
     */
    QString ext=filename.right(3);
    if(ext == "gif" || ext == "GIF"){
        if(show_messagebox){
            Global::error("Rotation is not supported for GIF files!");
        }
        else
        {
            QMessageBox::warning(0, tr("Error!"), tr("Rotation is not supported for GIF files!"));
        }
        return;
    }
    QString extension;
    if(ext=="jpg" || ext=="JPG" || ext=="peg" || ext=="PEG"){
        extension="JPG";
    }
    else if(ext=="png" || ext=="PNG"){
        extension="PNG";
    }
    else if(ext=="bmp" || ext=="BMP"){
        extension="BMP";
    }
    /*
     *     'switching', based on the Exif data "orientation"
     *     1        2       3      4         5            6           7          8
     *
     *   888888  888888      88  88      8888888888  88                  88  8888888888
     *   88          88      88  88      88  88      88  88          88  88      88  88
     *   8888      8888    8888  8888    88          8888888888  8888888888          88
     *   88          88      88  88
     *   88          88  888888  888888
     *
     */
    switch (rotation_type){
    case 2:
        QImage(filename).mirrored(true, false).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 3:
        QImage(filename).mirrored(true, true).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 4:
        QImage(filename).mirrored(false, true).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 5:
        QImage(filename).transformed(QTransform().rotate(90), Qt::SmoothTransformation).mirrored(true, false).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 6:
        QImage(filename).transformed(QTransform().rotate(90), Qt::SmoothTransformation).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 7:
        QImage(filename).transformed(QTransform().rotate(-90), Qt::SmoothTransformation).mirrored(true, false).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    case 8:
        QImage(filename).transformed(QTransform().rotate(-90), Qt::SmoothTransformation).save(filename, extension.toLocal8Bit().data(), 100);
        break;
    }
}

QStringList Global::listFolders(const QString &parentFolder, bool recursively, bool includeParent){
    QStringList all_dirs;
    if(recursively){
        if(includeParent){
            all_dirs << parentFolder;
        }
        QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

        while(directories.hasNext()){
            directories.next();
            all_dirs << directories.filePath();
        }
        return all_dirs;
    }
    else
    {
        //returns only the top level directories
        QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

        while(directories.hasNext()){
            directories.next();
            all_dirs << directories.filePath();
        }
        return all_dirs;
    }
}

bool Global::connectedToInternet()
{
    internetChecked_=false;

    disconnectFileDownloader();

    connect(fileDownloader_, SIGNAL(finished(QNetworkReply*)), this, SLOT(internetConnectionReplyFinished(QNetworkReply*)));

    fileDownloader_->get(QNetworkRequest(QUrl("http://google.com")));

    while(!internetChecked_){
        qApp->processEvents();
    }

    return connectedToInternet_;
}

void Global::internetConnectionReplyFinished(QNetworkReply *reply)
{
    connectedToInternet_=(reply->error() == QNetworkReply::NoError);
    internetChecked_=true;
    reply->deleteLater();
}

void Global::readClockIni(const QString &path)
{
    //Reading clock.ini of wallpaper clock to get useful info from there.
    QFile file(path+"/clock.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        Global::error("Could not open the clock.ini file ("+path+"/clock.ini) for reading successfully!");
        return;
    }

    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line=in.readLine();
        if(line.startsWith("refreshhourinterval="))
        {
            gv.refreshhourinterval=QString(line.right(line.count()-20)).toInt();
        }
        else if(line.startsWith("ampmenabled="))
        {
            gv.amPmEnabled=QString(line.right(line.count()-12)).toInt();
        }
        else if(line.startsWith("ampmenabled="))
        {
            gv.wallpaperClocksHourImages=QString(line.right(line.count()-11)).toInt();
        }
    }
    file.close();
}

QString Global::wallpaperClockNow(const QString &path, bool minuteCheck, bool hourCheck, bool amPmCheck, bool dayWeekCheck, bool dayMonthCheck, bool monthCheck)
{
    //returns where the saved wallpaper clock went

    //getting system date and time
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    //knowing the current hour, we know if it am or pm
    //12h format is needed for the analog clocks
    short hour_12h_format=0;
    QString am_or_pm;
    if (time.hour()>=12){
        am_or_pm="pm";
    }
    else
    {
        am_or_pm="am";
    }

    if(gv.wallpaperClocksHourImages==24 || time.hour()<12 ){
        hour_12h_format=time.hour();
    }
    else if(gv.wallpaperClocksHourImages!=24 && time.hour()>=12){
        hour_12h_format=time.hour()-12;
    }

    //create the image that will be used as desktop background
    QPixmap merge(path+"/bg.jpg");

    QPainter painter(&merge);
    if(monthCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/month"+QString::number(date.month())+".png"));
    }
    if(dayWeekCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/weekday"+QString::number(date.dayOfWeek())+".png"));
    }
    if(dayMonthCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/day"+QString::number(date.day())+".png"));
    }
    if(hourCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/hour"+QString::number(hour_12h_format*(60/gv.refreshhourinterval)+QString::number(time.minute()/gv.refreshhourinterval).toInt())+".png"));
    }
    if(minuteCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/minute"+QString::number(time.minute())+".png"));
    }
    if(gv.amPmEnabled && amPmCheck){
        painter.drawPixmap(0,0, QPixmap(path+"/"+am_or_pm+".png"));
    }
    painter.end();

    Global::remove(gv.wallchHomePath+WC_IMAGE+"*");

    QString currentWallpaperClock=gv.wallchHomePath+WC_IMAGE+QString::number(QDateTime::currentMSecsSinceEpoch())+".jpg";

    merge.save(currentWallpaperClock);

    WallpaperManager::setBackground(currentWallpaperClock, true, false, 4);
    if(gv.setAverageColor){
        Q_EMIT averageColorChanged();
    }

    if(gv.showNotification)
    {
        if(settings->value("clocks_notification", true).toBool()){
            if(QTime::currentTime().minute()==0)
                Global(false).desktopNotify(tr("Current wallpaper has been changed!")+"\n"+tr("Time is")+": "+QTime::currentTime().toString("hh:mm"), true, currentWallpaperClock);
        }
        else{
            Global(false).desktopNotify(tr("Current wallpaper has been changed!")+"\n"+tr("Time is")+": "+QTime::currentTime().toString("hh:mm"), true, currentWallpaperClock);
        }
    }

    return currentWallpaperClock;
}

QString Global::basenameOf(const QString &path){
    /*
     * Returns the fullname of the file that 'path' points to. E.g.:
     * path='/home/alex/a.txt' -> basenameOf(path)=='a.txt'
     * This can be done with the QFileInfo as well, but it is too time&resource consuming
     */

    short pathCount=path.count();
    for(short i=pathCount-1; i>=0; i--){
        if(path.at(i)=='/' || path.at(i)=='\\' ){
            if(i==(pathCount-1)){
                continue; //it was a directory
            }
            return path.right(pathCount-i-1);
        }
    }

    return QString(""); //it means that '/' or '\' does not exist
}

QString Global::dirnameOf(const QString &path){
    //returns the absolute folder path of the file
    return path.left(path.count()-(basenameOf(path).count()+1));//the +1 so as to remove the trailing '/'
}

QString Global::suffixOf(const QString &path){
    short pathCount=path.count();
    for(short i=pathCount-1; i>=0; i--){
        if(path.at(i)=='.'){
            return path.right(pathCount-i-1);
        }
    }

    return QString("");
}

int Global::websiteSliderValueToSeconds(short value)
{
    switch (value)
    {
    case 1:
        return 120;
        break;
    case 2:
        return 180;
        break;
    case 3:
        return 300;
        break;
    case 4:
        return 600;
        break;
    case 5:
        return 900;
        break;
    case 6:
        return 1200;
        break;
    case 7:
        return 1800;
        break;
    case 8:
        return 2700;
        break;
    case 9:
        return 3600;
        break;
    case 10:
        return 7200;
        break;
    case 11:
        return 10800;
        break;
    case 12:
        return 14400;
        break;
    case 13:
        return 21600;
        break;
    case 14:
        return 43200;
        break;
    case 15:
        return 86400;
        break;
    case 16:
        return 604800;
        break;
    default:
        return DEFAULT_SLIDER_DELAY;
        break;
    }
}

QString Global::timeNowToString(){
    return QTime::currentTime().toString("HH:mm");
}

bool Global::remove(const QString &files){
    /*
     * Removes one or multiple files (multiple files with * matching).
     * For removing entire folder, use removeDir() instead.
     */

    if(files.contains("*")){
        //more than one files to delete, use RegExp

        QString parent_dir = dirnameOf(files);
        if(parent_dir.isEmpty()){
            return false;
        }

        QDir rmDir(parent_dir);

        if(!rmDir.exists()){
            return false;
        }

        rmDir.setFilter(QDir::NoDotAndDotDot | QDir::Files);

        QRegExp rmValidation;
        rmValidation.setPatternSyntax(QRegExp::Wildcard);
        rmValidation.setPattern(basenameOf(files));

        bool result=true;

        Q_FOREACH(QString currentFile, rmDir.entryList()){
            if(rmValidation.indexIn(currentFile)!=-1){
                //currentFile matches the validation
                if(!QFile::remove(parent_dir+"/"+currentFile)){
                    Global::error("Could not remove file '"+parent_dir+"/"+currentFile+"'. Skipping...");
                    result=false;
                }
            }

        }
        //result is false if at least one file wasn't deleted properly.
        return result;
    }
    else
    {
        return QFile::exists(files) && QFile::remove(files);
    }
}

QString Global::getFilename(const QString &file){
    if(!file.contains('*')){
        return QString();
    }

    QString parentDirectory=dirnameOf(file);
    if(parentDirectory.isEmpty()){
        return QString();
    }

    QDir parentDir(parentDirectory);

    if(!parentDir.exists()){
        return QString();
    }

    parentDir.setFilter(QDir::NoDotAndDotDot | QDir::Files);

    QRegExp fileValidation;
    fileValidation.setPatternSyntax(QRegExp::Wildcard);
    fileValidation.setPattern(basenameOf(file));

    Q_FOREACH(QString curFile, parentDir.entryList()){
        if(fileValidation.indexIn(curFile)!=-1){
            return parentDirectory+'/'+curFile;
        }

    }
    return QString();

}

void Global::saveSecondsLeftNow(int secondsLeft, short forType){
    /*
     * for_type:
     * 0 - Wallpapers
     * 1 - Live Earth
     * 2 - Live Website
     */

    if(secondsLeft==-1){
        settings->setValue("seconds_left_interval_independence", INTERVAL_INDEPENDENCE_DEFAULT_VALUE);
    }
    else
    {
        QChar front;
        switch(forType){
        default:
        case 0:
            front='p';
            break;
        case 1:
            front='e';
            break;
        case 2:
            front='w';
            break;
        }
        QDateTime currentTime = QDateTime::currentDateTime();
        settings->setValue("seconds_left_interval_independence",
                           QString(front)+"."+
                           QString::number(secondsLeft)+"."+
                           currentTime.date().toString("yyyy")+":"+
                           currentTime.date().toString("MM")+":"+
                           currentTime.date().toString("dd")+":"+
                           currentTime.time().toString("HH")+":"+
                           currentTime.time().toString("mm")+":"+
                           currentTime.time().toString("ss")
                           );
    }

    settings->sync();
}

#ifdef ON_LINUX
void Global::gsettingsSet(const QString &schema, const QString &key, const QString &value){
    GSettings *settings = g_settings_new(schema.toLocal8Bit().data());

    g_settings_set_string (settings, key.toLocal8Bit().data(), value.toLocal8Bit().data());

    g_settings_sync ();

    if (settings != NULL){
        g_object_unref (settings);
    }
}

QString Global::gsettingsGet(const QString &schema, const QString &key){
    GSettings *settings = g_settings_new(schema.toLocal8Bit().data());
    gchar *printed = g_settings_get_string (settings, key.toLocal8Bit().data());
    QString finalSetting = QString(printed);
    g_free (printed);

    if (settings != NULL){
        g_object_unref (settings);
    }

    return finalSetting;
}
#endif //#ifdef ON_LINUX

QString Global::base64Decode(const QString &string){
    return QByteArray::fromBase64(QByteArray().append(string));
}

void Global::resetSleepProtection(int timeoutCount){
    gv.runningTimeOfProcess = QDateTime::currentDateTime();
    gv.timeToFinishProcessInterval = gv.runningTimeOfProcess.addSecs(timeoutCount);
}

void Global::addPreviousBackground(QStringList &previous_backgrounds, const QString &image){
    if(image.isEmpty()){
        return;
    }
    previous_backgrounds << image;
    if(previous_backgrounds.count()>PREVIOUS_PICTURES_LIMIT){
        previous_backgrounds.removeFirst();
    }
}

void Global::error(const QString &message){
    cerr << "Error: " << message.toLocal8Bit().data() << endl;
}

void Global::debug(const QString &message){
    cout << message.toLocal8Bit().data() << endl;
}

QString Global::secondsToMinutesHoursDays(int seconds)
{
    if(seconds%86400==0)
    {
        if(seconds/86400==1){
            return QString(QString::number(1)+" "+tr("day"));
        }
        else{
            return QString(QString::number(seconds/86400)+" "+tr("days"));
        }
    }
    else if(seconds%3600==0)
    {
        if(seconds/3600==1){
            return QString(QString::number(seconds/3600)+" "+tr("hour"));
        }
        else{
            return QString(QString::number(seconds/3600)+" "+tr("hours"));
        }
    }
    else if(seconds%60==0)
    {
        if(seconds/60==1){
            return QString(QString::number(seconds/60)+" "+tr("minute"));
        }
        else{
            return QString(QString::number(seconds/60)+" "+tr("minutes"));
        }
    }
    else if(seconds==1){
        return QString(QString::number(1)+" "+tr("second"));
    }
    else
    {
        return QString(QString::number(seconds)+" "+tr("seconds"));
    }
}

bool Global::foldersAreSame(QString folder1, QString folder2){
    if(!folder1.endsWith('/')){
        folder1+='/';
    }

    if(!folder2.endsWith('/')){
        folder2+='/';
    }

    return (folder1==folder2);
}

#ifdef ON_LINUX
void Global::changeIndicatorIcon(const QString &icon)
{
    if(icon=="normal")
    {
        if(gv.currentTheme=="ambiance"){
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_AMBIANCE_NORMAL, INDICATOR_DESCRIPTION);
        }
        else
        {
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_RADIANCE_NORMAL, INDICATOR_DESCRIPTION);
        }
    }
    else if(icon=="right")
    {
        if(gv.currentTheme=="ambiance"){
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_AMBIANCE_RIGHT, INDICATOR_DESCRIPTION);
        }
        else
        {
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_RADIANCE_RIGHT, INDICATOR_DESCRIPTION);
        }
    }
    else if(icon=="left"){
        if(gv.currentTheme=="ambiance"){
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_AMBIANCE_LEFT, INDICATOR_DESCRIPTION);
        }
        else
        {
            app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_RADIANCE_LEFT, INDICATOR_DESCRIPTION);
        }
    }
    else
    {
        gv.doNotToggleRadiobuttonFallback=false;
    }
}

void Global::changeIndicatorSelection(const QString &status){
    if(status=="wallpapers"){
        if(gv.wallpapersRunning){
            gv.doNotToggleRadiobuttonFallback=true;
            if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM (gv.wallpapersRadiobutton))){
                gv.doNotToggleRadiobuttonFallback=false;
            }
            else
            {
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.wallpapersRadiobutton), TRUE);
            }
            showWallpapersIndicatorControls(true, true);
        }
        else
        {
            if(gv.processPaused){
                showWallpapersIndicatorControls(true, false);
            }
            else
            {
                showWallpapersIndicatorControls(false, true);
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.iAmNotThere), TRUE);
                gv.doNotToggleRadiobuttonFallback=false;
            }
        }
    }
    else if(status=="pause"){
        showWallpapersIndicatorControls(true, false);
    }
    else if(status=="earth"){
        gv.doNotToggleRadiobuttonFallback=true;
        if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gv.liveEarthRadiobutton))){
            gv.doNotToggleRadiobuttonFallback=false;
        }
        else
        {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.liveEarthRadiobutton), TRUE);
        }
    }
    else if(status=="potd"){
        gv.doNotToggleRadiobuttonFallback=true;
        if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gv.potdRadiobutton))){
            gv.doNotToggleRadiobuttonFallback=false;
        }
        else
        {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.potdRadiobutton), TRUE);
        }
    }
    else if(status=="clocks"){
        gv.doNotToggleRadiobuttonFallback=true;
        if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gv.wallpaperClocksRadiobutton))){
            gv.doNotToggleRadiobuttonFallback=false;
        }
        else
        {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.wallpaperClocksRadiobutton), TRUE);
        }
    }
    else if(status=="website"){
        gv.doNotToggleRadiobuttonFallback=true;
        if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gv.liveWebsiteRadiobutton))){
            gv.doNotToggleRadiobuttonFallback=false;
        }
        else
        {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.liveWebsiteRadiobutton), TRUE);
        }
    }
    else
    {
        showWallpapersIndicatorControls(false, true);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gv.iAmNotThere), TRUE);
        gv.doNotToggleRadiobuttonFallback=false;
    }
}

void Global::showWallpapersIndicatorControls(bool show, bool pauseText){
    if(show){
        gtk_widget_hide(gv.wallpapersJustChange);
        gtk_widget_show(gv.wallpapersPlayPause);
    }
    else
    {
        gtk_widget_show(gv.wallpapersJustChange);
        gtk_widget_hide(gv.wallpapersPlayPause);
        gtk_widget_hide(gv.wallpapersNext);
        gtk_widget_hide(gv.wallpapersPrevious);
    }
    if(pauseText){
        if(gtk_widget_get_visible(gv.wallpapersPlayPause)){
            gtk_widget_show(gv.wallpapersNext);
            gtk_widget_show(gv.wallpapersPrevious);
        }
        gtk_menu_item_set_label((GtkMenuItem*) gv.wallpapersPlayPause, tr("   Pause").toLocal8Bit().data());
    }
    else
    {
        gtk_widget_hide(gv.wallpapersNext);
        gtk_widget_hide(gv.wallpapersPrevious);
        gtk_menu_item_set_label((GtkMenuItem*) gv.wallpapersPlayPause, tr("   Start").toLocal8Bit().data());
    }
}

#endif //#ifdef ON_LINUX

int Global::getSecondsTillHour(const QString &hour){
    int secsToPotd=QTime::currentTime().secsTo(QTime::fromString(hour, "hh:mm"));
    if(secsToPotd<0){
        secsToPotd=86400-(secsToPotd*-1);//seconds in one day
    }
    else if(secsToPotd==0){
        //this is the case where POTD time has come at the exact exact second of the minute change. POTD has come, so the progressbar has to be reset
        secsToPotd=86400;
    }
    return secsToPotd;
}

void Global::fixCacheSize(QString curWallpapersFolder){
    CacheEntry currentCacheEntry=getCacheStatus();
    if(currentCacheEntry.size<=MAX_IMAGE_CACHE*BYTES_PER_MiB){
        return;
    }

    //step 1: Remove all cached images that refer to non-existent images and all cached images that refer to images that have changed (same filename, different image)
    for(int i=0;i<currentCacheEntry.imageCount;i++){
        QString cached_image = currentCacheEntry.images.at(i);
        QString original_image=cacheToOriginalName(cached_image);
        if(!QFile::exists(original_image) || QFile(original_image).size()!=cacheGetSizeOfCachedName(cached_image)){
            QFile::remove(gv.cachePath+cached_image);
            currentCacheEntry.size-=currentCacheEntry.imagesSizeList.at(i);
            currentCacheEntry.images.removeAt(i);
            currentCacheEntry.imagesSizeList.removeAt(i);
            currentCacheEntry.imageCount--;
            i--;
        }
        if(i%1000==0){
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }

    if(currentCacheEntry.size<=MAX_IMAGE_CACHE*BYTES_PER_MiB){
        gv.currentImageCacheSize=currentCacheEntry.size;
        return;
    }

    //step 2: Now all cached images are perfectly valid (pointing to existing images). Remove the images that are not under Wallpapers' current path
    int max_cache=MAX_IMAGE_CACHE*BYTES_PER_MiB;
    for(int i=0;i<currentCacheEntry.imageCount && currentCacheEntry.size>max_cache;i++){
        QString cached_image = currentCacheEntry.images.at(i);
        QString original_image=cacheToOriginalName(cached_image);
        QString dir_name=Global::dirnameOf(original_image);
        if(!isSubfolder(dir_name, curWallpapersFolder)){
            QFile::remove(gv.cachePath+cached_image);
            currentCacheEntry.size-=currentCacheEntry.imagesSizeList.at(i);
            currentCacheEntry.images.removeAt(i);
            currentCacheEntry.imagesSizeList.removeAt(i);
            currentCacheEntry.imageCount--;
            i--;
        }
        if(i%1000==0){
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }

    if(currentCacheEntry.size<=MAX_IMAGE_CACHE*BYTES_PER_MiB){
        gv.currentImageCacheSize=currentCacheEntry.size;
        return;
    }

    //step 3: The only thing left is to delete the images that ARE into the default path.
    for(int i=currentCacheEntry.imageCount-1;i>=0 && currentCacheEntry.size>max_cache;i--){
        QFile::remove(gv.cachePath+currentCacheEntry.images.at(i));
        currentCacheEntry.size-=currentCacheEntry.imagesSizeList.at(i);
        currentCacheEntry.images.removeAt(i);
        currentCacheEntry.imagesSizeList.removeAt(i);
        if(i%1000==0){
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }

    gv.currentImageCacheSize=currentCacheEntry.size;
}

bool Global::isSubfolder(QString &subfolder, QString &parentFolder){
    //Note that this function returns true even if subfolder==parent_folder
    if(!subfolder.endsWith('/')){
        subfolder+='/';
    }
    if(!parentFolder.endsWith('/')){
        parentFolder+='/';
    }
    return subfolder.startsWith(parentFolder);
}

QString Global::originalToCacheName(QString imagePath){
    /*
     * How the cache works:
     * 1: All / are converted to ^ (or the cacheToChar)
     * 2: If ^ is already on the string, the indexes of it are saved at the start of the cached filename
     * 3: Lastly, the size of the original image is saved at the start of the cached image to prevent
     * having a different image with the same filename.
     */
#ifdef ON_WIN32
    QString file_size=QString::number(QFile(imagePath).size());
    imagePath.remove(1, 1); //removing the ":" from the filename
    if(imagePath.contains(cacheToChar)){
        //imagePath already includes the cacheToChar, so save the index(es) of it
        QString final_string=imagePath;
        short currentIndex=0;
        while((currentIndex=imagePath.indexOf(cacheToChar, currentIndex+1))!=-1)
            final_string.insert(0, QString::number(currentIndex)+cacheAlreadyIncludedDelimiter);
        final_string.insert(0, file_size+cache_size_char);
        return final_string.replace(cacheFromChar, cacheToChar);
    }
    else
    {
        imagePath.insert(0, file_size+cache_size_char);
        return imagePath.replace(cacheFromChar, cacheToChar);
    }
#else
    if(imagePath.contains(cacheToChar)){
        //imagePath already includes the cacheToChar, so save the index(es) of it
        QString final_string=imagePath;
        short currentIndex=0;
        while((currentIndex=imagePath.indexOf(cacheToChar, currentIndex+1))!=-1)
            final_string.insert(0, QString::number(currentIndex)+cacheAlreadyIncludedDelimiter);
        final_string.insert(0, QString::number(QFile(imagePath).size())+cache_size_char);
        return final_string.replace(cacheFromChar, cacheToChar);
    }
    else
    {
        imagePath.insert(0, QString::number(QFile(imagePath).size())+cache_size_char);
        return imagePath.replace(cacheFromChar, cacheToChar);
    }
#endif //#ifdef ON_WIN32
}

QString Global::cacheExcludeSize(const QString &cacheName){
    QChar mchar;
    short i=0, extrasCount=1, cacheNameCount=cacheName.count();
    for(mchar=cacheName.at(i);mchar!=cache_size_char && i<cacheNameCount;i++, mchar=cacheName.at(i)){
        extrasCount++;
    }
    return cacheName.right(cacheName.count()-extrasCount);
}

QString Global::cacheToOriginalName(QString cacheName){
    cacheName=cacheExcludeSize(cacheName);
    short cacheNameCount=cacheName.count();

    if(!cacheName.startsWith(cacheToChar)){
        //the rare case where the original filename already included the cacheToChar
        QStringList sNums=cacheName.split(cacheAlreadyIncludedDelimiter);
        QList<short> nums;
        short oc_count=sNums.count();
        short length_to_omit=0, numsCount=0;
        for(short i=0;i<oc_count;i++){
            bool isNum=false;
            short num=sNums.at(i).toShort(&isNum);
            if(isNum){
                length_to_omit+=(sNums.at(i).length()+1); //the +1 for the cacheAlreadyIncludedDelimiter
                nums.append(num);
                numsCount++;
            }
            else{
                break;
            }
        }
        cacheName=cacheName.right(cacheNameCount-length_to_omit);
        cacheName.replace(cacheToChar, cacheFromChar);
        for(short i=0;i<numsCount;i++)
            cacheName.replace(nums.at(i), 1, cacheToChar);
#ifdef ON_WIN32
        return cacheName.insert(1, ':'); //adding the ":" to the filename
#else
        return cacheName;
#endif //#ifdef ON_WIN32
    }
    else
    {
#ifdef ON_WIN32
        return cacheName.replace(cacheToChar, cacheFromChar).insert(1, ':'); //adding the ":" to the filename
#else
        return cacheName.replace(cacheToChar, cacheFromChar);
#endif //#ifdef ON_WIN32
    }
}

qint64 Global::cacheGetSizeOfCachedName(const QString &cacheName){
    //returns the size of the original image of cache_name, which is included inside cache_name
    QChar mchar;
    short i=0, extra_count=0, cache_name_count=cacheName.count();
    for(mchar=cacheName.at(i);mchar!=cache_size_char && i<cache_name_count;i++, mchar=cacheName.at(i))
        extra_count++;
    return (qint64) cacheName.left(extra_count).toInt();
}

CacheEntry Global::getCacheStatus(){
    //returns a 'cacheEntry' (all cached images, individual and total size of them)
    int final_size=0; int counter=0;
    CacheEntry entry;
    entry.images=QDir(gv.cachePath, QString(), QDir::Name, QDir::Files).entryList();
    Q_FOREACH(QString file, entry.images){
        qint64 currentSize=QFile(gv.cachePath+file).size();
        final_size+=currentSize;
        entry.imagesSizeList.append(currentSize);
        if((++counter)%1000==0){
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }
    entry.imageCount=counter;
    gv.currentImageCacheSize=entry.size=final_size;
    return entry;
}

bool Global::addToCacheSize(qint64 size){
    gv.currentImageCacheSize+=size;
    return (gv.currentImageCacheSize>MAX_IMAGE_CACHE*BYTES_PER_MiB);
}

QString Global::getOutputOfCommand(QString command, QStringList parameters){
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command, parameters,QIODevice::ReadWrite);

    if(!process.waitForStarted())
        return QString();

    QByteArray data;

    while(process.waitForReadyRead()){
        data.append(process.readAll());
    }

    return data.data();
}

void Global::openUrl(const QString &url){
    if(!QDesktopServices::openUrl(QUrl(url))){
        Global::error("I probably could not open \""+url+"\"");
    }
}

void Global::updateStartup()
{
    if(settings->value("Startup", true).toBool()){
#ifdef ON_WIN32
        if(!QFile(settings->value("ProgramFilesPath", "C:\\Program Files (x86)\\Wallch\\wallch.exe").toString()).exists()){
            return;
        }

        QSettings settings2("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        if(gv.wallpapersRunning){
            if(settings->value("Once", false).toBool()){
                settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --change");
            }
            else{
                settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --start");
            }
        }
        else if(gv.liveEarthRunning){
            settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --earth");
        }
        else if(gv.potdRunning){
            settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --potd");
        }
        else if(gv.liveWebsiteRunning){
            settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --website");
        }
        else if(gv.wallpaperClocksRunning){
            settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --clock");
        }
        else{
            settings2.setValue("Wallch", "C:\\Program Files (x86)\\Wallch\\wallch.exe --none");
        }
        settings2.sync();
#else
        if(!QDir(gv.homePath+AUTOSTART_DIR).exists()){
            if(!QDir().mkpath(gv.homePath+AUTOSTART_DIR)){
                Global::error("Failed to create ~"+QString(AUTOSTART_DIR)+" folder. Please check folder existence and permissions.");
            }
        }
        QString desktopFileCommand, desktopFileComment;

        if(gv.wallpapersRunning){
            if(settings->value("Once", false).toBool()){
                desktopFileCommand="/usr/bin/wallch --change";
                desktopFileComment="Wallch will pick a random picture from the list and set it as background";
            }
            else{
                desktopFileCommand="/usr/bin/wallch --start";
                desktopFileComment="Start Changing Wallpapers";
            }
        }
        else if(gv.liveEarthRunning){
            desktopFileCommand="/usr/bin/wallch --earth";
            desktopFileComment="Enable Live Earth";
        }
        else if(gv.potdRunning){
            desktopFileCommand="/usr/bin/wallch --potd";
            desktopFileComment="Enable Picture Of The Day";
        }
        else if(gv.liveWebsiteRunning){
            desktopFileCommand="/usr/bin/wallch --website";
            desktopFileComment="Enable Live Website";
        }
        else if(gv.wallpaperClocksRunning){
            desktopFileCommand="/usr/bin/wallch --clock";
            desktopFileComment="Enable Wallpaper Clock";
        }
        else{
            desktopFileCommand="/usr/bin/wallch --none";
            desktopFileComment="Just show indicator";
        }
#ifdef ON_LINUX
        short defaultValue=3;
#else
        short defaultValue=0;
#endif

        if(settings->value("startup_timeout", defaultValue).toInt()!=0){
            desktopFileCommand="bash -c 'sleep "+QString::number(settings->value("startup_timeout", defaultValue).toInt())+" && "+desktopFileCommand+"'";
        }

        Global::remove(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE);

        if(!createDesktopFile(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE, desktopFileCommand, desktopFileComment)){
            Global::error("There was probably an error while trying to create the desktop file "+gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE);
        }

#endif //#ifdef ON_WIN32
    }
}

bool Global::createDesktopFile(const QString &path, const QString &command, const QString &comment){
    QFile desktopFile(path);
    if(!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        return false;
    }

    QTextStream out(&desktopFile);
    out << "\n[Desktop Entry]\nType=Application\nName=Wallch\nExec="+command+"\nTerminal=false\nIcon=wallch\nComment="+comment+"\nCategories=Utility;Application;\n";
    desktopFile.close();
    return true;
}
