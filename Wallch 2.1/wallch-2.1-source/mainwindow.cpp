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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "preferences.h"
#include "screenshot.h"
#include "about.h"
#include "properties.h"
#include "statistics.h"
#include "history.h"
#include "extras.h"
#include "QOpenCVWidget.h"
#include "MyCameraWindow.h"
#include "glob.h"

#include <sys/stat.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <libnotify/notify.h>
#include <fstream>
#include <iostream>

#include <QMessageBox>
#include <QClipboard>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusMessage>
#include <QUrl>
#include <QTransform>
#include <QDrag>
#include <QProgressDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QAction>
#include <QSettings>
#include <QRect>
#include <QDesktopWidget>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QMenu>
#include <QShortcut>
#include <QListWidgetItem>
#include <QListWidget>
#include <QWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QtConcurrentRun>

using namespace std;

int prop=0;
int _start_running_=0;
int _live_earth_running_=0;
int _on_boot_once_running_;
int _on_boot_constant_running_;
int _show_notification_=0;
int _sound_notification_=0;
int _save_history_=0;
int start_normal_count=0;
int seconds_left=0;
int random_first=0;
int __statistic__images_n=0;
int __statistic__etimer_elapsed=0;
int number=0, tmp_number=0;
int seconds_minutes_hours=0;
int tmp_seconds_minutes_hours=0;
int camera_open=0;
int camera_success=1;
int history_type=0;
int version=21; //2.1

int pref=0;
int add=0;
int show_notification;
int minimize_to_tray=0;
int properties_times=0;
int close_minimize_app=0;
int propertieswindow=0;

bool paused=false;
bool preview=false;
bool histor=false;
bool extras_shown=false;
bool screenshot_shown=false;
bool about_shown=false;
bool statistics_shown=false;
bool start=true;
bool start_just_clicked=false;
bool pass_them_as_albums=true;
bool notification_first_time=true;

NotifyNotification* notification;
gboolean            success;
GError*             error = NULL;

QStringList previous_pictures;

MainWindow::MainWindow(QStringList previous_pictures_from_main, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //after all the basic checks, setting up the UI
    ui->setupUi(this);

    //      ***ARGUMENTS***
    int already_runs=system("pidof wallch > ~/.config/Wallch/Checks/concurrent; launched=$(awk '{print NF-1}' ~/.config/Wallch/Checks/concurrent); rm -rf ~/.config/Wallch/Checks/concurrent; if [ $launched -ne 0 ]; then exit 1; else exit 0; fi");
    QDBusMessage msg = QDBusMessage::createSignal("/", "do.action", "wallch_message");
    if(QCoreApplication::arguments().count()>1){
        QString first_argument= QCoreApplication::arguments().at(1);
        if(first_argument=="--earth" || first_argument=="--earth_w8" || first_argument=="--earth_unity" || first_argument=="--once" || first_argument=="--constant"){//these have been checked in main.cpp
                   _live_earth_running_=0;
        }
        else
        {
            //Error handling
        if((first_argument=="--start") || (first_argument=="--pause") || (first_argument=="--stop") || (first_argument=="--next") || (first_argument=="--previous") || (first_argument=="--help") || (first_argument=="-h")){
            if(QCoreApplication::arguments().count()>2){//1 is the program name, 2 is the first_argument
                cerr << QString("Error! Argument '"+first_argument+"' doesn't need any other options.\nUsage: wallch "+first_argument+"\nType wallch -h or --help for more options\n").toLocal8Bit().data();
                exit(2);
            }
        }
        else if(first_argument.right(7)!=".wallch"){
            cerr << QString("Invalid option: '"+first_argument+"'\nType wallch -h or --help for all available options\n").toLocal8Bit().data();
            exit(2);
        }

        ///
        //normall argument processing
        if(first_argument=="--help" || first_argument=="-h" ){
            cout << "Usage: wallch [OPTION...]\n\nWallch options\n  -h or --help Show help options.\n  --earth      Starts live earth wallpaper, updating every 30 minutes.\n  --once       Change desktop background once by picking randomly an image from the list.\n  --constant   Starts changing randomly pictures from the list, without opening the Wallch GUI.\n"
                    "\nNotes\n--once and --constant will only work if you have at least 2 images in the list.\n--earth will only work if you have internet connection.\n";
            exit(0);
        }
        else if(((first_argument=="--start") || (first_argument=="--pause") || (first_argument=="--stop") || (first_argument=="--next") || (first_argument=="--previous") || (first_argument=="--earth") || (first_argument=="--once") || (first_argument=="--constant")) && already_runs)
            msg << first_argument;
        else if(first_argument=="--start" && !already_runs)
            pass_them_as_albums=false;
        else if(first_argument!="--earth" || first_argument!="--once" || first_argument!="--constant")
        {//wallch album for sure
            if(already_runs){
                QString qnew_albums=QDir::homePath()+"/.config/Wallch/Checks/new_albums";
                QFile filenew_albums( qnew_albums );
                bool at_least_one_ok=false;
                if( filenew_albums.open( QIODevice::WriteOnly ) ) {
                    QTextStream textStream( &filenew_albums );
                    for(int i=1;i<QCoreApplication::arguments().count();i++){ //loop between the albums if more than 1 specified
                        if(QFile(QCoreApplication::arguments().at(i)).exists() && QCoreApplication::arguments().at(i).right(7)==".wallch"){
                            textStream << QCoreApplication::arguments().at(i);
                            textStream << '\n';
                            at_least_one_ok=true;
                        }
                        else
                            cerr << QString("Error! The file "+QCoreApplication::arguments().at(i)+" doesn't exist or it isn't a wallch file, skipping...").toLocal8Bit().data();
                    }
                    filenew_albums.close();
                }
                if(at_least_one_ok)
                    msg << "NEW_ALBUM"; //new album has been opened while the program is running, so just add the images to the already running program!
                else{
                    cerr << "Error, invalid album(s) specified. Please include the whole path.";
                    exit(2);
                }
            }
            else{
                for(int i=1;i<QCoreApplication::arguments().count();i++){ //loop between the albums if more than 1 specified
                    if(QFile(QCoreApplication::arguments().at(i)).exists() && QCoreApplication::arguments().at(i).right(7)==".wallch"){
                        QString qpath = QCoreApplication::arguments().at(i);
                        FILE *file = fopen ( qpath.toUtf8().data(), "r" );
                        if ( file != NULL )
                        {
                            ifstream file(qpath.toUtf8().data());
                            string line ;
                            QString qstr;
                            while(getline(file,line))
                            {
                                QListWidgetItem *item = new QListWidgetItem;
                                qstr = QString::fromUtf8(line.c_str());
                                item->setText(qstr);
                                item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                                ui->listWidget->addItem(item);
                            }
                        }
                        fclose ( file );
                    }
                }
            }
        }

    }
    }
    else if(already_runs){
        msg << "FOCUS"; //simpy another instance, send focus dbus message
    }
    if(msg.arguments().count()==1){
        QDBusConnection::sessionBus().send(msg);
        exit(0);
    }
   //listening to DBUS for another wallch instance/or to open a new album/or to do some action
   QDBusConnection::sessionBus().connect(QString(),QString(), "do.action", "wallch_message", this, SLOT(dbus_action(QString)));

    //Putting the values back
    previous_pictures=previous_pictures_from_main;
    QSettings settings2( "Wallch", "Preferences" );
    minimize_to_tray=settings2.value("tray", false).toBool();
    history_type = settings2.value("history").toInt();

    QSettings settings( "Wallch", "MainWindow" );
    int size = settings.beginReadArray("listwidgetitem");
    for (int i = 0; i < size; ++i){
        settings.setArrayIndex(i);
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(settings.value("item").toString());
        item->setStatusTip(tr("Double-click to set an item from the list as Background"));
        ui->listWidget->addItem(item);
    }
    settings.endArray();
    QFile timeslide ( QDir::homePath()+"/.config/Wallch/MainWindow.conf" );
    if (timeslide.exists()) ui->timerSlider->setValue(settings.value("timeSlider").toInt());
    ui->preview->setChecked(settings.value("preview", true).toBool());
    ui->checkBox->setChecked(settings.value("random_images", false).toBool());
    ui->checkBox_2->setChecked(settings.value("random_time", false).toBool());

    if(ui->checkBox_2->isChecked()){ ui->timerSlider->setEnabled(false); ui->label_10->setEnabled(false); ui->label_2->setEnabled(false);}
    if(ui->preview->isChecked()) preview=true;

    //Setting up the timer
    update_time = new QTimer(this);
    connect(update_time, SIGNAL(timeout()), this, SLOT(time_updater()));

       if (settings2.value("notification", false).toBool()) _show_notification_=1;
       else _show_notification_=0;

        if(settings2.value("sound", false).toBool())
        {
            if(settings2.value("custom", false).toBool()) _sound_notification_=2;
            else _sound_notification_=1;
        }

        if (settings2.value("checknone", false).toBool()) _save_history_=0;
        else _save_history_=1;

       //Setting up the tray  Icon.
       trayIcon = new QSystemTrayIcon(this);
       trayIcon->setIcon(QIcon(":/icons/Pictures/wallch.png"));
       trayIcon->setToolTip(QString("<table width='280'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>%1%2%3%4%5%6").arg(tr("Left click: Show window if process is not running/Next image if process is running.")).arg("</td></tr> <tr><td nowrap align='left'>").arg(tr("Middle click: Exit the application.")).arg("</td> </tr> <tr><td nowrap align='left'>").arg(tr("Right click: Show some options.")).arg("</td> </tr></table>"));

       connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(clickSysTrayIcon(QSystemTrayIcon::ActivationReason)));
       connect(this,SIGNAL(minimized()),this,SLOT(hide()),Qt::QueuedConnection);

       QMenu *changer_menu = new QMenu;
       Show_action = new QAction(tr("S&how"),this);
       Show_action->setIconVisibleInMenu(true);
       connect(Show_action, SIGNAL(triggered()), this, SLOT(showClicked()));
       changer_menu->addAction(Show_action);
       changer_menu->addSeparator();

       Start_action = new QAction(QIcon::fromTheme("media-playback-stop"), tr("&Start"), this);
       Start_action->setIconVisibleInMenu(true);
       connect(Start_action, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
       changer_menu->addAction(Start_action);

       Stop_action = new QAction(QIcon::fromTheme("media-playback-stop"), tr("&Stop"), this);
       Stop_action->setIconVisibleInMenu(true);
       connect(Stop_action, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked()));
       changer_menu->addAction(Stop_action);

       Next_action = new QAction(QIcon::fromTheme("media-seek-forward"), tr("&Next Image"), this);
       Next_action->setIconVisibleInMenu(true);
       connect(Next_action, SIGNAL(triggered()), this, SLOT(on_nextButton_clicked()));
       changer_menu->addAction(Next_action);

       Previous_action = new QAction(QIcon::fromTheme("media-seek-backward"), tr("P&revious image"), this);
       Previous_action->setIconVisibleInMenu(true);
       connect(Previous_action, SIGNAL(triggered()), this, SLOT(on_previousButton_clicked()));
       changer_menu->addAction(Previous_action);
       changer_menu->addSeparator();

       Settings_action = new QAction(tr("P&references"),this);
       Settings_action->setIconVisibleInMenu(true);
       connect(Settings_action, SIGNAL(triggered()), this, SLOT(ShowPreferences()));
       changer_menu->addAction(Settings_action);

       About_action = new QAction(tr("&About"), this);

       About_action->setIconVisibleInMenu(true);;
       connect(About_action, SIGNAL(triggered()), this, SLOT(menushowabout()));
       changer_menu->addAction(About_action);
       changer_menu->addSeparator();

       Quit_action = new QAction(tr("&Quit"), this);
       Quit_action->setIconVisibleInMenu(true);;
       connect(Quit_action, SIGNAL(triggered()), this, SLOT(close_minimize()));
       changer_menu->addAction(Quit_action);

       trayIcon->setContextMenu(changer_menu);

   //SIGNAL-SLOT action here
   connect(ui->action_About, SIGNAL(triggered()), this, SLOT(menushowabout()));
   connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(ShowPreferences()));
   connect(ui->actionExtras, SIGNAL(triggered()), this, SLOT(ShowExtras()));
   connect(ui->action_Check_For_Updates, SIGNAL(triggered()), this, SLOT(checkForUpdate()));
   connect(ui->actionQuit_Ctrl_Q, SIGNAL(triggered()), this, SLOT(close_minimize()));
   connect(ui->action_Start, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
   connect(ui->actionS_top, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked()));
   connect(ui->action_Load, SIGNAL(triggered()), this, SLOT(load()));
   connect(ui->actionRemove_list, SIGNAL(triggered()), this, SLOT(remove_list()));
   connect(ui->action_Previous_Image_Shift_Ctrl_B, SIGNAL(triggered()), this, SLOT(on_previousButton_clicked()));
   connect(ui->action_Next_Image, SIGNAL(triggered()), this, SLOT(on_nextButton_clicked()));
   connect(ui->actionReport_A_Bug, SIGNAL(triggered()), this, SLOT(bug()));
   connect(ui->actionGet_Help_Online, SIGNAL(triggered()), this, SLOT(Get_Help_Online()));
   connect(ui->actionAdd_single_images, SIGNAL(triggered()), this, SLOT(on_addButton_clicked()));
   connect(ui->actionAdd_Folder, SIGNAL(triggered()), this, SLOT(on_addfolder_clicked()));
   connect(ui->save_as, SIGNAL(triggered()), this, SLOT(save_album()));

//setting up the shortcut keys!
(void) new QShortcut(Qt::CTRL + Qt::Key_Q, this, SLOT(close_minimize()));
(void) new QShortcut(Qt::CTRL + Qt::Key_P, this, SLOT(ShowPreferences()));
(void) new QShortcut(Qt::CTRL + Qt::Key_E, this, SLOT(ShowExtras()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_S, this, SLOT(on_startButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_O, this, SLOT(on_stopButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N, this, SLOT(on_nextButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::Key_U, this, SLOT(checkForUpdate()));
(void) new QShortcut(Qt::CTRL + Qt::Key_O, this, SLOT(load()));
(void) new QShortcut(Qt::CTRL + Qt::Key_I, this, SLOT(on_addButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(on_addfolder_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::Key_H, this, SLOT(on_actionHistory_triggered()));
(void) new QShortcut(Qt::Key_Delete, this, SLOT(on_removeButton_clicked()));
(void) new QShortcut(Qt::Key_Return, this, SLOT(on_listWidget_itemDoubleClicked()));
(void) new QShortcut(Qt::Key_F1, this, SLOT(on_actionContents_triggered()));
(void) new QShortcut(Qt::ALT + Qt::Key_Return, this, SLOT(showProperties()));
(void) new QShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_P, this, SLOT(on_previousButton_clicked()));

if(ui->listWidget->count()<=1)
    disable_start_buttons();

    //checking to see if constant_on_boot is running, and if so, disable the start button and enable the stop in order to stop it
    if(!system("pidof -x constant_on_boot > /dev/null")){
        ui->time_for_next->show(); enable_stop_buttons(); disable_previous_and_next_buttons(); ui->checkBox->setEnabled(false); ui->checkBox_2->setEnabled(false); ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false);
        ui->actionAdd_Folder->setEnabled(false);  ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false); disable_start_buttons();
        //ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false);
        }
    else ui->time_for_next->hide();

ui->time_for_next->setValue(100);
ui->time_for_next->setFormat(tr("Calculating"));
//making the statistic's timer...
statistic_timer = new QTimer(this);
connect(statistic_timer, SIGNAL(timeout()), this, SLOT(add_to_stats()));
//adding one for the times launched used for statistics as well
double times_launched=0;
QSettings settings3 ( "Wallch", "Statistics");
times_launched=settings3.value("times_launched").toDouble();
times_launched++;
settings3.setValue("times_launched", times_launched);


//Moving the window to the center of screen!
QRect rect = QApplication::desktop()->availableGeometry();
this->move(rect.center() - this->rect().center());
//enabling dragandrop at the listwidget
setAcceptDrops(true);
// SetMouseTracking to true so the items of the listwidget can have statusbar
ui->listWidget->setMouseTracking(1);
// Set the add icon at the menubar visible
ui->menuAdd_Files->menuAction()->setIconVisibleInMenu(1);
//Disabling the Next button (nothing is running, so!?)
disable_stop_buttons();
disable_previous_and_next_buttons();
statistic_timer->start(1000);

ui->startButton->setIcon(QIcon::fromTheme("media-playback-start"));
ui->action_Start->setIcon(QIcon::fromTheme("media-playback-start"));
ui->stopButton->setIcon(QIcon::fromTheme("media-playback-stop"));
ui->actionS_top->setIcon(QIcon::fromTheme("media-playback-stop"));
ui->previousButton->setIcon(QIcon::fromTheme("media-seek-backward"));
ui->action_Previous_Image_Shift_Ctrl_B->setIcon(QIcon::fromTheme("media-seek-backward"));
ui->nextButton->setIcon(QIcon::fromTheme("media-seek-forward"));
ui->action_Next_Image->setIcon(QIcon::fromTheme("media-seek-forward"));
ui->removeButton->setIcon(QIcon::fromTheme("list-remove"));
ui->removeallButton->setIcon(QIcon::fromTheme("edit-delete"));
ui->addfolder->setIcon(QIcon::fromTheme("folder-new"));
ui->actionAdd_Folder->setIcon(QIcon::fromTheme("folder-new"));
ui->addButton->setIcon(QIcon::fromTheme("list-add"));
ui->moveupButton->setIcon(QIcon::fromTheme("go-up"));
ui->movedownButton->setIcon(QIcon::fromTheme("go-down"));
ui->webcamButton->setIcon(QIcon::fromTheme("camera-web"));
ui->screenshotButton->setIcon(QIcon::fromTheme("camera-photo"));
ui->actionQuit_Ctrl_Q->setIcon(QIcon::fromTheme("application-exit"));
ui->actionHistory->setIcon(QIcon::fromTheme("history"));
ui->action_Preferences->setIcon(QIcon::fromTheme("preferences-desktop"));
ui->actionAdd_single_images->setIcon(QIcon::fromTheme("insert-image"));
ui->actionHistory->setIcon(QIcon::fromTheme("face-smile"));
ui->actionAdd_a_Wallch_album->setIcon(QIcon::fromTheme("list-add"));
ui->action_Start->setIcon(QIcon::fromTheme("media-playback-start"));
Start_action->setIcon(QIcon::fromTheme("media-playback-start"));
Stop_action->setIcon(QIcon::fromTheme("media-playback-stop"));
Previous_action->setIcon(QIcon::fromTheme("media-seek-backward"));
Next_action->setIcon(QIcon::fromTheme("media-seek-forward"));
ui->action_Load->setIcon(QIcon::fromTheme("folder"));
ui->actionExtras->setIcon(QIcon::fromTheme("applications-accessories"));

on_timerSlider_valueChanged(ui->timerSlider->value());
if(_new_album_while_on_command_line_mode_)
    add_new_album();

if ( ui->listWidget->count() < 3 ) ui->save_as->setEnabled(false);
}

void MainWindow::closeEvent( QCloseEvent * )
{
    if(screenshot_shown) Screenshot->close();
    if(camera_open)  mainWin->close();
    if (!minimize_to_tray){
    if(system("echo > ~/.config/Wallch/MainWindow.conf"))
        cerr << "Error writing to ~/.config/Wallch/MainWindow.conf. Please check file's permissions.\n";

    QApplication::setQuitOnLastWindowClosed(1);
    QSettings //Saving the settings to the configuration file.
       settings( "Wallch", "MainWindow" );
    settings.beginWriteArray("listwidgetitem");
    for (int i = 0; i < ui->listWidget->count(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("item", ui->listWidget->item(i)->text() );
    }
    settings.endArray();
       settings.setValue( "timeSlider", ui->timerSlider->value());
       settings.setValue("random_images", ui->checkBox->isChecked());
       settings.setValue("random_time", ui->checkBox_2->isChecked());
       settings.setValue("preview", ui->preview->isChecked());
       settings.sync();

       //checking to see if the earth wallpaper is activated
       if(_live_earth_running_){
       QMessageBox msgBox2; QPushButton *Nostop; msgBox2.setWindowTitle(tr("Live Earth Wallpaper"));msgBox2.setInformativeText(tr("Do you want to stop the process or let it run?"));msgBox2.setText(tr("<b>Live Earth Wallpaper is running.</b>"));msgBox2.addButton(tr("Stop the process"), QMessageBox::ActionRole);Nostop = msgBox2.addButton(tr("Let it run"), QMessageBox::ActionRole);msgBox2.setIconPixmap(QIcon::fromTheme("info").pixmap(QSize(60,60)));msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox2.exec();
       if (msgBox2.clickedButton() == Nostop){
           if(system("wallch --earth_w8& > /dev/null 2> /dev/null"))
               cerr << "Could not possibly start wallch in Live Earth Mode...! The program will now exit manually\n";
       }
       }
       //Writing the delay to the delay file. This is done in the on_startbutton_clicked() normally but it is needed here because
       //of the constant_on_boot script (to take the time that existed on close, and not in the last on_start_button_clicked()
       ofstream newFile;
       newFile.open(QString(QDir::homePath()+"/.config/Wallch/Checks/delay").toLocal8Bit().data());
       if (seconds_minutes_hours==1)  newFile << number;
       if (seconds_minutes_hours==60)  newFile << number*60;
       if (seconds_minutes_hours==3600)  newFile << number*3600;
       newFile.close();
       double images_number_already=0, uptime_already=0;
       QSettings settings3("Wallch", "Statistics");
       uptime_already=settings3.value("time_number").toDouble();
       images_number_already=settings3.value("images_number").toDouble();
       __statistic__images_n+=images_number_already;
       __statistic__etimer_elapsed+=uptime_already;
       settings3.setValue("time_number", __statistic__etimer_elapsed);
       settings3.setValue("images_number", __statistic__images_n);
   }
    else {
        trayIcon->show();
        Q_EMIT minimized();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    if(pref || add || histor)
        MainWindow::showNormal();
    if (!minimize_to_tray){ //Means that minimized isn't selected.
        QMainWindow::changeEvent(e);
        switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
        }
    }
    else{ //Means that minimized is selected

    switch (e->type()){
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    //see if the event is changing the window state

    case QEvent::WindowStateChange:
        //if it is, we need to see that this event is minimize

        if(isMinimized()){
            trayIcon->show();
            Q_EMIT minimized();
        }

    default:
        break;

        QMainWindow::changeEvent(e); }}
}

void MainWindow::clickSysTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
    //reason is a variable that holds the type of activation or click done on the icon tray
    switch (reason) {
    case QSystemTrayIcon::Trigger: //if it's a normal click
        if(ui->nextButton->isEnabled()){ //change image
        on_nextButton_clicked();
        return;
        }
        if(!pref && !histor){//hide the icon
    trayIcon->hide();
    //show the main window
    this->showNormal();
}
    break;
    case QSystemTrayIcon::MiddleClick:
    //quit the application
    close();
    break;
    case QSystemTrayIcon::Unknown:
    break;
    case QSystemTrayIcon::Context:
    break;
    default :
            ;
    }
}

void MainWindow::showClicked()
{
    this->showNormal();
    trayIcon->hide();
}

void MainWindow::livearth(){//try to ping google
    if(extras_shown)
    Q_EMIT fix_livearth_buttons();
    int online = system("ping -c 1 google.com > /dev/null");
        if ( online != 0 ){ //Unable to reach google
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Live Earth Wallpaper"));
            msgBox.setText("<b>"+tr("Unable to connect")+".</b><br>"+tr("Check your internet connection!"));
            msgBox.setIconPixmap(QIcon::fromTheme("info").pixmap(QSize(100,100)));
                msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
                msgBox.exec();
                return;
        }
        earth_timer = new QTimer(this);
        connect(earth_timer,SIGNAL(timeout()),this,SLOT(start_livearth_extras_concurrent_thread()));
        _live_earth_running_=1;
        QtConcurrent::run(this, &MainWindow::livearth_extras);
        earth_timer->start(1800000);
        QString image="";
        QtConcurrent::run(this, &MainWindow::desktop_notify, image, 1);
}

void MainWindow::start_livearth_extras_concurrent_thread(){
    QtConcurrent::run(this, &MainWindow::livearth_extras);
}

void MainWindow::stop_livearth(){
    if(earth_timer->isActive())
        earth_timer->stop();
}

void MainWindow::livearth_extras(){
    QDateTime time;
    QString filename="world"+QString::number(time.currentMSecsSinceEpoch())+".jpg";
    QString command="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+QDir::homePath()+"/.config/Wallch/"+filename+"\";";
    if(system(QString("cd ~/.config/Wallch; wget http://dl.dropbox.com/u/11379868/setupwallpaper 2> /dev/null; location=\"$(cat setupwallpaper)\"; rm -f setupwallpaper; "
           "wget -O 1"+filename+" \"$location\" 2> /dev/null;"
           "if [ ! -s 1"+filename+" ]; then exit 1; else "
           "    rm -f world*.jpg; mv 1"+filename+" "+filename+"; "
           "    "+command+" "
           "fi").toLocal8Bit().data())){
        cerr << "Couldn't get the image, check internet connection etc, trying again in 3 seconds...\n";
        sleep(3);
    }
    else
        cout << "Live earth is set as your Desktop Background. Updating in 30 minutes...\n";
}

void MainWindow::checkForUpdate(){
    if(system("ping -c 1 www.google.com > /dev/null")){
        qApp->processEvents();
                    QMessageBox *msgBox = new QMessageBox;
                    msgBox->setWindowTitle(tr("Update"));
                    msgBox->setText(tr("There was an error while trying to<br>connect to the server.<br>Please check your internet connection<br> or try again in some minutes."));
                    msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80)));
                    msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
                    msgBox->setWindowModality(Qt::NonModal);
                    msgBox->show();
                    msgBox->exec();
                    return;
                }
                if(system("cd ~/.config/Wallch/; wget https://dl.dropbox.com/u/11379868/1 2> /dev/null")){
                    QMessageBox *msgBox = new QMessageBox;msgBox->setWindowTitle(tr("Update"));msgBox->setText(tr("There was an error while trying to<br>connect to the server.<br>Please check your internet connection<br> or try again in some minutes."));msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80))); msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox->setWindowModality(Qt::NonModal);
                    msgBox->exec();
                    return;
                }
                QString update = QDir::homePath()+"/.config/Wallch/1";
                QString last_version;
                if(!QFile(update).exists()) {  //file doesn't exist, an error occured.
                    QMessageBox *msgBox = new QMessageBox;msgBox->setWindowTitle(tr("Update"));msgBox->setText(tr("Unknown error, try again in some minutes."));msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80))); msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox->setWindowModality(Qt::NonModal);
                    msgBox->exec();
                    return;
                }
                QFile update_file(update);
                update_file.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream in(&update_file);
                last_version=QString(in.readLine());
                update_file.close();
                last_version.replace(QString("\n"),QString(""));
                last_version.replace(QString("."),QString(""));
                int latest_version=last_version.toInt();
                remove(update.toLocal8Bit().data());
                if(latest_version<=version){
                   QMessageBox *msgBox = new QMessageBox; msgBox->setWindowTitle(tr("Update"));msgBox->setText(tr("The current version of this program<br>is the last version available.<br>So, your program is up to date!"));msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80)));msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox->setWindowModality(Qt::NonModal);
                   msgBox->exec();
                   return;
                }
                //latest_version>version so new is available
                QString demical=last_version.right(1);
                last_version.chop(1);
                last_version.append(QString("."));
                last_version.append(demical);
                QMessageBox *msgBox = new QMessageBox;
                msgBox->setWindowTitle(tr("Update"));
                msgBox->setInformativeText(tr("A new version of this program is available! Download the new version?"));
                msgBox->setText("<b>"+tr("Version")+" " + last_version + " "+tr("is available!")+"\n</b>");
                msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(100,100)));
                msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
                QPushButton *updateButton;
                updateButton = msgBox->addButton(tr("Yes, update my app!"), QMessageBox::ActionRole);
                msgBox->addButton(tr("No, I'll stay with the old one"), QMessageBox::ActionRole);
                msgBox->setWindowModality(Qt::NonModal);


                msgBox->exec();
                if (msgBox->clickedButton() == updateButton)
                {
                    if(system("xdg-open http://wallch.t35.com/"))
                        cerr << "Error opening http://wallch.t35.com/\n";
                }
}

void MainWindow::on_addButton_clicked()
{
    if (!ui->addButton->isEnabled())
        return;
    if (minimize_to_tray)
        add=1;
    QStringList path;
    path = QFileDialog::getOpenFileNames(this, tr("Choose Pictures"), QDir::homePath(), "*png *gif *bmp *jpg *jpeg");
    add=0;
    if(path.count() == 0) return;
    for(int count=0; count < path.count(); count++){
        QString qstr = path[count];
                   QListWidgetItem *item = new QListWidgetItem;
                   item->setText(qstr);
                   item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                   ui->listWidget->addItem(item);
    }

    if(ui->listWidget->count()<=1)
        disable_start_buttons();
    else
        enable_start_buttons();

    if ( ui->listWidget->count() >= 3 ) ui->save_as->setEnabled(true);
}

void MainWindow::on_removeButton_clicked()
{
     if (ui->listWidget->count()==1) ui->listWidget->clear(); else delete ui->listWidget->currentItem();
     if ( ui->listWidget->count() < 2 ){
         disable_start_buttons();
         if ( ui->listWidget->count() == 0) ui->label_9->clear();
     }
     if ( ui->listWidget->count() < 3 ) ui->save_as->setEnabled(false);

}

void MainWindow::on_removeallButton_clicked()
{
    disable_start_buttons();
    disable_stop_buttons();
    ui->save_as->setEnabled(false);
    ui->listWidget->clear();
    ui->label_9->clear();
}

void MainWindow::on_moveupButton_clicked()
{
    if(ui->listWidget->count()>=2 && ui->listWidget->selectionModel()->hasSelection() && ui->listWidget->currentRow()!=0){
        QString a=ui->listWidget->currentItem()->text();
        ui->listWidget->item(ui->listWidget->currentRow())->setText(ui->listWidget->item(ui->listWidget->currentRow()-1)->text());
        ui->listWidget->item(ui->listWidget->currentRow()-1)->setText(a);
        QModelIndex indexbelow = ui->listWidget->model()->index( ui->listWidget->currentIndex().row()-1, 0, QModelIndex() );
        ui->listWidget->setCurrentIndex(indexbelow);
    }
}

void MainWindow::on_movedownButton_clicked()
{
    if(ui->listWidget->count()>=2 && ui->listWidget->selectionModel()->hasSelection() && ui->listWidget->currentRow()+1!=ui->listWidget->count()){
        QString a=ui->listWidget->currentItem()->text();
        ui->listWidget->item(ui->listWidget->currentRow())->setText(ui->listWidget->item(ui->listWidget->currentRow()+1)->text());
        ui->listWidget->item(ui->listWidget->currentRow()+1)->setText(a);
        QModelIndex indexbelow = ui->listWidget->model()->index( ui->listWidget->currentIndex().row()+1, 0, QModelIndex() );
        ui->listWidget->setCurrentIndex(indexbelow);
    }
}

void MainWindow::on_listWidget_itemSelectionChanged()
{
    if(preview){
        QString filename = QString::fromUtf8(ui->listWidget->item(ui->listWidget->currentRow())->text().toUtf8());
        ui->label_9->setBackgroundRole(QPalette::Base);
        QImage image(filename);
        ui->label_9->setPixmap(QPixmap::fromImage(image));
    }
}

void MainWindow::on_listWidget_itemDoubleClicked()
{
    if(_live_earth_running_){
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Live Earth Wallpaper"));
        msgBox.setText(tr("Live Earth Wallpaper is running, if you want to continue, stop the process!"));
        msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec(); return;
    }
    if (ui->startButton->text()==tr("&Start") || ui->listWidget->count() == 1)
    {
        if(!QFile(ui->listWidget->currentItem()->text()).exists() || QImage(ui->listWidget->currentItem()->text()).isNull()) return;
        QString qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+ui->listWidget->currentItem()->text()+"\"&";
        __statistic__images_n++;
        previous_pictures << ui->listWidget->currentItem()->text();
        if(system(qcommand.toLocal8Bit().data()))
            cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
        if(_show_notification_)
            QtConcurrent::run(this, &MainWindow::desktop_notify,ui->listWidget->currentItem()->text(), 0);
        if(_sound_notification_)
            sound_notify();
        if(_save_history_)
            QtConcurrent::run(this, &MainWindow::save_history, ui->listWidget->currentItem()->text());

    }
    else
        errorstopprocess();
}

void MainWindow::on_screenshotButton_clicked()
{
    if(screenshot_shown){ //screenshot dialog is running so point to it
        Screenshot->raise();
        Screenshot->activateWindow();
        return;
    }
    screenshot_shown=true; //Tell that screenshot starts running
    Screenshot = new screenshot(this);
    connect(Screenshot,SIGNAL(hide_it()),this,SLOT(hide_mainwindow()));
    connect(Screenshot,SIGNAL(show_it()),this,SLOT(show_mainwindow()));
    Screenshot->exec();
    screenshot_shown=false; //Tell that screenshot stops running
}

void MainWindow::hide_mainwindow()
{
    //hides the window from the screenshot dialog by sending a signal
    hide();
}

void MainWindow::show_mainwindow()
{
    //same as above
    show();
}

void MainWindow::bug(){
    if(system("xdg-open https://bugs.launchpad.net/wallpaper-changer"))
        cerr << "Error opening https://bugs.launchpad.net/wallpaper-changer\n";
}

void MainWindow::remove_list(){
    if (ui->addButton->isEnabled() || ui->listWidget->count() == 1 ){
        for(int i=0;i<ui->listWidget->count();i++){
            if(!QFile(ui->listWidget->item(i)->text()).exists()){
                if(ui->listWidget->count()==1){
                    ui->listWidget->clear();
                    break;
                }
                else
                {
                    delete ui->listWidget->item(i);
                    i--; //When deleting the non-existent item ->count() will be reduced by 1,
                    //so i++ will be 2 files after the deleted one. That's why i--
                }
            }
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText(tr("In order to do that stop the process!"));
        msgBox.setWindowTitle(QString::fromUtf8("Error!"));
        msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(56,56)));
        msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec();
    }
    if ( ui->listWidget->count() < 2 )
        disable_start_buttons();

    if ( ui->listWidget->count() < 3 ) ui->save_as->setEnabled(false);
}

void MainWindow::remove_disk(){
    if (ui->addButton->isEnabled() || ui->listWidget->count() == 1 ){
        QMessageBox msgBox2;
        QPushButton *Yes;
        msgBox2.setWindowTitle(tr("Image Deletion"));
        msgBox2.setInformativeText(tr("The image you selected will be permanently deleted from your hard disk. Are you sure?"));
        msgBox2.setText("<b>"+tr("Image Deletion")+"</b>");
        Yes = msgBox2.addButton(tr("Yes"), QMessageBox::ActionRole);
        msgBox2.addButton(tr("No"), QMessageBox::ActionRole);
        msgBox2.setIconPixmap(QIcon::fromTheme("user-trash").pixmap(QSize(100,100)));
        msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox2.exec();
        if (msgBox2.clickedButton() == Yes){
            int curr_ind = ui->listWidget->currentRow();
            QString qto_remove = ui->listWidget->item(curr_ind)->text().toUtf8();
            const char *to_remove = qto_remove.toLatin1().data();
                       if( remove( to_remove ) != 0 ){
                            QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText(tr("Image deletion failed! possible reasons are: a) Image doesn't exist or b) You don't have the permission to delete the image"));msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec();
                       }
                       else {
                            if(!ui->listWidget->count()) return;
                            if(ui->listWidget->count()==1) {
                                ui->listWidget->clear();
                                ui->label_9->clear();
                                return;
                            }
                            if(ui->listWidget->currentItem()->isSelected())
                                delete ui->listWidget->currentItem();
                       }
        } else return;
    }
        else
    {QMessageBox msgBox; msgBox.setText(tr("In order to do that stop the process!")); msgBox.setWindowTitle(QString::fromUtf8("Error!")); msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(56,56)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec(); }

        if(ui->listWidget->count()<=1)
            disable_start_buttons();
}

void MainWindow::load(){
    if (!ui->action_Load->isEnabled()) return;
    QString Qpath;

    Qpath = QFileDialog::getOpenFileName(this, tr("Choose Album"), QDir::homePath(), "*.wallch");

    if(Qpath.count() == 0) return; else ui->label_9->clear();
    char *path = Qpath.toUtf8().data();

    FILE *file=fopen(path,"r");
           if (file!=NULL)
           {
               ifstream file(path) ;
               string line ;
               while(getline(file,line))
               {
                   QString qstr = QString::fromUtf8(line.c_str());
                   QListWidgetItem *item = new QListWidgetItem;
                                    item->setText(qstr);
                                    item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                   ui->listWidget->addItem(item);
               }

           }
               fclose ( file );
               if ( ui->listWidget->count() <2 )
                   disable_start_buttons();
               else
                   enable_start_buttons();
}

void MainWindow::rotate_right(){
    if(!ui->listWidget->currentItem()->isSelected())
        return;
    QImage image(ui->listWidget->currentItem()->text());
    if(!QFile(ui->listWidget->currentItem()->text()).exists() || image.isNull())
        return;
    QString ext=ui->listWidget->currentItem()->text().right(3);
    if(ext == "gif" || ext == "GIF"){
        QMessageBox::warning(0, "Wallch | Error", "Rotation is not supported for gif files!");
        return;
    }
    const char* extension="";
    if(ext=="jpg" || ext=="JPG" || ext=="peg" || ext=="PEG")
        extension="JPG";
    else if(ext=="png" || ext=="PNG")
        extension="PNG";
    else if(ext=="bmp" || ext=="BMP")
        extension="BMP";
    QTransform rot;
    rot.rotate(90);
    image = image.transformed(rot, Qt::SmoothTransformation);
    image.save(ui->listWidget->currentItem()->text(), extension, 100);
    on_listWidget_itemSelectionChanged();
}

void MainWindow::rotate_left(){
    if(!ui->listWidget->currentItem()->isSelected())
        return;
    QImage image(ui->listWidget->currentItem()->text());
    if(!QFile(ui->listWidget->currentItem()->text()).exists() || image.isNull())
        return;
    QString ext=ui->listWidget->currentItem()->text().right(3);
    if(ext == "gif" || ext == "GIF"){
        QMessageBox::warning(0, "Wallch | Error", "Rotation is not supported for gif files!");
        return;
    }
    const char* extension="";
    if(ext=="jpg" || ext=="JPG" || ext=="peg" || ext=="PEG")
        extension="JPG";
    else if(ext=="png" || ext=="PNG")
        extension="PNG";
    else if(ext=="bmp" || ext=="BMP")
        extension="BMP";
    QTransform rot;
    rot.rotate(-90);
    image = image.transformed(rot,Qt::SmoothTransformation);
    image.save(ui->listWidget->currentItem()->text(), extension, 100);
    on_listWidget_itemSelectionChanged();
}

void MainWindow::copy_path(){
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->listWidget->currentItem()->text());
}

void MainWindow::copy_image(){
    QClipboard *clip = QApplication::clipboard();
    clip->setImage(QImage(ui->listWidget->currentItem()->text()), QClipboard::Clipboard);
}

void MainWindow::on_listWidget_customContextMenuRequested()
{
    QMenu menu;
        menu.addAction(tr("Set this item as Background"),this,SLOT(on_listWidget_itemDoubleClicked()));
        menu.addAction(tr("Remove non-existent Pictures"),this,SLOT(remove_list()));
        menu.addAction(tr("Delete image from disk"),this,SLOT(remove_disk()));
        menu.addAction(tr("Rotate Right"),this,SLOT(rotate_right()));
        menu.addAction(tr("Rotate Left"),this,SLOT(rotate_left()));
        menu.addAction(tr("Copy path to clipboard"),this,SLOT(copy_path()));
        menu.addAction(tr("Copy image to clipboard"),this,SLOT(copy_image()));
        menu.addAction(tr("Open folder"),this,SLOT(Openfolder()));
        menu.addAction(tr("Properties"),this,SLOT(showProperties()));

    if (ui->listWidget->count() > 0){
        if(!ui->addButton->isEnabled() || !ui->listWidget->currentIndex().isValid() || !ui->listWidget->currentIndex().isValid() || !ui->listWidget->currentItem()->isSelected())
        {
            menu.actions().at(0)->setEnabled(false);
            menu.actions().at(1)->setEnabled(false);
            menu.actions().at(2)->setEnabled(false);
            menu.actions().at(3)->setEnabled(false);
            menu.actions().at(4)->setEnabled(false);
        }
    menu.exec(QCursor::pos());
    }

}

void MainWindow::on_preview_clicked()
{
    if(ui->preview->isChecked()){
        preview=true;
        if(ui->listWidget->count()!=0 && ui->listWidget->currentRow()!=-1){
        QString filename = ui->listWidget->item(ui->listWidget->currentRow())->text().toAscii();
        ui->label_9->setBackgroundRole(QPalette::Base);
        QImage image(filename);
        ui->label_9->setPixmap(QPixmap::fromImage(image));}
    }
    else
    {
        preview=false;
        ui->label_9->clear();
    }
}

void MainWindow::on_webcamButton_clicked()
{
    if(camera_open!=1){
        mainWin = new MyCameraWindow(0);
        if(camera_success){
            mainWin->setWindowTitle(tr("Take Webcam Image"));
            camera_open=1;
            mainWin->show();
        }
    }
}

void MainWindow::Get_Help_Online()
{
    if(system("xdg-open https://answers.launchpad.net/wallpaper-changer/+addquestion"))
        cerr << "Error opening https://answers.launchpad.net/wallpaper-changer/+addquestion\n";
}

void MainWindow::errorstopprocess()
{
    QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText(tr("In order to change the current Desktop Wallpaper, please stop the current process."));msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(56,56)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec();
}

void MainWindow::on_addfolder_clicked()
{
    if (!ui->addfolder->isEnabled()) return;
    if (minimize_to_tray) add=1;
    QString qpath;
    qpath = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), QDir::homePath());
    add=0;
    if(qpath.count() == 0) return;
    QFile file9( QDir::homePath()+"/.config/Wallch/Checks/add_folder" );

    if( file9.open( QIODevice::WriteOnly ) ) {
      // file opened and is overwriten with WriteOnly
      QTextStream textStream( &file9 );
      textStream << qpath;
      file9.close();
    }
    //find pictures
    if(system("find \"$(cat ~/.config/Wallch/Checks/add_folder)\" \\( -name \"*.png\" -o -name \"*.gif\" -o -name \"*.bmp\" -o -name \"*jpg\" -o -name \"*jpeg\"  -o -name \"*PNG\" -o -name \"*.GIF\" -o -name \"*.BMP\" -o -name \"*JPG\" -o -name \"*JPEG\" \\) -print > ~/.config/Wallch/Checks/add_folder_pics"))
        cerr << "Error while trying to use command 'find'\n";
    //add files to listwidget
    FILE *file = fopen ( QString(QDir::homePath()+"/.config/Wallch/Checks/add_folder_pics").toLocal8Bit().data(), "r" );
           if ( file != NULL )
           {
               ifstream file( QString(QDir::homePath()+"/.config/Wallch/Checks/add_folder_pics").toLocal8Bit().data() ) ;
               string line ;
               while( std::getline( file, line ) )
               {
                   QString qstr = QString::fromUtf8(line.c_str());
                   QListWidgetItem *item = new QListWidgetItem;
                                    item->setText(qstr);
                                    item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                   ui->listWidget->addItem(item);
               }

           }
               fclose ( file );
               if ( ui->listWidget->count() <2 )
                   disable_start_buttons();
               else
                   enable_start_buttons();
               //remove temp files used
               if(system("rm -rf ~/.config/Wallch/Checks/add_fodler*"))
                   cerr << "Error while trying to remove ~/.config/Wallch/Checks/add_folder* .Please check file(s) permissions!";

               if ( ui->listWidget->count() >= 3 ) ui->save_as->setEnabled(true);
}

void MainWindow::close_minimize()
{
    minimize_to_tray=0; close();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(!ui->addButton->isEnabled()) return;
    if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urlList = event->mimeData()->urls();
    for (QList<QUrl>::const_iterator i = urlList.begin(); i != urlList.end();i++)
    {
       QString strFile2 = (*i).toString();
       if(strFile2.endsWith(".png") || strFile2.endsWith(".gif") || strFile2.endsWith(".bmp") || strFile2.endsWith(".jpg") || strFile2.endsWith(".jpeg") || strFile2.endsWith(".PNG") || strFile2.endsWith(".GIF") || strFile2.endsWith(".BMP") || strFile2.endsWith(".JPG") || strFile2.endsWith(".JPEG")){
          QString strFile1 = strFile2.mid(7, strFile2.length() - 2);
          ui->listWidget->addItem(strFile1);
       }
       else if(strFile2.endsWith(".wallch"))
       {
           QString file1 = strFile2.mid(7,strFile2.length()-2);
           char *path = file1.toUtf8().data();
           FILE *file = fopen ( path, "r" );
                  if ( file != NULL )
                  {
                      ifstream file( path ) ;
                      string line ;
                      while( std::getline( file, line ) )
                      {
                          QString qstr = QString::fromUtf8(line.c_str());
                          QListWidgetItem *item = new QListWidgetItem;
                                           item->setText(qstr);
                                           item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                          ui->listWidget->addItem(item);
                      }
                  }
                  fclose ( file );
       }
    }
    if ( ui->listWidget->count() <2 )
       disable_start_buttons();
    else enable_start_buttons();

    event->acceptProposedAction();
}

void MainWindow::on_actionContents_triggered()
{
    if(system("yelp file:///usr/share/gnome/help/wallch/C/wallch.xml 2> /dev/null&"))
        cerr << "Error opening /usr/share/gnome/help/wallch/C/wallch.xml with yelp! Check for file existence and/or for /usr/bin/yelp\n";
}

void MainWindow::dbus_action(const QString &msg){//QDBus signal handling
    if(msg=="FOCUS"){ //another application instance has come without any arguments, so focus to the already running process!
        this->showNormal();
        this->setFocusPolicy(Qt::StrongFocus);
        this->setFocus();
        this->raise();
        this->setVisible(1);
        this->activateWindow();
        if(minimize_to_tray){
            if(trayIcon->isVisible())
                trayIcon->hide();
        }
    }
    else if(msg=="NEW_ALBUM")
        add_new_album();
    else if(msg=="--start"){
        if(!_start_running_)
            on_startButton_clicked();
    }
    else if(msg=="--pause"){
        if(_start_running_)
            on_startButton_clicked();
    }
    else if(msg=="--stop")
        on_stopButton_clicked();
    else if(msg=="--next")
        on_nextButton_clicked();
    else if(msg=="--previous")
        on_previousButton_clicked();
    else if(msg=="--earth" || msg=="--earth_w8" || msg=="--earth_unity"){
        if(!_live_earth_running_)
            livearth();
        else
            cerr << "Live earth is already running!\n";
    }
    else if(msg=="--constant"){
        if(!_start_running_)
            on_startButton_clicked();
    }
    else if(msg=="--once"){
        if(!_start_running_ && ui->listWidget->count()>2){
            srand(time(0));
            int random_pic=rand()%ui->listWidget->count();
            QString qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \""+ui->listWidget->item(random_pic)->text()+"\"";
            __statistic__images_n++;
            previous_pictures << ui->listWidget->item(random_pic)->text();
            if(system(qcommand.toLocal8Bit().data()))
                cerr << "Error executing command that changes background!\n";
        }

    }
    else
        cerr << QString("Program received the unknown dbus message: " + msg +"\n").toLocal8Bit().data();
}

void MainWindow::on_startButton_clicked()
{    //Error handling
    if (!ui->startButton->isEnabled()) return; //this is the case it is launched from a Shortcut hotkey
    if (ui->listWidget->count()<4) ui->checkBox->setChecked(false);
    if (start){
        start=false;
    if(_live_earth_running_){
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Live Earth Wallpaper"));
        msgBox.setText(tr("Live Earth Wallpaper is running, if you want to continue, stop the process!"));
        msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec(); return;
    }
    ui->startButton->setText(tr("Pau&se"));
    ui->action_Start->setText(tr("Pau&se                                    Shift+Ctrl+S"));
    if(minimize_to_tray){
        Start_action->setText(tr("Pau&se"));
        Start_action->setIcon(QIcon::fromTheme("media-playback-pause"));
    }
    ui->startButton->setIcon(QIcon::fromTheme("media-playback-pause"));
    ui->action_Start->setIcon(QIcon::fromTheme("media-playback-pause"));
    if(!paused){//add animation
        //setting the effect
        QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
        opacityEffect->setOpacity(0.0);
        ui->time_for_next->setGraphicsEffect(opacityEffect);
        ui->time_for_next->show();
        QPropertyAnimation* anim = new QPropertyAnimation(this);
        anim->setTargetObject(opacityEffect);
        anim->setPropertyName("opacity");
        anim->setDuration(500);
        anim->setStartValue(opacityEffect->opacity());
        anim->setEndValue(1);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
    ////////////////////////////////////////////
    if(ui->timerSlider->value() < 3 ) seconds_minutes_hours=1;
    else if(ui->timerSlider->value()==3) seconds_minutes_hours=60;
    else if(ui->timerSlider->value() > 3 && ui->timerSlider->value() < 11) seconds_minutes_hours=60;
    else if(ui->timerSlider->value()==11) seconds_minutes_hours=3600;
    else if(ui->timerSlider->value() > 11) seconds_minutes_hours=3600;

    if (ui->timerSlider->value()==1 || ui->timerSlider->value()==6) number=10;
    else if (ui->timerSlider->value()==2 || ui->timerSlider->value()==9) number=30;
    else if (ui->timerSlider->value()==3 || ui->timerSlider->value()==11) number=1;
    else if (ui->timerSlider->value()==4 || ui->timerSlider->value()==13) number=3;
    else if (ui->timerSlider->value()==5) number=5;
    else if (ui->timerSlider->value()==7) number=15;
    else if (ui->timerSlider->value()==8) number=20;
    else if (ui->timerSlider->value()==10) number=45;
    else if (ui->timerSlider->value()==12) number=2;
    else if (ui->timerSlider->value()==14) number=4;
    else if (ui->timerSlider->value()==15) number=6;
    else if (ui->timerSlider->value()==16) number=12;
    else number=24;
    start_just_clicked=true; //<-- this means that this is the first time that it comes through the time_updater() so DO NOT update number using the possibly non-updated tmp_number (in case the user changed the timerslider value while the process was stopped.
    time_updater();
    update_time->start(1000);
            enable_stop_buttons(); enable_previous_and_next_buttons(); ui->checkBox->setEnabled(false); ui->checkBox_2->setEnabled(false); ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false); ui->actionAdd_Folder->setEnabled(false);
        ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false);
    _start_running_=1; //start pressed. If live earth wallpaper is running, it must know that start is pressed and show a warning
    }

    else {
        start=true;
        ui->startButton->setText(tr("&Start"));
        ui->action_Start->setText(("&Start                                    Shift+Ctrl+S"));
            if(minimize_to_tray)
                Start_action->setText(tr("&Start"));
        ui->startButton->setIcon(QIcon::fromTheme("media-playback-start"));
        ui->action_Start->setIcon(QIcon::fromTheme("media-playback-start"));
        if(minimize_to_tray)
            Start_action->setIcon(QIcon::fromTheme("media-playback-start"));

        paused=true;
        update_time->stop();
        ui->time_for_next->setFormat(ui->time_for_next->format()+" - Paused.");
        disable_previous_and_next_buttons();
        disable_stop_buttons();
        ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false); ui->actionAdd_Folder->setEnabled(false);  ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false);
        ui->actionS_top->setEnabled(false);
        if(ui->listWidget->count() != 0){ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true);}
        _start_running_=0; //start stopped
    }
}


void MainWindow::start_normal(){
        __statistic__images_n++;
        QString qcommand, ending;
            qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \"";
            ending="\"&";

        QString qimage;
        if(ui->checkBox->isChecked()){
               srand(time(0));
               int r = (rand() % ui->listWidget->count());
               if(!QImage(ui->listWidget->item(r)->text()).isNull()){
                   qimage=ui->listWidget->item(r)->text();
                   qcommand+=qimage+ending;;
                   previous_pictures << qimage;
                   if(system(qcommand.toLocal8Bit().data()))
                       cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
               }
       }
       else
       {
            if(start_normal_count>=ui->listWidget->count())
                start_normal_count=0;
            if(!QImage(ui->listWidget->item(start_normal_count)->text()).isNull()){
                qimage=ui->listWidget->item(start_normal_count)->text();
                qcommand+=qimage+ending;
                if(system(qcommand.toLocal8Bit().data())) cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
            }
            start_normal_count++;
        }
        if(ui->checkBox_2->isChecked()){
            srand(time(0));
            seconds_left = (rand()%1080)+120;
            random_first=seconds_left;
        }
        if(_show_notification_)
            QtConcurrent::run(this, &MainWindow::desktop_notify, qimage, 0);
        if(_sound_notification_)
            sound_notify();
        if(_save_history_)
            QtConcurrent::run(this, &MainWindow::save_history, qimage);
}

void MainWindow::time_updater(){
    if(paused)
        paused=false;
    if(!seconds_left){
        if(tmp_number*tmp_seconds_minutes_hours!=number*seconds_minutes_hours && tmp_number!=0 && !start_just_clicked){
            /*its quite complex here, well, if while changing pictures the user changes the timerslider, we want
            it to automatically change the timer while showing the next wallpaper. For this reason we have a temp value to
            store the value of the timerslider and now we set it to the normal value(number) Same thing with the seconds_minutes_hours
            the start_just_clicked variable exists so as not to come through here if the user stopped, changed value (while _start_running_ was false and so
            tmp_number wasn't updated) and then pushed start again. By doing so, it would come through this check and it would take the old non-updated value
            of tmp_number. If you didn't understand no problem, it was hard for me too xD*/
            number=tmp_number;
            seconds_minutes_hours=tmp_seconds_minutes_hours;
        }
        if(start_just_clicked)
            start_just_clicked=false;
        if(ui->checkBox_2->isChecked()){
            seconds_left = (rand()%1080)+120;
            random_first=seconds_left;
        }
        else{
            if (seconds_minutes_hours==1) seconds_left=number;
            if (seconds_minutes_hours==60) seconds_left=number*60;
            if (seconds_minutes_hours==3600) seconds_left=number*3600;
        }
        start_normal();

    }
    int minutes_left=0,hours_left=0, puttotime;
    if(seconds_left>=60){
        minutes_left=seconds_left/60;
        if(minutes_left>=60){
            hours_left=minutes_left/60;
        }
        minutes_left=minutes_left-hours_left*60;
        puttotime=seconds_left-(hours_left*3600+minutes_left*60);
    }
    else
        puttotime=seconds_left;
    if(!hours_left && !minutes_left)
        ui->time_for_next->setFormat(QString::number(puttotime)+"s");
    else if(!hours_left)
        ui->time_for_next->setFormat(QString::number(minutes_left)+"m " + QString::number(puttotime)+"s");
    else if(!minutes_left)
        ui->time_for_next->setFormat(QString::number(hours_left) + "h " + QString::number(puttotime)+"s");
    else
        ui->time_for_next->setFormat(QString::number(hours_left) + "h " + QString::number(minutes_left)+"m " + QString::number(puttotime)+"s");
    if(ui->checkBox_2->isChecked())
        ui->time_for_next->setValue((seconds_left*100)/random_first);
    else
        if (seconds_minutes_hours==1) ui->time_for_next->setValue((seconds_left*100)/(number));
        if (seconds_minutes_hours==60) ui->time_for_next->setValue((seconds_left*100)/(number*60));
        if (seconds_minutes_hours==3600) ui->time_for_next->setValue((seconds_left*100)/(number*3600));
    seconds_left--;
    if(ui->time_for_next->isHidden())
        ui->time_for_next->show();
}

void MainWindow::on_stopButton_clicked()
{
    if (!ui->stopButton->isEnabled()) return;
    ui->addButton->setEnabled(true); ui->listWidget->setDragDropMode(QAbstractItemView::DragDrop ); ui->addfolder->setEnabled(true); ui->action_Load->setEnabled(true); ui->actionAdd_single_images->setEnabled(true); ui->actionAdd_Folder->setEnabled(true);  ui->removeallButton->setEnabled(true); ui->removeButton->setEnabled(true); ui->moveupButton->setEnabled(true); ui->movedownButton->setEnabled(true);
    if(ui->listWidget->count()!=0){ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true);}
    start=true;
    ui->startButton->setText(tr("&Start"));
    ui->action_Start->setText(tr("&Start                                    Shift+Ctrl+S"));
    if(minimize_to_tray)
        Start_action->setText(tr("&Start"));

    ui->startButton->setIcon(QIcon::fromTheme("media-playback-start"));
    ui->action_Start->setIcon(QIcon::fromTheme("media-playback-start"));
    if(minimize_to_tray)
        Start_action->setIcon(QIcon::fromTheme("media-playback-start"));

    //Hide the progress bar with an opacity effect
    QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this); // make sure to create using new, since effect has to be alive as long as the target widget is using it.
    opacityEffect->setOpacity(1.0); // initially widget should be visible
    ui->time_for_next->setGraphicsEffect(opacityEffect);
    QPropertyAnimation* anim = new QPropertyAnimation(this);
    anim->setTargetObject(opacityEffect);
    anim->setPropertyName("opacity");
    anim->setDuration(500);
    anim->setStartValue(opacityEffect->opacity());
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    start_normal_count=0;
    seconds_left=0;
    update_time->stop();

    disable_previous_and_next_buttons();
    enable_start_buttons();
    disable_stop_buttons();
    _start_running_=0; //start stopped

}

void MainWindow::on_nextButton_clicked()
{
    if (!ui->nextButton->isEnabled())
                return;
    __statistic__images_n++;
    if(tmp_number*tmp_seconds_minutes_hours!=number*seconds_minutes_hours && tmp_number!=0){
        number=tmp_number;
        seconds_minutes_hours=tmp_seconds_minutes_hours;
    }

    QString qcommand, ending;
        qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \"";
        ending="\"&";
    //string quote="\"&";
    QString qimage;
    if(ui->checkBox->isChecked()){
        srand(time(0));
        int r = (rand() % ui->listWidget->count());
        qimage=ui->listWidget->item(r)->text();
        if(!QFile(qimage).exists() && QImage(qimage).isNull())
            return;
        qcommand+=qimage+ending;
        previous_pictures << qimage;
        if(system(qcommand.toLocal8Bit().data())) cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
        }
    if(ui->checkBox_2->isChecked()){
        srand(time(0));
        seconds_left = (rand()%1080)+120;
        random_first=seconds_left;
        update_time->start(1000);
    }
    if(!ui->checkBox->isChecked()){ //no random image, change image manually
       if(start_normal_count>=ui->listWidget->count())
           start_normal_count=0;
       qimage=ui->listWidget->item(start_normal_count)->text();
       if(!QFile(qimage).exists() && QImage(qimage).isNull())
           return;
       qcommand+=qimage+ending;
       if(system(qcommand.toLocal8Bit().data())) cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
       start_normal_count++;
}
    if(!ui->checkBox_2->isChecked()){
        if (seconds_minutes_hours==1) seconds_left=number;
        if (seconds_minutes_hours==60) seconds_left=number*60;
        if (seconds_minutes_hours==3600) seconds_left=number*3600;
        update_time->start(1000);
    }
    if(_show_notification_)
        QtConcurrent::run(this, &MainWindow::desktop_notify, qimage, 0);
    if(_sound_notification_)
        sound_notify();
    if(_save_history_)
        QtConcurrent::run(this, &MainWindow::save_history, qimage);

}

void MainWindow::on_previousButton_clicked()
{
    if(!ui->previousButton->isEnabled()) return;
    if(tmp_number*tmp_seconds_minutes_hours!=number*seconds_minutes_hours && tmp_number!=0){
        number=tmp_number;
        seconds_minutes_hours=tmp_seconds_minutes_hours;
    }
    __statistic__images_n++;
    update_time->stop();
    QString qcommand, ending;
        qcommand="gconftool-2 --type string --set /desktop/gnome/background/picture_filename \"";
        ending="\"&";
    QString qimage;
    if(ui->checkBox->isChecked()){
        if(previous_pictures.count()>1){
            qimage=previous_pictures.at(previous_pictures.count()-2);
            if(!QFile(qimage).exists() && QImage(qimage).isNull())
                return;
            previous_pictures.removeLast();
        }
        else{
            srand(time(0));
            int r = (rand() % ui->listWidget->count());
            qimage=ui->listWidget->item(r)->text();
            if(!QFile(qimage).exists() && QImage(qimage).isNull())
                return;
        }
        qcommand+=qimage+ending;
        if(system(qcommand.toLocal8Bit().data())) cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
        }
    if(ui->checkBox_2->isChecked()){
        srand(time(0));
        seconds_left = (rand()%1080)+120;
        random_first=seconds_left;
        update_time->start(1000);
    }
    if(!ui->checkBox->isChecked()){ //no random image, change image manually
        if(start_normal_count==1)
           start_normal_count=ui->listWidget->count()+1;

       qimage=ui->listWidget->item(start_normal_count-2)->text();
       if(!QFile(qimage).exists() && QImage(qimage).isNull())
           return;

       qcommand+=qimage+ending;
       if(system(qcommand.toLocal8Bit().data())) cerr << "Error while changing desktop image. Please check the ~/.gconf* paths for permission issues.";
       start_normal_count--;
}
    if(!ui->checkBox_2->isChecked()){
        update_time->start(1000);
        if (seconds_minutes_hours==1) seconds_left=number;
        if (seconds_minutes_hours==60) seconds_left=number*60;
        if (seconds_minutes_hours==3600) seconds_left=number*3600;
    }
    if(_show_notification_)
        QtConcurrent::run(this, &MainWindow::desktop_notify, qimage, 0);
    if(_sound_notification_)
        sound_notify();
    if(_save_history_)
        QtConcurrent::run(this, &MainWindow::save_history, qimage);

}

void MainWindow::handle(){

}

void MainWindow::desktop_notify(QString qimage, int notify){
    if(!QFile(qimage).exists() && !notify)
        return;
    QString body;
    if(notify==1)
        body=tr("Live Earth Wallpaper is active<br>and it will change every 30 minutes!");
    else
        body=tr("Your current wallpaper has changed!");
    if (!notify_init ("update-notifications"))
            return;
    if(notification_first_time){
        notification_first_time=false;
        /* try the icon-summary-body case */
        if(notify==0)
            notification = notify_notification_new ( "Wallch", body.toLocal8Bit().data(), qimage.toLocal8Bit().data(), NULL);
        else
            notification = notify_notification_new ( "Wallch", body.toLocal8Bit().data(), NULL, NULL);
        error = NULL;
        success = notify_notification_show (notification, &error);
        if (!success)
        {
                g_print ("That did not work ... \"%s\".\n", error->message);
                g_error_free (error);
        }

        g_signal_connect (G_OBJECT (notification), "closed", G_CALLBACK (&MainWindow::handle), NULL);

        return;
    }
    /* update the current notification with new content */
    if(notify==0)
        success = notify_notification_update (notification, "Wallch",body.toLocal8Bit().data(),qimage.toLocal8Bit().data());
    else
        success = notify_notification_update (notification, "Wallch",body.toLocal8Bit().data(), NULL);

    error = NULL;
    success = notify_notification_show (notification, &error);
    if (!success)
    {
            g_print ("That did not work ... \"%s\".\n", error->message);
            g_error_free (error);
            return;
    }
    g_signal_connect (G_OBJECT (notification), "closed", G_CALLBACK (&MainWindow::handle), NULL);
}

void MainWindow::sound_notify(){
    if(_sound_notification_==1){
        if(system(QString("canberra-gtk-play -f "+QString(PREFIX)+"/share/wallch/files/notification.ogg > /dev/null&").toLocal8Bit().data()))
            cerr << "Error executing canberra-gtk-play\n";
    }
    else{
        if(system("mpg321 \"$(cat ~/.config/Wallch/Checks/text_to_button)\" 2> /dev/null&"))
            cerr << "Error executing mpg321\n";
    }
}

void MainWindow::save_history(QString qimage){
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

void MainWindow::on_actionStatistics_triggered()
{
    if(!statistics_shown){
        statistics_shown=true;
        statistics *Statistics = new statistics(this);
        Statistics->exec();
        statistics_shown=false;
    }
}

void MainWindow::add_to_stats(){
    __statistic__etimer_elapsed++; //adding one every second.
}

//All dialogs Here
void MainWindow::on_actionHistory_triggered()
{
    if(histor)
        return;
    histor=true;
    History = new history (this);
    History->exec();
    histor=false;
}

void MainWindow::menushowabout(){
    About = new about(this);
    if(about_shown)
    {
        About->raise();
        About->activateWindow();
    }
    else
    {
        about_shown=true;
        About->exec();
        about_shown=false;
    }
}

void MainWindow::ShowPreferences()
{
    if(pref)
        return;
    Preferences = new preferences(this);
    pref=true;
    Preferences->exec(); //executing the preferences dialog.
    pref=false;
}

void MainWindow::ShowExtras()
{
    if(extras_shown)
        return;
    extras_shown=true;
    extras = new Extras(this);
    connect(extras,SIGNAL(start_live_earth()),this,SLOT(livearth()));
    connect(extras,SIGNAL(stop_live_earth()),this,SLOT(stop_livearth()));
    disconnect(this,0,extras,0);
    connect(this,SIGNAL(fix_livearth_buttons()),extras,SLOT(fix_buttons()));
    extras->exec(); //executing the preferences dialog.
    extras_shown=false;
}

void MainWindow::Openfolder(){
    if(!ui->listWidget->currentItem()->isSelected())
        return;
    QString qcommand="xdg-open \"$(dirname \""+ui->listWidget->currentItem()->text()+"\")\"";
    if(system(qcommand.toLocal8Bit().data()))
        cerr << "Error while executing '" << qcommand.toLocal8Bit().data() << "'";
}

void MainWindow::showProperties()
{
        if(!ui->listWidget->currentItem()->isSelected())
            return;

        QFile file9 ( QDir::homePath()+"/.config/Wallch/Checks/image_path" );
        if(file9.open(QIODevice::WriteOnly)){
          QTextStream textStream(&file9);
          textStream << QString::fromUtf8(ui->listWidget->currentItem()->text().toUtf8());
          file9.close();
        }
        if(!QFile(ui->listWidget->currentItem()->text()).exists() || QImage(ui->listWidget->currentItem()->text()).isNull()){ //if image doesn't exist or it is null
            QMessageBox msgBox;msgBox.setWindowTitle(tr("Properties"));msgBox.setText(tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallch.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec();
                if(system("rm -rf ~/.config/Wallch/Checks/image_property_path"))
                    cerr << "Error removing ~/.config/Wallch/Checks/image_property_path. Check file's permissions.\n";
                return;
            }
        else{  //image exists and it's not null, open the properties window.
             if(prop)
                 Properties->close();
             QString img_filename=ui->listWidget->currentItem()->text();
             Properties = new properties(img_filename,this);
             Properties->show();
             prop=1;
         }
}

void MainWindow::on_timerSlider_valueChanged(int value)
{
    QString time;

    if(value < 3 ){ time=" "+tr("seconds"); if (_start_running_) tmp_seconds_minutes_hours=1; else seconds_minutes_hours=1;}
    else if(value==3){ time=" "+tr("minute"); if (_start_running_) tmp_seconds_minutes_hours=60; else seconds_minutes_hours=60;}
    else if(value > 3 && value < 11){ time=" "+tr("minutes"); if (_start_running_) tmp_seconds_minutes_hours=60; else seconds_minutes_hours=60;}
    else if(value==11){ time=" "+tr("hour"); if (_start_running_) tmp_seconds_minutes_hours=3600; else seconds_minutes_hours=3600;}
    else if(value > 11){ time=" "+tr("hours"); if (_start_running_) tmp_seconds_minutes_hours=3600; else seconds_minutes_hours=3600;}

    if (value==1 || value==6){ if(_start_running_) tmp_number=10; else number=10;}
    else if (value==2 || value==9){ if(_start_running_) tmp_number=30; else number=30;}
    else if (value==3 || value==11){ if(_start_running_) tmp_number=1; else number=1;}
    else if (value==4 || value==13){ if(_start_running_) tmp_number=3; else number=3;}
    else if (value==5){ if(_start_running_) tmp_number=5; else number=5;}
    else if (value==7){ if(_start_running_) tmp_number=15; else number=15;}
    else if (value==8){ if(_start_running_) tmp_number=20; else number=20;}
    else if (value==10){ if(_start_running_) tmp_number=45; else number=45;}
    else if (value==12){ if(_start_running_) tmp_number=2; else number=2;}
    else if (value==14){ if(_start_running_) tmp_number=4; else number=4;}
    else if (value==15){ if(_start_running_) tmp_number=6; else number=6;}
    else if (value==16){ if(_start_running_) tmp_number=12; else number=12;}
    else if (value==17){ if(_start_running_) tmp_number=24; else number=24;}

    if(_start_running_) ui->label_10->setText(QString::number(tmp_number) + time); else ui->label_10->setText(QString::number(number) + time);
}

void MainWindow::disable_start_buttons()
{
    ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false);
    if (minimize_to_tray) Start_action->setEnabled(false);
}

void MainWindow::enable_start_buttons()
{
    ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true);
    if (minimize_to_tray) Start_action->setEnabled(true);
}

void MainWindow::disable_stop_buttons()
{
    ui->stopButton->setEnabled(false); ui->actionS_top->setEnabled(false);
    if (minimize_to_tray) Stop_action->setEnabled(false);
}

void MainWindow::enable_stop_buttons()
{
    ui->stopButton->setEnabled(true); ui->actionS_top->setEnabled(true);
    if (minimize_to_tray) Stop_action->setEnabled(true);
}

void MainWindow::disable_previous_and_next_buttons()
{
    ui->nextButton->setEnabled(false); ui->previousButton->setEnabled(false);
    ui->action_Previous_Image_Shift_Ctrl_B->setEnabled(false); ui->action_Next_Image->setEnabled(false);
    if (minimize_to_tray) {Previous_action->setEnabled(false); Next_action->setEnabled(false);}
}

void MainWindow::enable_previous_and_next_buttons()
{
    ui->nextButton->setEnabled(true); ui->previousButton->setEnabled(true);
    ui->action_Previous_Image_Shift_Ctrl_B->setEnabled(true); ui->action_Next_Image->setEnabled(true);
    if (minimize_to_tray) {Previous_action->setEnabled(true); Next_action->setEnabled(true);}
}

void MainWindow::add_new_album(){
    //no comments....
            if (!ui->addButton->isEnabled()) return;
            FILE *file = fopen ( QString(QDir::homePath()+"/.config/Wallch/Checks/new_albums").toLocal8Bit().data(), "r" );
                   if ( file != NULL )
                   {
                       ifstream file( QString(QDir::homePath()+"/.config/Wallch/Checks/new_albums").toLocal8Bit().data() ) ;
                       string line1;
                       while( getline( file, line1 ) )
                       {
                           const char* line = line1.c_str ();
                           FILE *file1 = fopen ( line, "r" );
                                  if ( file1 != NULL )
                                  {
                                      ifstream file( line ) ;
                                      string line ;
                                      while( std::getline( file, line ) )
                                      {
                                          QString qstr = QString::fromUtf8(line.c_str());
                                          QListWidgetItem *item = new QListWidgetItem;
                                                           item->setText(qstr);
                                                           item->setStatusTip(tr("Double-click to set an item from the list as Background"));
                                          ui->listWidget->addItem(item);
                                      }

                                  }
                       }
                   }
                   if ( ui->listWidget->count() <2 )
                       disable_start_buttons();
                   else
                       enable_start_buttons();

                   this->showNormal();
                   this->setFocusPolicy(Qt::StrongFocus);
                   this->setFocus();
                   this->raise();
                   this->setVisible(1);
                   this->activateWindow();
                   if(minimize_to_tray){
                       if(trayIcon->isVisible())
                           trayIcon->hide();
                   }
}

void MainWindow::on_checkBox_2_clicked()
{
    if(ui->checkBox_2->isChecked()) {ui->timerSlider->setEnabled(false); ui->label_10->setEnabled(false); ui->label_2->setEnabled(false);}
    else {ui->timerSlider->setEnabled(true); ui->label_10->setEnabled(true); ui->label_2->setEnabled(true);}
}

void MainWindow::save_album()
{
    if(ui->listWidget->count() < 3) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Not enough images!"));
        msgBox.setInformativeText(tr("You must select up to 3 pictures in order to save an album!"));
        msgBox.setText("<b>" + tr("The images are not enough.") + "</b>");
            msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(100,100)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
            msgBox.exec();
            return;
    }

    QString format = "wallch";
    QString initialPath = QDir::currentPath() + "/album." + format;
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(format.toUpper())
                               .arg(format));
    if(!fileName.isEmpty()){
        QFile file8( fileName );

        if( file8.open( QIODevice::WriteOnly ) ) {
          // file opened and is overwriten with WriteOnly
          QTextStream textStream( &file8 );
          for( int i=0; i < ui->listWidget->count(); ++i ){
             textStream << ui->listWidget->item(i)->text();
             textStream << '\n';
          }
          file8.close();
        }
    }
}
