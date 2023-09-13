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

#ifdef Q_OS_LINUX
    #include <libnotify/notify.h>
    #include <libexif/exif-data.h>
#else
# ifdef Q_OS_WIN
    #include <windows.h>
# endif
#endif

#include "glob.h"
#include "mainwindow.h"

QSettings *settings = new QSettings("wallch", "Settings");

Global::Global(){}
Global::~Global(){}

bool Global::runsOnBattery(){
#ifdef Q_OS_LINUX
    QFile file("/sys/class/power_supply/BAT0/status");
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }
    QTextStream in(&file);
    QString content;
    content = in.readAll();
    file.close();
    return content.startsWith("Discharging");
#else
# ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS pwr;
    GetSystemPowerStatus(&pwr);
    return !(pwr.BatteryFlag & 8);
# endif
#endif
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

#ifdef Q_OS_LINUX
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
#endif

QString Global::setAverageColor(const QString &image){
    //sets the desktop's average color and returns the name of it.
    QString averageColor = WallpaperManager::getAverageColorOf(image).name();
    ColorManager::setPrimaryColor(averageColor);

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

#ifdef Q_OS_LINUX
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
#else
    if(notification_==NULL)
    {
        notification_ = new Notification(text, image, 0);
        notification_->setAttribute(Qt::WA_DeleteOnClose);
        connect(notification_, SIGNAL(destroyed()), this, SLOT(notificationDestroyed()));
        connect(this, SIGNAL(updateNotification(QString,QString)), notification_, SLOT(setupNotification(QString,QString)));
        notification_->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);//look like a popup
        notification_->setAttribute(Qt::WA_X11NetWmWindowTypeNotification, true);//stay on top
        notification_->setAttribute(Qt::WA_ShowWithoutActivating, true);//do not take focus
        notification_->show();
    }
    else
        Q_EMIT updateNotification(text, image);
#endif
}

#ifndef Q_OS_LINUX
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
    QStringList allDirs;
    if(recursively){
        if(includeParent){
            allDirs << parentFolder;
            if(gv.symlinks)
            {
                if(QDir(parentFolder).entryList().count()!=QDir(parentFolder).entryList(QDir::AllEntries | QDir::NoSymLinks).count())
                {
                    Q_FOREACH(QString file, QDir(parentFolder).entryList(QDir::AllEntries | QDir::NoDotAndDotDot)){
                        if(QFileInfo(parentFolder+"/"+file).isSymLink())
                        {
                            allDirs << Global::listFolders(QFileInfo(parentFolder+"/"+file).symLinkTarget(),true,true);
                        }
                    }
                }
            }
        }
        QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

        while(directories.hasNext()){
            directories.next();
            QString directFilepath=directories.filePath();
            allDirs << directFilepath;
            if(gv.symlinks)
            {
                if(QDir(directFilepath).entryList().count()!=QDir(directFilepath).entryList(QDir::AllEntries | QDir::NoSymLinks).count())
                {
                    Q_FOREACH(QString file, QDir(directFilepath).entryList(QDir::AllEntries | QDir::NoDotAndDotDot)){
                        if(QFileInfo(directFilepath+"/"+file).isSymLink())
                        {
                            allDirs << Global::listFolders(QFileInfo(directFilepath+"/"+file).symLinkTarget(),true,true);
                        }
                    }
                }
            }
        }
        allDirs.removeDuplicates();
        return allDirs;
    }
    else
    {
        //returns only the top level directories
        QDirIterator directories(parentFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

        while(directories.hasNext()){
            directories.next();
            allDirs << directories.filePath();
        }

        if(gv.symlinks)
        {
            if(QDir(parentFolder).entryList().count()!=QDir(parentFolder).entryList(QDir::AllEntries | QDir::NoSymLinks).count())
            {
                Q_FOREACH(QString file, QDir(parentFolder).entryList(QDir::AllEntries | QDir::NoDotAndDotDot)){
                    if(QFileInfo(parentFolder+"/"+file).isSymLink())
                    {
                        allDirs << Global::listFolders(QFileInfo(parentFolder+"/"+file).symLinkTarget(),true,true);
                    }
                }
            }
        }
        allDirs.removeDuplicates();
        return allDirs;
    }
}

QString Global::basenameOf(const QString &path){
    /*
     * Returns the fullname of the file that 'path' points to. E.g.:
     * path='/home/alex/a.txt' -> basenameOf(path)=='a.txt'
     * This can be done with the QFileInfo as well, but it is too time&resource consuming
     */

    short pathCount=path.count();
    bool isItselfDir=false;
    for(short i=pathCount-1; i>=0; i--){
        if(path.at(i)=='/' || path.at(i)=='\\' ){
            if(i==(pathCount-1)){
                isItselfDir=true;
                continue; //it was a directory
            }
            if(isItselfDir){
                QString withDirSeparator = path.right(pathCount-i-1);
                return withDirSeparator.left(withDirSeparator.count()-1);
            }
            else
            {
                return path.right(pathCount-i-1);
            }
        }
    }

    return QString(); //it means that '/' or '\' does not exist
}

QString Global::dirnameOf(const QString &path){
    //returns the absolute folder path of the file
    return QDir::toNativeSeparators(path.left(path.count()-(basenameOf(path).count()+1)));//the +1 so as to remove the trailing '/'
}

QString Global::suffixOf(const QString &path){
    // returns the extension, without the dot
    short pathCount=path.count();
    for(short i=pathCount-1; i>=0; i--){
        if(path.at(i)=='.'){
            return path.right(pathCount-i-1);
        }
    }

    return QString();
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

    if(files.endsWith('*')){
        //more than one files to delete, use RegExp

        QString parentDirectory = dirnameOf(files);
        if(parentDirectory.isEmpty()){
            return false;
        }

        QDir rmDir(parentDirectory);

        if(!rmDir.exists()){
            return false;
        }

        rmDir.setFilter(QDir::NoDotAndDotDot | QDir::Files);

        QRegularExpression rmValidation;
        rmValidation.setPattern(QRegularExpression::wildcardToRegularExpression(basenameOf(files)));

        bool result = true;

        Q_FOREACH(QString currentFile, rmDir.entryList()){
            if(rmValidation.match(currentFile).hasMatch()){
                //currentFile matches the validation
                if(!QFile::remove(parentDirectory+"/"+currentFile)){
                    Global::error("Could not remove file '"+parentDirectory+"/"+currentFile+"'. Skipping...");
                    result = false;
                    break;
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

    QRegularExpression fileValidation;
    fileValidation.setPattern(QRegularExpression::wildcardToRegularExpression(basenameOf(file)));

    Q_FOREACH(QString curFile, parentDir.entryList()){
        if(fileValidation.match(curFile).hasMatch())
            return parentDirectory+'/'+curFile;
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
    if(previous_backgrounds.count() > PREVIOUS_PICTURES_LIMIT){
        previous_backgrounds.removeFirst();
    }
}

void Global::error(const QString &message){
    cerr << "(" << QDateTime::currentDateTime().toString("dd/MM/yy HH:mm:ss").toLocal8Bit().data() << ") Error: " << message.toLocal8Bit().data() << endl;
}

void Global::debug(const QString &message){
    cout << message.toLocal8Bit().data() << endl;
}

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

bool Global::isSubfolder(QString &subFolder, QString &parentFolder){
    //Note that this function returns true even if subfolder==parent_folder
    if(!subFolder.endsWith('/')){
        subFolder+='/';
    }
    if(!parentFolder.endsWith('/')){
        parentFolder+='/';
    }
    return subFolder.startsWith(parentFolder);
}

void Global::openUrl(const QString &url){
    if(!QDesktopServices::openUrl(QUrl(url)))
        Global::error(url + " could not be opened");
}

QPixmap Global::roundedCorners(const QImage &image, const int radius){
    QBrush brush(image);

    QPen pen;
    pen.setColor(QColor(0, 0, 0, 0));

    QPixmap roundedPixmap(image.width(), image.height());
    roundedPixmap.fill(Qt::transparent);

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawRoundedRect(0, 0, image.width(), image.height(), radius, radius);
    painter.end();

    return roundedPixmap;
}

// Clipboard management

void Global::copyImageToClipboard(const QString &imagePath){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(QImage(imagePath));
}

void Global::copyTextToClipboard(const QString &text){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}
