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
#include <QtConcurrentRun>

#include <iostream>
#include <fstream>
#include <libnotify/notify.h>
using namespace std;

#include "mainwindow.h"
#include "glob.h"

int _new_album_while_on_command_line_mode_=0;
int delay=0; //used on --constant
int already_checked_from_main=0;

bool connect_to_earth=false;
bool connect_to_constant =false;
bool started_with_earth=false;
bool notification_first_time_main=true;
bool previous_clicked=false;
bool first_time_earth_notification_active_or_no_connection=true;

NotifyNotification* notification_main;
gboolean            success_main;
GError*             error_main = NULL;

QStringList previous_pictures_main;

QStringList pictures; //used on --constant and --once


void closed_handler (){

}

void desktop_notify(QString qimage, int warning_notification){

    QString body;
    if(warning_notification==0){
        if(!QFile(qimage).exists() || QImage(qimage).isNull())
            return;
        body=QObject::tr("Your current wallpaper has changed!");
    }
    else if(warning_notification==1)
        body=QObject::tr("There are lots of invalid images or too few from the ones you have selected. Please 'clear' your selected images and try again!");
    else if(warning_notification==2)
        body=QObject::tr("You didn't have more than 1 picture in the list!");
    else if(warning_notification==3)
        body=QObject::tr("Unable to connect") + ". " + QObject::tr("Check your internet connection!");
    else if(warning_notification==4)
        body=QObject::tr("Live Earth Wallpaper is active and it will change every 30 minutes!");
    else if(warning_notification==5)
        body=QObject::tr("Requested Graphical User Interface! Every prior process has stopped!");
    if (!notify_init ("update-notifications"))
            return;
    if(notification_first_time_main){
        notification_first_time_main=false;
        /* try the icon-summary-body case */
        if(warning_notification==0)
            notification_main = notify_notification_new ( "Wallch", body.toLocal8Bit().data(), qimage.toLocal8Bit().data(), NULL);
        else
            notification_main = notify_notification_new ( "Wallch", body.toLocal8Bit().data(), NULL, NULL);
        error_main = NULL;
        success_main = notify_notification_show (notification_main, &error_main);
        if (!success_main)
        {
                g_print ("That did not work ... \"%s\".\n", error_main->message);
                g_error_free (error_main);
        }

        g_signal_connect (G_OBJECT (notification_main), "closed", G_CALLBACK (closed_handler), NULL);
        return;
    }
    /* update the current notification with new content */
    if(warning_notification==0)
        success_main = notify_notification_update (notification_main, "Wallch", body.toLocal8Bit().data(), qimage.toLocal8Bit().data());
    else
        success_main = notify_notification_update (notification_main, "Wallch", body.toLocal8Bit().data(), NULL);
    error_main = NULL;
    success_main = notify_notification_show (notification_main, &error_main);
    if (!success_main)
    {
            g_print ("That did not work ... \"%s\".\n", error_main->message);
            g_error_free (error_main);
            return;
    }
    g_signal_connect (G_OBJECT (notification_main), "closed", G_CALLBACK (closed_handler), NULL);

}

void show_notif_that_gui_requested(){
    desktop_notify(NULL, 5);
    cout << "Requested GUI mode! Stopping the process and starting the GUI now...\n";;
}

void get_delay(){
    QFile delay_file(QDir::homePath()+"/.config/Wallch/Checks/delay");
    delay_file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in3(&delay_file);
    delay=QString(in3.readLine()).toInt();
    delay_file.close();
    if(delay<10 || delay>86400){
        cerr << "Invalid delay! Defaulting the delay to 5 minutes!\n";
        delay=300;
    }
}

void get_pictures(){
    QSettings settings( "Wallch", "MainWindow" );
    int size = settings.beginReadArray("listwidgetitem");
    for (int i = 0; i < size; ++i){
        settings.setArrayIndex(i);
        pictures << settings.value("item").toString();
    }
    if(pictures.count()<2){
        desktop_notify(NULL,2);
        exit(2);
    }
}

void sound_notify(){
    if(_sound_notification_==1){
        if(system(QString("canberra-gtk-play -f "+QString(PREFIX)+"/share/wallch/files/notification.ogg > /dev/null&").toLocal8Bit().data()))
            cerr << "Error executing canberra-gtk-play\n";
    }
    else{
        if(system("mpg321 \"$(cat ~/.config/Wallch/Checks/text_to_button)\" 2> /dev/null&"))
            cerr << "Error executing mpg321\n";
    }
}

void save_history(QString qimage){
    QString keep_history_of;
    if(history_type==1)
        keep_history_of="monthenday";
    else if(history_type==2)
        keep_history_of="monthendayentime";
    else
        keep_history_of="monthendayentimeensecs";//history_type==3
    QString qcommand="month=$(date '+%B'); year=$(date '+%Y'); keep_history_of="+keep_history_of+"; mkdir -p ~/.config/Wallch/History/\"${year} ${month}\"/; "
            "if [ X\"$keep_history_of\" = X\"monthenday\" ]; then "
            "echo \"$(date '+%a') $(date '+%d') $(date '+%h') | Image:"+qimage+"\" >> ~/.config/Wallch/History/\"${year} ${month}\"/\"$(date +'%d') $(date '+%A')\";"
            " elif [ X\"$keep_history_of\" = X\"monthendayentime\" ]; then "
            "echo \"$(date '+%a') $(date '+%d') $(date '+%h') $(date '+%H'):$(date '+%M') | Image:"+qimage+"\" >> ~/.config/Wallch/History/\"${year} ${month}\"/\"$(date +'%d') $(date '+%A')\";"
            " elif [ X\"$keep_history_of\" = X\"monthendayentimeensecs\" ]; then "
            "echo \"$(date '+%a') $(date '+%d') $(date '+%h') $(date '+%H'):$(date '+%M'):$(date '+%S') | Image:"+qimage+"\" >> ~/.config/Wallch/History/\"${year} ${month}\"/\"$(date +'%d') $(date '+%A')\";"
            "fi;";
    if(system(qcommand.toLocal8Bit().data()))
        cerr << "Error writing history!\n";
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

    bool connected_to_internet(){

        if(!system("ping -c 1 google.com > /dev/null 2> /dev/null")){
            if(this->isActive()){
                this->stop();
                this->continue_with_live_earth();
            }
            return true;
        }
        else{
            cerr << "There's no internet connection!\n";
            return false;
        }
    }

    void connect_to_slot(){
        if(connect_to_earth)
            connect(this, SIGNAL(timeout()), this, SLOT(livearth()));
        if(connect_to_constant)
            connect(this, SIGNAL(timeout()), this, SLOT(constant()));
    }

    void disconnect_from_slot(){
        disconnect(this,0,this,0);
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
        this->disconnect_from_slot();
        connect_to_earth=true;
        this->connect_to_slot();
        this->dbus_connect();
        first_time_earth_notification_active_or_no_connection=true;
        QtConcurrent::run(this, &Boot_Timer::livearth);
        this->start(1800000); //30 min
    }

    void dbus_action(QString action){
        if(action=="FOCUS"){
            show_notif_that_gui_requested();
            if(this->isActive())
                this->stop();
            MainWindow *w= new MainWindow(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            w->show();
        }
        else if(action=="NEW_ALBUM"){
            _new_album_while_on_command_line_mode_=1;
            show_notif_that_gui_requested();
            if(this->isActive())
                this->stop();
            MainWindow *w= new MainWindow(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            w->show();
        }
        else if(action=="--earth" || action=="--earth_unity"){
            if(connect_to_earth){
                cerr << "Already running in Live Earth Wallpaper Mode, skipping...\n";
                return;
            }
            else if(connect_to_constant){
                cout << "Switching from Constant to Live Earth Wallpaper mode...\n";
                connect_to_constant=false;
                connect_to_earth=true;
                if(this->isActive())
                    this->stop();
                this->disconnect_from_slot();
                this->connect_to_slot();
                first_time_earth_notification_active_or_no_connection=true;
                QtConcurrent::run(this, &Boot_Timer::livearth);
                //this->livearth();
                this->start(1800000);
            }
        }
        else if(action=="--constant"){
            if(connect_to_constant && this->isActive()){
                cerr << "Already running in Constant Mode, skipping...\n";
                return;
            }
            else if(connect_to_constant && !this->isActive()){
                get_delay();
                this->constant();
                this->start(delay*1000);
            }
            else if(connect_to_earth){
                cout << "Switching from Live Earth Wallpaper to Constant Mode...\n";
                connect_to_constant=true;
                connect_to_earth=false;
                if(this->isActive())
                    this->stop();
                this->disconnect_from_slot();
                this->connect_to_slot();
                if(started_with_earth){
                    started_with_earth=false;
                    get_pictures();
                    check_notifications();
                }
                this->constant();
                get_delay();
                this->start(delay*1000);
            }
        }
        else if(action=="--once"){
            if(connect_to_earth){
                get_pictures();
                check_notifications();
            }
            this->constant();
        }
        else if(action=="--start"){
            cout << "Received 'start' message so the program's starting...\n";
            this->stop();
            MainWindow *w= new MainWindow(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            w->show();
        }
        else if(action=="--stop"){
            cout << "Received 'stop' message... Terminating application...\n";
            exit(0);
        }
        else if(action=="--next"){
            if(connect_to_earth || connect_to_constant){
                if(this->isActive()){
                    cout << "Got signal 'next' and changing image...";
                    this->stop();
                    if(connect_to_earth){
                        this->livearth();
                        this->start(1800000);
                    }
                    else if(connect_to_constant){
                        this->constant();
                        get_delay();
                        this->start(delay*1000);
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
                    cout << "Got signal previous and changing image...";
                    this->stop();
                    if(connect_to_earth){
                        this->livearth();
                        this->start(1800000);
                    }
                    else if(connect_to_constant){
                        previous_clicked=true;
                        this->constant();
                        if(started_with_earth)
                            get_delay();
                        this->start(delay*1000);
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
  void livearth(){
      QDateTime time;
      QString filename="world"+QString::number(time.currentMSecsSinceEpoch())+".jpg";
      QString command="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+QDir::homePath()+"/.config/Wallch/"+filename+"\";";
      if(system(QString("cd ~/.config/Wallch; wget http://dl.dropbox.com/u/11379868/setupwallpaper 2> /dev/null; location=\"$(cat setupwallpaper)\"; rm -f setupwallpaper; "
             "wget -O 1"+filename+" \"$location\" 2> /dev/null; sleep 1;"
             "if [ ! -s 1"+filename+" ]; then exit 1; else "
              "   rm -f world*.jpg; mv 1"+filename+" "+filename+"; "
             "    "+command+" "
             "fi").toLocal8Bit().data())){
          if(first_time_earth_notification_active_or_no_connection){
              first_time_earth_notification_active_or_no_connection=false;
              desktop_notify(NULL,3);
          }
          cerr << "Couldn't get the image, check internet connection etc, trying again in 3 seconds...\n";
          sleep(3);
      }
      else{
          if(first_time_earth_notification_active_or_no_connection){
              first_time_earth_notification_active_or_no_connection=false;
              desktop_notify(NULL,4);
          }
          cout << "Live earth is set as your Desktop Background. Updating every 30 minutes...\n";
      }
  }

  void constant(){
      if(!pictures.count()){
          get_pictures();
          if(!pictures.count()){
              cerr << "Could not get pictures or not enough pictures/no pictures at all selected!\nPlease select pictures from Wallch's main window and try again. The program will now exit.\n";
              exit(2);
          }
      }
      QString qcommand;
      if(previous_clicked && previous_pictures_main.count()>1){
          previous_clicked=false;
          QString picture=previous_pictures_main.at(previous_pictures_main.count()-2);
          previous_pictures_main.removeLast();
          if(!QFile(picture).exists() || QImage(picture).isNull()){
              cerr << "Error! Previous image not existent or invalid! Skipping...\n";
              return;
          }
              qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+picture+"\"";
          if(_show_notification_)
              QtConcurrent::run(desktop_notify,picture, 0);
          if(_sound_notification_)
              QtConcurrent::run(sound_notify);
          if(_save_history_)
              QtConcurrent::run(save_history, picture);
      }
      else
      {
          srand(time(0));
          int pic_position=rand()%pictures.count();
          FILE* pipe = popen("gconftool-2 --get /desktop/gnome/background/picture_filename", "r");
          char buffer[128];
          string result = "";
          while(!feof(pipe)) {
              if(fgets(buffer, 128, pipe) != NULL)
                  result += buffer;
              }
          pclose(pipe);

          QString current_wallpaper = QString::fromStdString(result);
          current_wallpaper.replace(QString("\n"),QString(""));
          int limit=0;
          while(!QFile(pictures.at(pic_position)).exists() || QImage(pictures.at(pic_position)).isNull() || pictures.at(pic_position)==current_wallpaper){
              pic_position=rand()%pictures.count(); //taking other image till find a valid one
              if(limit++==30){
                  //warning type 1
                  desktop_notify(NULL, 1);
                  exit(2);
              }
          }
              qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+pictures.at(pic_position)+"\"";
          previous_pictures_main << pictures.at(pic_position);
          if(_show_notification_)
              QtConcurrent::run(desktop_notify,pictures.at(pic_position), 0);
          if(_sound_notification_)
              QtConcurrent::run(sound_notify);
          if(_save_history_)
              QtConcurrent::run(save_history, pictures.at(pic_position));
      }
      if(system(qcommand.toLocal8Bit().data()))
          cerr << "Error executing command that changes background!\n";
  }
};

Boot_Timer boottimer;

#include "main.moc"
QString othergl="/.config/Wallch/Checks";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings settings( "Wallch", "Preferences" );
             int langvalue=settings.value( "language" ).toInt();
     if (langvalue){ //0=English
         QTranslator *translator = new QTranslator();
         if(langvalue==1)
             translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_fa.qm");
         else if(langvalue==2)
             translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_de.qm");
         else if(langvalue==3)
             translator->load(QString::fromStdString(PREFIX)+"/share/wallch/translations/wallch_gr.qm");
         a.installTranslator(translator);
    }

    //checking to see whether the files are placed into the config or not. (usually the files aren't there at first-time run)
    if(!QFile(QDir::homePath()+othergl).exists()){
        if(system("mkdir -p ~/.config/Wallch/Checks; mkdir -p ~/.config/Wallch/History")){
            cerr << "FATAL Error making the configuration folders. Check the permissions of your ~/.config folder.\n The program will now exit.\n";
            exit(2);
        }
        //setting some default values...
        QSettings
           settings2( "Wallch", "Preferences" );
           settings2.setValue("tray", false);
           settings2.setValue("notification", true);
           settings2.setValue("history", 2);
           settings2.sync();
    }

    //checking the command-line arguments for the boot-options (because these won't show the gui)
    ///
    int already_runs=system("pidof wallch > ~/.config/Wallch/Checks/concurrent; launched=$(awk '{print NF-1}' ~/.config/Wallch/Checks/concurrent); rm -rf ~/.config/Wallch/Checks/concurrent; if [ $launched -ne 0 ]; then exit 1; else exit 0; fi");
    if(QCoreApplication::arguments().count()>1){
        QString first_argument= QCoreApplication::arguments().at(1);
        if(first_argument=="--earth_w8"){
            sleep(3); //this is done in case you have chose to let live earth running while you closed the app!
            //taking again the 'if wallch is running' after 3 secs....
            already_runs=system("pidof wallch > ~/.config/Wallch/Checks/concurrent; launched=$(awk '{print NF-1}' ~/.config/Wallch/Checks/concurrent); rm -rf ~/.config/Wallch/Checks/concurrent; if [ $launched -ne 0 ]; then exit 1; else exit 0; fi");
            if(already_runs){
                cerr << "Program started with the argument --earth_w8, but the timeout of three seconds was over and another wallch process is already running. Exiting..\n.";
                exit(2);
            }
        }
        if((first_argument=="--earth") || (first_argument=="--earth_w8") || (first_argument=="--earth_unity") || (first_argument=="--once") || (first_argument=="--constant")){
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
            if(first_argument=="--earth" || first_argument=="--earth_w8"){
                _live_earth_running_=1;
                started_with_earth=true;
                //try to ping google
                boottimer.dbus_connect();
                cout << "Checking for internet connection...\n";
                if(!boottimer.connected_to_internet()){
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
                //try to ping google
                boottimer.dbus_connect();
                cout << "Checking for internet connection...\n";
                if(!boottimer.connected_to_internet()){
                    cerr << "\nCould not ping google, so probably there isn't internet connection, the program will now exit...\n";
                    desktop_notify(NULL,3);
                    exit(2);
                }
                //here we have internet connection...
                cout << "Internet connection: ON\nStarting Live Earth Wallpaper Process...\n";
                //connecting to dbus so as to receive messages....
                //now we will download an editable from us online file to see which is the online location of the live earth file
                //So, if the site is down for long or if the host close, then we will have the opportunity to change the link and point to other site/image etc.
                connect_to_earth=true;
                boottimer.connect_to_slot();
                first_time_earth_notification_active_or_no_connection=true;
                boottimer.livearth();
                boottimer.start(1800000); //30 min
                return a.exec();
            }
            else if(first_argument=="--constant"){
                get_pictures();
                get_delay();
                cout << "Your Desktop Background will change every " << delay << " seconds.\n";
                connect_to_constant=true;
                boottimer.connect_to_slot();
                boottimer.dbus_connect();
                check_notifications();
                boottimer.constant();
                boottimer.start(delay*1000);
                return a.exec();
            }
            else if(first_argument=="--once"){
                cout << "Your Desktop Background has been changed.\n";
                get_pictures();
                check_notifications();
                boottimer.constant();
                exit(0);
            }
        }
        else {
            MainWindow w(previous_pictures_main);
            QApplication::setQuitOnLastWindowClosed(false);
            w.show();
            return a.exec();
        }
    }
    else
    {
        MainWindow w(previous_pictures_main);
        QApplication::setQuitOnLastWindowClosed(false);
        w.show();
        return a.exec();
    }
    ///
}
