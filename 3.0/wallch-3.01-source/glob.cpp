#define QT_NO_KEYWORDS

#include <QString>
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QMessageBox>
#include <QDateTime>
#include <QSettings>

#include <iostream>
#include <libnotify/notify.h>

#include "glob.h"
#include "mainwindow.h"
using namespace std;

//normal variables
bool notification_first_time=true;
NotifyNotification* notification;
gboolean            success;
GError*             error = NULL;

void Global::enable_unity(UnityLauncherEntry *entry, bool state){
    if(entry)
        unity_launcher_entry_set_progress_visible(entry, state);
}

void Global::set_unity_value(UnityLauncherEntry *entry, int timeout, int delay){
    if(entry){
        float progress = (float) timeout/delay;
        if(progress>1)
            progress/=100; //<- apf :P time_for_next needs 0-100 and this needs from 0 till 1 :P
        unity_launcher_entry_set_progress(entry, progress);
    }
}

void Global::save_image(QNetworkReply *data)
{
    QString filename;
    if(_live_earth_running_){
        QDateTime time;
        filename=QDir::homePath()+"/.config/Wallch/world"+QString::number(time.currentMSecsSinceEpoch())+".jpg";
    }
    else if(_photo_of_day_running_)
        filename=QDir::homePath()+"/.config/Wallch/photo_of_day.jpg";
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)){
        qDebug() << "Error saving image...\n";
        return;
    }
    file.write(data->readAll());
    file.close();
    if(!Global::set_background(filename)){
        cerr << "Something went wrong while setting the desktop background!\n";
        return;
    }
    if(_live_earth_running_ && _live_earth_show_message_){
        _live_earth_show_message_=0;
        if(!Global::desktop_notify("info",5))
            cerr << "Error while displaying notification!\n";
    }
    else if(_photo_of_day_running_ && _photo_of_day_show_message_){
        _photo_of_day_show_message_=0;
        if(!Global::desktop_notify("info",7))
            cerr << "Error while displaying notification!\n";
    }
}

void Global::photo_of_day_now(){
    QSettings settings( "Wallch", "Extras" );
    QDate date;
    settings.setValue("yearmonthday",QString::number(date.currentDate().year())+QString::number(date.currentDate().month())+QString::number(date.currentDate().dayOfWeek()));
    settings.sync();
    Global *photo_day=new Global;
    photo_day->download_image("http://dl.dropbox.com/u/11379868/photo_of_day");
}

void Global::start_photo_of_day_non_static(){
    Global::photo_of_day_now();
}

void Global::read_file(QNetworkReply *reply){
    if (reply->error()){
        qDebug() << "There was an error while processing your request:\n" << reply->errorString();
        return;
    }
    QString link_to_image(reply->readAll());
    link_to_image.chop(1);

    //removing all other images before attempting to get the new one...
    if(_live_earth_running_){
        if(system("rm -f ~/.config/Wallch/world*.jpg"))
            cerr << "Error removing ~/.config/Wallch/world*.jpg\n";
    }
    else if(_photo_of_day_running_){
        /*
          Here we *have* to check if the link has changed. Wikipedia sometimes
          tends to change this link at 01:00 UTC or later sometimes, when usually
          at approximately 00:00 UTC, so, if a user has set local time that matches
          00:45 UTC for example, the same image may be set two times in two different
          days. Hence, the checking... As for Live Earth, that's not such a big deal,
          as image changes every half an hour. So, there's a checking, and there will
          be another try of downloading the online file hosted on dropbox and checking
          the link. This will be done once every 10 mins till the link changes...
        */
        QSettings settings ( "Wallch", "Extras" );
        QString previous_img_URL=settings.value("previous_img_url", "").toString();
        if(previous_img_URL==link_to_image){
            /*
              Image has not yet change, so start a timer that will retry setting the
              Picture Of the Day in 10 minutes, this will be repeated till success!
            */
            cerr << "Image has not yet been updated, retrying in 10 mins.\n";
            QTimer::singleShot(600000, this, SLOT(start_photo_of_day_non_static()));
            return;
        }
        else
        {
        settings.setValue("previous_img_url", link_to_image);
        settings.sync();
        }
        if(system("rm -f ~/.config/Wallch/photo_of_day.jpg"))
            cerr << "Error removing ~/.config/Wallch/photo_of_day.jpg\n";
    }

    QNetworkAccessManager *download_image = new QNetworkAccessManager(this);
    QObject::connect(download_image, SIGNAL(finished(QNetworkReply*)), this, SLOT(save_image(QNetworkReply*)));
    download_image->get(QNetworkRequest(QUrl(link_to_image)));

}

void Global::download_image(QString url){
    QNetworkAccessManager *download_file = new QNetworkAccessManager(this);
    QObject::connect(download_file, SIGNAL(finished(QNetworkReply*)), this, SLOT(read_file(QNetworkReply*)));
    download_file->get(QNetworkRequest(QUrl(url)));
}

void Global::livearth(){
    Global *live = new Global();
    live->download_image("http://dl.dropbox.com/u/11379868/setupwallpaper");
}

bool Global::set_background(QString qimage){
    if(!QFile(qimage).exists())
        return false;
    QProcess *change_background = new QProcess(0);
    QString exec="gsettings";
    QStringList params;
    if(gnome_version==2){
        exec="gconftool-2";
        params << "--type" << "string" << "--set" << "/desktop/gnome/background/picture_filename" << qimage;
    }
    else if(gnome_version==3){
        qimage="file://"+qimage;
        exec="gsettings";
        params << "set" << "org.gnome.desktop.background" << "picture-uri" << qimage;
    }
    change_background->start(exec, params);
    if(!change_background->waitForFinished()){
        cerr << "Probably something went wrong while changing desktop background image!\n";
        delete change_background;
        return false;
    }
    if(change_background->exitCode()){
        cerr << "Probably something went wrong while changing desktop background image!\n";
        delete change_background;
        return false;
    }
    delete change_background;
    __statistic__images_n++;
    return true;
}

void Global::save_history(QString qimage){
    //getting the values of the day I'm interested in
    QDate date;
    QString month_short=QDate::shortMonthName(QDate::currentDate().month());
    QString month_long=QDate::longMonthName(QDate::currentDate().month());
    QString day_short=QDate::shortDayName(QDate::currentDate().dayOfWeek());
    QString day_long=QDate::longDayName(QDate::currentDate().dayOfWeek());
    QString day_of_month=QString::number(date.currentDate().day());
    QString year=QString::number(date.currentDate().year());
    QDir dir;
    //writing these values to the history file
    dir.mkpath(QDir::homePath()+"/.config/Wallch/History/"+year+" "+month_long);
    QFile his_file(QDir::homePath()+"/.config/Wallch/History/"+year+" "+month_long+"/"+day_of_month+" "+day_long);
    his_file.open(QIODevice::Append);
    QTextStream out(&his_file);
    if (history_type==1)
            out << day_short+" "+day_of_month+" "+month_short+" | Image:"+qimage << endl;
    else if(history_type==2){
        QTime time;
        QString hour=QString::number(time.currentTime().hour());
        QString minute=QString::number(time.currentTime().minute());
        out << day_short+" "+day_of_month+" "+month_short+" "+hour+":"+minute+" | Image:"+qimage << endl;
    }
    else{
        QTime time;
        QString hour=QString::number(time.currentTime().hour());
        QString minute=QString::number(time.currentTime().minute());
        QString second=QString::number(time.currentTime().second());
        out << day_short+" "+day_of_month+" "+month_short+" "+hour+":"+minute+":"+second+" | Image:"+qimage << endl;
    }
    his_file.close();
}

void Global::sound_notify(){
    if(_sound_notification_==1){
        if(system(QString("canberra-gtk-play -f "+QString(PREFIX)+"/share/wallch/files/notification.ogg > /dev/null&").toLocal8Bit().data()))
            cerr << "Error executing canberra-gtk-play\n";
    }
    else{
        if(system("mpg321 \"$(cat ~/.config/Wallch/Checks/text_to_button)\" 2> /dev/null&"))
            cerr << "Error executing mpg321\n";
    }
}

void Global::handle(){

}

bool Global::desktop_notify(QString qimage, int message_type){
    QString body;
    if(message_type==0)
    {
        if(!QFile(qimage).exists()){
            cerr << "Image for the notification doesn't exist!\n";
            return false;
        }
        body=QObject::tr("Your current wallpaper has changed!");
    }
    else if(message_type==1)
        body=QObject::tr("Live Earth Wallpaper is active<br>and it will change every 30 minutes!");
    else if(message_type==2)
        body=QObject::tr("There are lots of invalid images or too few. Please make a check of your images and try again!");
    else if(message_type==3)
        body=QObject::tr("You didn't have more than 1 picture in the list!");
    else if(message_type==4)
        body=QObject::tr("Unable to connect") + ". " + QObject::tr("Check your internet connection!");
    else if(message_type==5)
        body=QObject::tr("Live Earth Wallpaper is active and it will change every 30 minutes!");
    else if(message_type==6)
        body=QObject::tr("Requested Graphical User Interface! Every prior process has stopped!");
    else if(message_type==7)
        body=QObject::tr("Photo of the day is active and it will change once a day!");
    else if(message_type==8)
        body=QObject::tr("No process is running to do that.");
    if (!notify_init ("update-notifications")){
        cerr << "Couldn't display notification, unknown error!\n";
        return false;
    }
    if(notification_first_time){
        notification_first_time=false;

        notification = notify_notification_new ( "Wallch", body.toLocal8Bit().data(), qimage.toLocal8Bit().data());
        error = NULL;
        success = notify_notification_show (notification, &error);
        if (!success)
        {
                g_print ("That did not work ... \"%s\".\n", error->message);
                g_error_free (error);
        }

        g_signal_connect (G_OBJECT (notification), "closed", G_CALLBACK (&Global::handle), NULL);

        return true;
    }
    /* update the current notification with new content */
    success = notify_notification_update (notification, "Wallch",body.toLocal8Bit().data(), qimage.toLocal8Bit().data());


    error = NULL;
    success = notify_notification_show (notification, &error);
    if (!success)
    {
            g_print ("That did not work ... \"%s\".\n", error->message);
            g_error_free (error);
            return false;
    }
    g_signal_connect (G_OBJECT (notification), "closed", G_CALLBACK (&Global::handle), NULL);
    return true;
}

void Global::rotate_img(QString filename, int rotation_type){
    /*
      rotation_type is corresponding to the exif values of the image
      you can find these types here: http://paste.ubuntu.com/766679/
    */
    QString ext=filename.right(3);
    if(ext == "gif" || ext == "GIF"){
        QMessageBox::warning(0, tr("Wallch | Error"), tr("Rotation is not supported for GIF files!"));
        return;
    }
    const char* extension="";
    if(ext=="jpg" || ext=="JPG" || ext=="peg" || ext=="PEG")
        extension="JPG";
    else if(ext=="png" || ext=="PNG")
        extension="PNG";
    else if(ext=="bmp" || ext=="BMP")
        extension="BMP";
    QImage image(filename);
    QImage img1;
    QTransform rot;
    /*
      'switching', based on the Exif data "orientation"
      1        2       3      4         5            6           7          8

    888888  888888      88  88      8888888888  88                  88  8888888888
    88          88      88  88      88  88      88  88          88  88      88  88
    8888      8888    8888  8888    88          8888888888  8888888888          88
    88          88      88  88
    88          88  888888  888888

    */
    switch (rotation_type){
    case 2:
        img1=image.mirrored(true, false);
        img1.save(filename, extension, 100);
        return;
        break;
    case 3:
        rot.rotate(180);
        break;
    case 4:
        img1=image.mirrored();
        img1.save(filename, extension, 100);
        return;
        break;
    case 5:
        rot.rotate(90);
        img1 = img1.transformed(rot, Qt::SmoothTransformation);
        img1=image.mirrored(true, false);
        img1.save(filename, extension, 100);
        return;
        break;
    case 6:
        rot.rotate(90);
        break;
    case 7:
        rot.rotate(-90);
        img1 = img1.transformed(rot, Qt::SmoothTransformation);
        img1=image.mirrored(true, false);
        img1.save(filename, extension, 100);
        return;
        break;
    case 8:
        rot.rotate(-90);
        break;
    }
    //there's no mirror only rotation, so apply.
    image = image.transformed(rot, Qt::SmoothTransformation);
    image.save(filename, extension, 100);
}

bool Global::connected_to_internet()
{
    QProcess *ping_google = new QProcess(0);
    QString exec="ping";
    QStringList params;
    params << "google.com" << "-c" << "1" << "-w" << "2";
    ping_google->start(exec, params);
    if(!ping_google->waitForFinished(2005))
        return false;
    if(ping_google->exitCode())
        return false;
    return true;
}

bool Global::already_runs(){
    return !system("pidof wallch | grep \" \" > /dev/null");
}
