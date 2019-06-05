/*Wallch - WallpaperChanger

A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vytanos

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.*/

#define QT_NO_KEYWORDS

#include <QtGui/QApplication>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusMessage>
#include <QArgument>
#include <QTimer>
#include <QFile>
#include <QSettings>
#include <QObject>
#include <QFileSystemWatcher>
#include <QtXml/QXmlStreamReader>

#include <iostream>
#include <fstream>
#include <unity/unity/unity.h>
using namespace std;
#include "mainwindow.h"
#include "glob.h"

int _new_album_while_on_command_line_mode_=0;
int _save_history_=0;
int delay=0; //used on Constant and Live Earth
int already_checked_from_main=0;
int unity_prog=0;
int timeout_count=0;
int gnome_version=3;

bool connect_to_earth=false;
bool connect_to_constant =false;
bool started_with_earth=false;
bool notification_first_time_main=true;
bool previous_clicked=false;
bool it_was_paused=false;

UnityLauncherEntry *unity_support;

QString hour_minute_photoofday_main;

QStringList previous_pictures_main;
QStringList monitored_folders_main;
QStringList folders_that_changed_main;

QFileSystemWatcher *watch_files_main = new QFileSystemWatcher;

QTimer *folder_monitor_reset = new QTimer;

QStringList pictures; //used on --constant and --once

void show_notif_that_gui_requested(){
    if(!_photo_of_day_running_){
        //if picture of the day is running, its process won't stop
        if(!Global::desktop_notify("info", 6))
            cerr << "Something went wrong while displaying a notification\n";
        cout << "Requested GUI mode! Stopping the process and starting the GUI now...\n";;
    }
}

void get_delay(){
    QSettings settings( "Wallch", "MainWindow" );
    delay=settings.value("delay", 300).toInt();
    if(delay<10 || delay>86400){
        cerr << "Invalid delay! Defaulting the delay to 5 minutes!\n";
        delay=300;
    }
    timeout_count=delay;
}

void save_statistics(){
    //saving the number of images that changed this time for statistics' use
    QSettings settings("Wallch", "Statistics");
    double images_number_already=settings.value("images_number").toDouble();
    __statistic__images_n+=images_number_already;
    settings.setValue("images_number", __statistic__images_n);
}

void check_notifications(){
    already_checked_from_main=1;
    QSettings pr( "Wallch", "Preferences" );
    if (pr.value("notification", false).toBool()) _show_notification_=1;
    else _show_notification_=0;

     if(pr.value("sound", false).toBool())
     {
         if(pr.value("custom", false).toBool()) _sound_notification_=2;
         else _sound_notification_=1;
     }
     history_type=pr.value("history").toInt();
     if (history_type) _save_history_=1;
     else _save_history_=0;
}

class Boot_Timer : public QTimer {
  Q_OBJECT
public:
  explicit Boot_Timer(QObject *parent = 0) : QTimer(parent) {

  }
public Q_SLOTS:
    void dir_changed(QString path){
        folders_that_changed_main << path;
        if(folder_monitor_reset->isActive())
            folder_monitor_reset->stop();
        folder_monitor_reset->start(1500);
    }
    void research_dirs(){
        folder_monitor_reset->stop();
        folders_that_changed_main.removeDuplicates();
        for(int i=0;i<folders_that_changed_main.count();i++){
            QString folder=folders_that_changed_main.at(i);
            int pictures_count = pictures.count();
            QFileInfo info;
            for(int i=0; i<pictures_count; i++){
                info.setFile(pictures.at(i));
                if(info.absoluteDir().absolutePath()==folder){
                    pictures.removeAt(i);
                    pictures_count--;
                    i--;
                }
            }
            get_files(folder);
        }
        if(pictures.count()<2){
            cerr << "Wallch was in folder monitoring non-GUI mode, but there are not enough pictures in the monitored folder(s).\nThe program will now exit!\n";
            exit(2);
        }
    }
    void get_files(QString path){
        //this function is for adding files of a monitored folder
        QDir dir_get_all_pictures;
        dir_get_all_pictures.setPath(path);
        dir_get_all_pictures.setFilter(QDir::Files);
        QStringList filters;
        filters << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.gif" << "*.GIF" << "*.bmp" << "*.BMP" << "*.svg" << "*.SVG";
        QStringList all_files=dir_get_all_pictures.entryList(filters);
        int all_files_count = all_files.count();
        QString temp;
        for(int i=0; i<all_files_count; i++){
          temp=path;
          temp+="/";
          temp+=all_files.at(i);
          pictures << temp;
        }
    }
    void get_pictures(bool once){
        QSettings settings( "Wallch", "MainWindow" );
        QSettings settings2( "Wallch", "Extras");
        int monitored_on=settings2.value("monitored_on", false).toBool();
        if(!monitored_on){ //monitored folders are disabled, search normally the images of the list!
            int size = settings.beginReadArray("listwidgetitem");
            for (int i = 0; i < size; ++i){
                settings.setArrayIndex(i);
                pictures << settings.value("item").toString();
            }
            if(pictures.count()<2){
                if(!Global::desktop_notify("info",3))
                    cerr << "Something went wrong while displaying a notification\n";
                exit(2);
            }
        }
        else
        {//folder monitoring is on, add all the pictures of the monitored folders!
            int size = settings2.beginReadArray("monitored_folders");
            for (int i = 0; i < size; ++i){
                settings2.setArrayIndex(i);
                if(!once){//no need to monitor anything if that's an 'once' and not 'constant'
                    cout << "Folder monitoring is ON! Tracking folders...\n";
                    connect(watch_files_main, SIGNAL(directoryChanged(QString)), this, SLOT(dir_changed(QString)));
                    connect(folder_monitor_reset, SIGNAL(timeout()), this, SLOT(research_dirs()));
                    monitored_folders_main << settings2.value("item").toString();
                    watch_files_main->addPath(settings2.value("item").toString());
                }
                get_files(settings2.value("item").toString());
            }
            settings2.endArray();
        }
    }
    void connected_to_internet(){
        if(Global::connected_to_internet()){
            if(!_photo_of_day_running_)
                continue_with_live_earth();
            else{
                QSettings settings( "Wallch", "Extras" );
                QString hm=settings.value("hour_min_photoofday", "0:0").toString();
                continue_with_pic_of_day(hm, false);
            }
        }
        else
            cerr << "No internet connection, trying again in 3 secs...\n";
    }
    void time_outer(){
        timeout_count--;
        if(unity_prog)
            Global::set_unity_value(unity_support, timeout_count, delay);
        if(!timeout_count){
            if(connect_to_earth)
                Global::livearth();
            else if(connect_to_constant)
                constant();

            timeout_count=delay;
        }
    }

    void check_pic_of_day(){
        QTime time;
        QString cur_hm=QString::number(time.currentTime().hour())+QString::number(time.currentTime().minute());
        if(cur_hm==hour_minute_photoofday_main){
            QSettings settings( "Wallch", "Extras" );
            //<-checking if this day is another of the previous day...
            QString yearmonthday=settings.value("yearmonthday", "").toString();
            QDate date;
            if(yearmonthday!=QString::number(date.currentDate().year())+QString::number(date.currentDate().month())+QString::number(date.currentDate().dayOfWeek())){
                //the previous time that photoofday changes was another day
                Global::photo_of_day_now();
            }
            else{
                /*
                the previous time was today, so the picture must be still there,
                so re-set it as background, no need to download anything!
                */
                if(!Global::set_background(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg")){
                    cerr << "Something went wrong while setting the desktop background!\n"\
                            "Probably you set the photo of the day today but you removed the original\n"\
                            "downloaded photo. If it didn't happen like this, please report this as a bug!\n"\
                            "Wallch will now attemp to download again the picture of the day.\n";
                    /*
                      Here, re-setting the configuration so as to re-download the image!
                    */
                   /* QSettings settings2( "Wallch", "Extras" );
                    settings2.setValue("previous_img_url", "retry");
                    settings2.sync();*/
                    Global::photo_of_day_now();
                    /*QSettings settings( "Wallch", "Extras" );
                    QString hm=settings.value("hour_min_photoofday", "0:0").toString();
                    continue_with_pic_of_day(hm, true);*/
                }
            }
        }
    }

    void connect_to_slot(){
        if(!_photo_of_day_running_){
            connect(this, SIGNAL(timeout()), this, SLOT(time_outer()));
            if(unity_prog){
                unity_support = unity_launcher_entry_get_for_desktop_id("wallch.desktop");
                Global::enable_unity(unity_support, true);
            }
        }
        else
        {
            connect(this, SIGNAL(timeout()), this, SLOT(check_pic_of_day()));
        }
    }

    void disconnect_from_slot(){
        disconnect(this, 0, this, 0);
    }

    void connect_to_check_internet(){
        connect(this, SIGNAL(timeout()), this, SLOT(connected_to_internet()));
    }

    void continue_with_live_earth(){
        //here we have internet connection...
        cout << "Internet connection: ON\nStarting Live Earth Wallpaper Process...\n";
        //connecting to dbus so as to receive messages....
        //now we will download an editable from us online file to see which is the online location of the live earth file
        //So, if the site is down for long or if the host close, then we will have the opportunity to change the link and point to other site/image etc.
        timeout_count=30;
        delay=30;
        this->disconnect_from_slot();
        connect_to_earth=true;
        this->connect_to_slot();
        this->dbus_connect();
        _live_earth_show_message_=2;
        Global::livearth();
        delay=1800;
        timeout_count=delay;
        this->start(1000);
    }

    void continue_with_pic_of_day(QString hour_min_main, bool check_internet_connection){
        if(check_internet_connection){
            if(!Global::connected_to_internet()){
                //will check every 3 secs if there's internet connection...
                this->disconnect_from_slot();
                this->connect_to_check_internet();
                this->start(3000);
            }
            else{
                QSettings settings ( "Wallch", "Extras" );
                QString hour_minute=settings.value("hour_min_photoofday", "0:0").toString();
                this->continue_with_pic_of_day(hour_minute, false);
            }
            return;
        }
        //here we have internet connection...
        cout << "Internet connection: ON\nStarting Picture Of The Day Process...\n";
        //now we will download an editable from us online file to see which is the online location of the live earth file
        //So, if the site is down for long or if the host close, then we will have the opportunity to change the link and point to other site/image etc.
        this->disconnect_from_slot();
        this->connect_to_slot();
        this->dbus_connect();
        _photo_of_day_show_message_=1;
        QStringList hour_min_entries;
        hour_min_entries=hour_min_main.split(":");
        hour_minute_photoofday_main=hour_min_entries.at(0);
        hour_minute_photoofday_main+=hour_min_entries.at(1);

        QSettings settings( "Wallch", "Extras" );
        settings.setValue("hour_min_photoofday",hour_min_main);
        settings.sync();
        //yearmonthday contains the last time that photo of the day was changed...
        QString yearmonthday=settings.value("yearmonthday", "").toString();
        QDate date;
        if(yearmonthday!=QString::number(date.currentDate().year())+QString::number(date.currentDate().month())+QString::number(date.currentDate().dayOfWeek())){
            //the previous time that photoofday changes was another day
            Global::photo_of_day_now();
        }
        else{
            cout << "Picture of the day has already been downloaded\n";
            if(QFile(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg").exists()){
                if(!Global::set_background(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg")){
                    cerr << "Error while trying to set as background the picture of the day!\n"
                            "Picture Of The Day will be downloaded again.\n";
                    /*
                      Here, re-setting the configuration so as to re-download the image!
                    */
                    QSettings settings( "Wallch", "Extras" );
                    settings.setValue("yearmonthday", "");
                    settings.setValue("previous_img_url", "retry");
                    settings.sync();
                    continue_with_pic_of_day(hour_min_main, true);
                }
                if(!Global::desktop_notify("info",7))
                    cerr << "Error while displaying notification!\n";
            }
            else
            {
                cerr << "Something went wrong while setting the desktop background!\n"\
                        "Probably you set the pic of the day today but you removed the original\n"\
                        "downloaded photo. If it didn't happen like this, please report this as a bug!\n"\
                        "Wallch will now attemp to download again the picture of the day.\n";
                QSettings settings( "Wallch", "Extras" );
                settings.setValue("yearmonthday", "");
                settings.setValue("previous_img_url", "retry");
                settings.sync();
                continue_with_pic_of_day(hour_min_main, true);
            }
        }
        this->start(59500);
    }

    void dbus_action(QString action){
        if(action=="FOCUS"){
            show_notif_that_gui_requested();
            if(this->isActive())
                this->stop();
            MainWindow *w= new MainWindow(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            Global::enable_unity(unity_support, false);
            w->show();
        }
        else if(action=="NEW_ALBUM"){
            _new_album_while_on_command_line_mode_=1;
            show_notif_that_gui_requested();
            if(this->isActive())
                this->stop();
            MainWindow *w= new MainWindow(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            Global::enable_unity(unity_support, false);
            w->show();
        }
        else if(action=="--earth" || action=="--earth_unity"){
            if(connect_to_earth){
                cerr << "Already running in Live Earth Wallpaper Mode, skipping...\n";
                return;
            }
            else{
                cout << "Switching to Live Earth Wallpaper mode...\n";
                connect_to_constant=false;
                connect_to_earth=true;
                if(this->isActive())
                    this->stop();
                this->disconnect_from_slot();
                this->connect_to_slot();
                _live_earth_show_message_=2;
                Global::livearth();
                delay=1800;
                timeout_count=delay;
                this->start(1000);
                Global::enable_unity(unity_support, true);
            }
        }
        else if(action=="--pause" && connect_to_constant && this->isActive()){
            this->stop();
            it_was_paused=true;
        }
        else if(action=="--picofday"){
            if(_photo_of_day_running_){
                cerr << "Already running in Picture Of The Day Mode, skipping...\n";
                return;
            }
            cout << "Switching to Picture Of The Day Mode...\n";
            connect_to_constant=connect_to_earth=0;
            if(this->isActive())
                this->stop();
            this->disconnect_from_slot();
            _photo_of_day_running_=1;
            QSettings settings ( "Wallch", "Extras" );
            QString hour_minute=settings.value("hour_min_photoofday", "0:0").toString();
            this->continue_with_pic_of_day(hour_minute, true);
            Global::enable_unity(unity_support, false);
        }
        else if(action=="--constant"){
            if(connect_to_constant){
                if(this->isActive()){
                    cerr << "Already running in Constant Mode, skipping...\n";
                    return;
                }
                else
                {
                    /*
                       if it was paused (from the --pause argument)
                       then there isn't a need to call the this->constant
                       It has to continue from where it left it.
                    */
                    if(it_was_paused)
                        it_was_paused=false;
                    else{
                        get_delay();
                        this->constant();
                    }
                    this->start(1000);
                }
            }
            else{
                cout << "Switching to Constant Mode...\n";
                connect_to_constant=true;
                connect_to_earth=false;
                if(this->isActive())
                    this->stop();
                this->disconnect_from_slot();
                this->connect_to_slot();
                if(started_with_earth){
                    started_with_earth=false;
                    get_pictures(false);
                    check_notifications();
                }
                this->constant();
                get_delay();
                this->start(1000);
                Global::enable_unity(unity_support, true);
            }
        }
        else if(action=="--once"){
            if(!connect_to_constant){
                get_pictures(true);
                check_notifications();
            }
            this->constant();
        }
        else if(action=="--stop"){
            cout << "Received 'stop' message... Terminating application...\n";
            save_statistics();
            exit(0);
        }
        else if(action=="--next"){
            if(connect_to_earth || connect_to_constant){
                if(this->isActive()){
                    cout << "Got signal 'next' and changing image...\n";
                    this->stop();
                    if(connect_to_earth){
                        Global::livearth();
                        timeout_count=delay;
                        this->start(1000);
                    }
                    else if(connect_to_constant){
                        this->constant();
                        get_delay();
                        this->start(1000);
                    }
                }
                else
                    cerr << "Got signal but no process is running...\n";
            }
            else
                cerr << "Got signal 'next' but nothing's running, skipping...\n";
        }
        else if(action=="--previous"){
            if(connect_to_earth || connect_to_constant){
                if(this->isActive()){
                    cout << "Got signal previous and changing image...\n";
                    this->stop();
                    if(connect_to_earth){
                        Global::livearth();
                        timeout_count=delay;
                        this->start(1000);
                    }
                    else if(connect_to_constant){
                        previous_clicked=true;
                        this->constant();
                        if(started_with_earth)
                            get_delay();
                        this->start(1000);
                    }
                }
                else
                    cerr << "Got signal but no process is running...";
            }
            else
                cerr << "Got signal 'previous' but nothing's running, skipping...\n";
        }
    }

    void dbus_connect(){
        QDBusConnection::sessionBus().connect(QString(),QString(), "do.action", "wallch_message", this, SLOT(dbus_action(QString)));
    }
    ////////////
  void constant(){
      if(!pictures.count()){
          cerr << "Could not get pictures or not enough pictures/no pictures at all selected!\nPlease select pictures from Wallch's main window and try again. The program will now exit.\n";
          exit(2);
      }
      if(previous_clicked && previous_pictures_main.count()>1){
          previous_clicked=false;
          QString picture=previous_pictures_main.at(previous_pictures_main.count()-2);
          previous_pictures_main.removeLast();
          if(!QFile(picture).exists() || QImage(picture).isNull()){
              cerr << "Error! Previous image not existent or invalid! Skipping...\n";
              return;
          }
          if(!Global::set_background(picture)){
              cerr << "Something went wrong while setting the picture!\n";
              return;
          }
          if(_show_notification_){
              if(!Global::desktop_notify(picture, 0))
                  cerr << "Something went wrong while displaying a notification!\n";
          }
          if(_sound_notification_)
              Global::sound_notify();
          if(_save_history_)
              Global::save_history(picture);
      }
      else
      {
          srand(time(0));
          int pic_position=rand()%pictures.count();
          FILE* pipe;
          if(gnome_version==2)
              pipe=popen("gconftool-2 --get /desktop/gnome/background/picture_filename", "r");
          else
              pipe=popen("gsettings get org.gnome.desktop.background picture-uri", "r");
          char buffer[128];
          string result = "";
          while(!feof(pipe)) {
              if(fgets(buffer, 128, pipe) != NULL)
                  result += buffer;
              }
          pclose(pipe);

          QString current_wallpaper = QString::fromStdString(result);
          if(gnome_version==2)
              current_wallpaper.chop(1);
          else
          {
              current_wallpaper.chop(2);
              current_wallpaper.remove(0,8);
          }
          int limit=0;
          while(!QFile(pictures.at(pic_position)).exists() || QImage(pictures.at(pic_position)).isNull() || pictures.at(pic_position)==current_wallpaper){
              pic_position=rand()%pictures.count(); //taking other image till find a valid one
              if(limit++==30){
                  //warning type 2
                  if(!Global::desktop_notify("info", 2))
                      cerr << "Something went wrong while displaying a notification\n";
                  save_statistics();
                  exit(2);
              }
          }
          if(!Global::set_background(pictures.at(pic_position)))
              cerr << "Something went wrong while setting the desktop background!\n";
          previous_pictures_main << pictures.at(pic_position);
          if(_show_notification_){
              if(!Global::desktop_notify(pictures.at(pic_position), 0))
                  cerr << "Something went wrong while displaying a notification\n";
          }
          if(_sound_notification_)
              Global::sound_notify();
          if(_save_history_)
              Global::save_history(pictures.at(pic_position));
      }
  }
};

Boot_Timer boottimer;

#include "main.moc"
QString othergl="/.config/Wallch/Checks";

/*PROGRAM IS STARTING HERE*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings settings( "Wallch", "Preferences" );
    int langvalue=settings.value("language", 4).toInt(); //<- default language is 4(english) have to change it in preferences.cpp too.
    unity_prog = settings.value("unity_prog").toBool();
        QTranslator *translator = new QTranslator();
        if(langvalue==0)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_su.qm");
        else if(langvalue==1)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_bs.qm");
        else if(langvalue==2)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_hr.qm");
        else if(langvalue==3)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_da.qm");
        else if(langvalue==5)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_fr.qm");
        else if(langvalue==6)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_de.qm");
        else if(langvalue==7)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_el.qm");
        else if(langvalue==8)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_he.qm");
        else if(langvalue==9)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_it.qm");
        else if(langvalue==10)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_nl.qm");
        else if(langvalue==12)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_nb.qm");
        else if(langvalue==12)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_pl.qm");
        else if(langvalue==13)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_pt.qm");
        else if(langvalue==14)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_ru.qm");
        else if(langvalue==15)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_sk.qm");
        else if(langvalue==16)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_es.qm");
        else if(langvalue==17)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_sv.qm");
        else if(langvalue==18)
            translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_tr.qm");
        a.installTranslator(translator);

    //checking to see whether the files are placed into the config or not. (the files aren't there at first-time run)
    if(!QFile(QDir::homePath()+othergl).exists()){ //this is the 1st time to run Wallch!
        if(system("mkdir -p ~/.config/Wallch/Checks; mkdir -p ~/.config/Wallch/History")){
            cerr << "FATAL Error making the configuration folders. Check the permissions of your ~/.config folder.\nThe program will now exit.\n";
            exit(2);
        }
        //setting some default values...
           settings.setValue("tray", false);
           settings.setValue("history", 2);
           settings.setValue("unity_prog", true);
           settings.sync();
    }
    //checking gnome-version
    QFile gnome_version_file("/usr/share/gnome/gnome-version.xml");
    if (!gnome_version_file.open(QIODevice::ReadOnly | QIODevice::Text))
        cerr << "Couldn't open file /usr/share/gnome/gnome-version.xml for reading, assuming gnome version is 3.\nIf this is wrong please change the read permissions of this file.\n";
    else
    {
        QXmlStreamReader xml(&gnome_version_file);
        for(!xml.atEnd(); !xml.hasError(); xml.readNext()) {
            if(xml.name().toString()=="platform"){
                gnome_version=xml.readElementText().toInt();
                break;
            }
        }
    }

    //checking the command-line arguments for the boot-options (because these won't show the gui)

    bool already_runs=Global::already_runs();
    if(QCoreApplication::arguments().count()>1){
        QString first_argument= QCoreApplication::arguments().at(1);
        if(first_argument=="--earth_w8"){
            sleep(3); //this is done in case you have chosen to let live earth running while you closed the app!
            //taking again the 'if wallch is running' after 3 secs....
            already_runs=Global::already_runs();
            if(already_runs){
                cerr << "Program started with the argument --earth_w8, but the timeout of three seconds was over and another wallch process is already running. Exiting...\n.";
                exit(2);
            }
        }
        else if(first_argument=="--picofday_w8"){
            sleep(3); //this is done in case you have chosen to pic of day running while you closed the app!
            //taking again the 'if wallch is running' after 3 secs....
            already_runs=Global::already_runs();
            if(already_runs){
                cerr << "Program started with the argument --picofday_w8, but the timeout of three seconds was over and another wallch process is already running. Exiting..\n.";
                exit(2);
            }
        }
        if(first_argument=="--picofday" || first_argument=="--picofday_w8" || first_argument=="--earth" || first_argument=="--earth_w8" || first_argument=="--earth_unity" || first_argument=="--once" || first_argument=="--constant"){
            if(already_runs){ //sending argument.... The already running process, if in GUI, will start normally doing one of the actions... otherwise, if in command line mode, it will start the GUI...
                QDBusMessage msg = QDBusMessage::createSignal("/", "do.action", "wallch_message");
                msg << first_argument;
                QDBusConnection::sessionBus().send(msg);
                exit(2);
            }
            if(QCoreApplication::arguments().count()>2){//1 is the program name, 2 is the first_argument
                cerr << QString("Error! Argument '"+first_argument+"' doesn't need any other options.\nUsage: wallch "+first_argument+"\nType wallch --help for more options\n").toLocal8Bit().data();
                exit(2);
            }
            if(first_argument=="--picofday" || first_argument=="--picofday_w8"){
                _photo_of_day_running_=1;
                boottimer.dbus_connect();
                if(!Global::connected_to_internet()){
                    //will check every 3 secs if there's internet connection...
                    boottimer.disconnect_from_slot();
                    boottimer.connect_to_check_internet();
                    boottimer.start(3000);
                }
                else{
                    QSettings settings ( "Wallch", "Extras" );
                    QString hour_minute=settings.value("hour_min_photoofday", "0:0").toString();
                    boottimer.continue_with_pic_of_day(hour_minute, true);
                }
                return a.exec();
            }
            if(first_argument=="--earth" || first_argument=="--earth_w8"){
                _live_earth_running_=1;
                started_with_earth=true;
                boottimer.dbus_connect();
                //try to ping google
                cout << "Checking for internet connection...\n";
                if(!Global::connected_to_internet()){
                    boottimer.disconnect_from_slot();
                    boottimer.connect_to_check_internet();
                    boottimer.start(3000);
                }
                else
                    boottimer.continue_with_live_earth();
                return a.exec();
            }
            else if(first_argument=="--earth_unity"){
             /*
                 this is called when the user tries a unity option. So, the user can see if he has internet or not.
                 does the check for internet connection, if there's no internet it exits immediately! the user then would connect to it and try again
                 but --earth as a startup option mainly because it doesn't close if it doesn't detect internet as --earth_unity, provided that the PC doesn't connect to the internet immediately.
             */
                _live_earth_running_=1;
                started_with_earth=true;
                //connecting to dbus so as to receive messages....
                boottimer.dbus_connect();
                cout << "Checking for internet connection...\n";
                if(!Global::connected_to_internet()){
                    cerr << "\nCould not ping google, so probably there isn't internet connection, the program will now exit...\n";
                    if(!Global::desktop_notify("info", 4))
                        cerr << "Something went wrong while displaying a notification\n";
                    exit(2);
                }
                //here we have internet connection...
                cout << "Internet connection: ON\nStarting Live Earth Wallpaper Process...\n";
                //now we will download an editable from us online file to see which is the online location of the live earth file
                //So, if the site is down for long or if the host close, then we will have the opportunity to change the link and point to other site/image etc.
                connect_to_earth=true;
                boottimer.connect_to_slot();
                _live_earth_show_message_=2;
                Global::livearth();
                delay=1800;
                timeout_count=delay;
                boottimer.start(1000);
                return a.exec();
            }
            else if(first_argument=="--constant"){
                boottimer.get_pictures(false);
                get_delay();
                cout << "Your Desktop Background will change every " << delay << " seconds.\n";
                connect_to_constant=true;
                boottimer.connect_to_slot();
                boottimer.dbus_connect();
                check_notifications();
                boottimer.constant();
                boottimer.start(1000);
                return a.exec();
            }
            else if(first_argument=="--once"){
                boottimer.get_pictures(true);
                check_notifications();
                boottimer.constant();
                save_statistics();
                exit(0);
            }
        }
        else if(first_argument=="--help" || first_argument=="-h" ){
            cout << "Usage: wallch [OPTION]\n\nWallch options\n  -h or --help Show help options.\n  --earth      Starts live earth wallpaper, updating every 30 minutes.\n  --once       Change desktop background once by picking randomly an image from the list.\n  --constant   Starts changing randomly pictures from the list, without opening the Wallch GUI.\n"
                    "\nNotes\n--once and --constant will only work if you have at least 2 images in the list.\n--earth will only work if you have internet connection.\n";
            exit(0);
        }
        else {
            MainWindow w(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            Global::enable_unity(unity_support, false);
            w.show();
            return a.exec();
        }
    }
    else
    {
        MainWindow w(previous_pictures_main);
        QApplication::setQuitOnLastWindowClosed(false);
        Global::enable_unity(unity_support, false);
        w.show();
        return a.exec();
    }
    ///
}
