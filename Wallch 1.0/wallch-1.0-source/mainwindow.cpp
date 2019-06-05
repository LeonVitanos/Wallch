/*Wallch - WallpaperChanger -A tool for changing Desktop Wallpapers automatically-
Copyright © 2010-2011 by Alex Solanos and Leon Vytanos

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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "preferences.h"
#include "screenshot.h"
#include "about.h"
#include "properties.h"
#include "glob.h"

#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include <QMessageBox>
#include <QtDBus/QtDBus>
#include <QDebug>
#include <QUrl>
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
#include "QListWidgetItem"
#include <QListWidget>
#include "QWidget"
#include "QListWidget"
using namespace std;

int _language_=1;
int _start_running_=0;
int _live_earth_running_=0;

char version[2]="1";
char othergl[35]="/.config/Wallch/Settings";
char checkoutput[20];
char *p=getenv("USER");

int d;
int pref=0;
int add=0;
int preview=0;
int show_notification;
int random_image=0, random_time=0;
int minimize_to_tray=0;
int properties_times=0;
int screen_is_on=0;
int exists=1;
int close_minimize_app=0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //checking to see whether the files are placed into the config or not. (usually the files aren't there at first-time run)
    struct stat stFileInfo;
    int intStat;
    char home1[150]="/home/";
    char other1[100]="/.config/Wallch/Settings";
    strcat(home1,p);
    strcat(home1,other1);
    intStat = stat(home1,&stFileInfo);
    if(intStat != 0){
        if(system("if [ -d /usr/share/wallch ]; then cp -r /usr/share/wallch/config/Wallch /home/$USER/.config/; else echo \"Folder /usr/share/wallch/ needed by the program doesn't exist. ERROR\"; fi "))
        cout << "Error copying /usr/share/wallch/config/Wallch to ~/.config. Please check the folder's existence or permissions.\n";
    }
    if(system("pidof wallch > /home/$USER/.config/Wallch/Checks/concurrent; launched=`awk '{print NF-1}' /home/$USER/.config/Wallch/Checks/concurrent`; if [ $launched -ne 0 ]; then echo \"Close\" > /home/$USER/.config/Wallch/Checks/concurrent; else echo \"Open\" > /home/$USER/.config/Wallch/Checks/concurrent; fi"))
        cout << "Error, could not write to ~/.config/Wallch/Checks/concurrent. Check file's permissions.\n";
    char home2[150]="/home/";
    strcat(home2,p);
    char other2[100]="/.config/Wallch/Checks/concurrent";
    strcat(home2,other2);
    char linepid [ 128 ];
   FILE *filepid = fopen ( home2, "r" );
       if(fgets ( linepid, sizeof linepid, filepid )==NULL)
           cout << "Error reading ~/.config/Wallch/Checks/concurrent!\n";
   fclose ( filepid );
   if(!strcmp (linepid,"Close\n")){ //sending the DBUS signal...
       QDBusMessage msg = QDBusMessage::createSignal("/", "open.album", "wallch_message");
       if(QCoreApplication::arguments().count()>1){ //means that an album has been opened, so open it with the already running process!
           //write the album paths into a file and then send the message to open them with the already running process...
           char new_albums[100]="/home/";
           char other_new_albums[100]="/.config/Wallch/Checks/new_albums";
           strcat(new_albums,p);
           strcat(new_albums,other_new_albums);
           QFile filenew_albums( new_albums );
           if( filenew_albums.open( QIODevice::WriteOnly ) ) {
             // file opened and is overwriten with WriteOnly
             QTextStream textStream( &filenew_albums );
             for(int i=1;i<QCoreApplication::arguments().count();i++){
                textStream << QCoreApplication::arguments().at(i);
                textStream << '\n';
             }
             filenew_albums.close();
           }
           msg << "NEW_ALBUM";
       }
       else{ //simple another instance
           msg << "FOCUS";
       }
       QDBusConnection::sessionBus().send(msg);
       if(system("echo \"Exiting because of already running process of wallch! Wallch has normally been focused, if not, current wallch processes are:`pidof wallch`\nIf you want to kill one of them type 'kill ID'\"; echo \"\" > /home/$USER/.config/Wallch/Checks/concurrent"))
            cout << "Error, couldn't output current wallch processes.\n";
       exit(2);
   }
   //listening to DBUS for another wallch instance/or to open a new album...
   QDBusConnection::sessionBus().connect(QString(),QString(), "open.album", "wallch_message", this, SLOT(album_slot(QString)));
   //checking to see if live earth wallpaper is running...

   if(system("pidof -x setupwallpaper > /home/$USER/.config/Wallch/Checks/earth; launched=`cat /home/$USER/.config/Wallch/Checks/earth`; if [ X\"$launched\" != X\"\" ]; then echo \"runs\" > /home/$USER/.config/Wallch/Checks/earth; else echo \"doesnt run\" > /home/$USER/.config/Wallch/Checks/earth; fi"))
       cout << "Error while trying to write to ~/.config/Wallch/Checks/earth. Please check file's permissions.\n";

   char home_earth[150]="/home/";
   strcat(home_earth,p);
   char other_earth[100]="/.config/Wallch/Checks/earth";
   strcat(home_earth,other_earth);
   char line_earth [ 128 ];
   FILE *file_earth = fopen ( home_earth, "r" );
      if(fgets ( line_earth, sizeof line_earth, file_earth )==NULL)
          cout << "Error reading ~/.config/Wallch/Checks/earth\n";
   fclose ( file_earth );
   if(!strcmp (line_earth,"runs\n"))
       _live_earth_running_=1;

   //checking the language
   char home3[150]="/home/";
   strcat(home3,p);
   strcat(home3,othergl);
   char languageline[128];
      FILE *file5 = fopen ( home3, "r" );
      if(fgets ( languageline, sizeof languageline, file5 )==NULL)
          cout << "FATAL: Error reading ~/.config/Wallch/Settings, which is an essential file for the functionality of the program!\n";
      fclose ( file5 );
  if(!strcmp (languageline,"Language: Greek\n")) _language_=2;

   //Putting the values back to spinboxes and to listwidget...
    QSettings settings( "Wallch", "DesktopWallpaperChanger" );
    int size = settings.beginReadArray("listwidgetitem");
    for (int i = 0; i < size; ++i) {     settings.setArrayIndex(i);
    QListWidgetItem *item = new QListWidgetItem;
                     item->setText(settings.value("item").toString());
                     if(_language_==1)
                        item->setToolTip("Double-click to set this item as Background");
                     else
                          item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως background"));
                      ui->listWidget->addItem(item);

    }
    settings.endArray();
                    ui->spinBox->setValue( settings.value( "spinBox" ).toInt() );
                    ui->spinBox_2->setValue( settings.value( "spinBox_2" ).toInt() );

    //Setting up the timers
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(write_random_image()));
    ti2 = new QTimer(this);
    connect(ti2, SIGNAL(timeout()), this, SLOT(write_random_time()));

    //if widget is empty, then disable the checkboxes to show that something goes wrong :P
    if ( ui->listWidget->count() == 0 ) {ui->checkBox->setEnabled(false); ui->checkBox_2->setEnabled(false);} else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true);}
    //updating the randomimage and random time. Writing 0 to these files means that randomimage and randomtime are still disabled.
    if(system("echo 0 > /home/$USER/.config/Wallch/Checks/randomimage"))
        cout << "Error writing to ~/.config/Wallch/Checks/randomimage. Check file's permissions.\n";
    if(system("echo 0 > /home/$USER/.config/Wallch/Checks/randomtime"))
        cout << "Error writing to ~/.config/Wallch/Checks/randomtime. Check file's permissions.\n";
    //Checking to see if we will create the tray (tray-menu,tray-Icon etc) eithr because was previously selected by the user or because the user has selected to
    //start automatically changing wallpapers on program launch and has also selected the app to be automatically minimized when this is done
    //checking to see if preview is ON or OFF
    char home4[150]="/home/";
    strcat(home4,p);
    strcat(home4,othergl);
    char preview3[128];
    int languagei25;
       FILE *file35 = fopen ( home4, "r" );
       for ( languagei25 = 0; languagei25 < 3; ++languagei25 ) //getting the third line...
       {
           if(fgets ( preview3, sizeof preview3, file35 )==NULL)
               cout << "Error reading ~/.config/Wallch/Settings, which is an essential file for the functionality of the program!\n";
       }
       fclose ( file35 );

       if(!strcmp (preview3,"Preview: True\n")){preview=1; ui->preview->setCheckState(Qt::Checked);}
   else ui->preview->setCheckState(Qt::Unchecked);

   char randomoimages[128];
   int languagei55;
      FILE *file65 = fopen ( home4, "r" );
      for ( languagei55 = 0; languagei55 < 8; ++languagei55 )
      {
      if(fgets ( randomoimages, sizeof randomoimages, file65 )==NULL)
          cout << "Error reading ~/.config/Wallch/Settings, which is an essential file for the functionality of the program!\n";
      }
      fclose ( file65 );

      if(!strcmp (randomoimages,"Random Images: True\n"))ui->checkBox->setCheckState(Qt::Checked);

      char randomotime[128];
      int languagei65;
         FILE *file75 = fopen ( home4, "r" );
         for ( languagei65 = 0; languagei65 < 9; ++languagei65 )
         {
         if(fgets ( randomotime, sizeof randomotime, file75 )==NULL)
             cout << "Error reading ~/.config/Wallch/Settings, which is an essential file for the functionality of the program!\n";
         }
         fclose ( file75 );

         if(!strcmp (randomotime,"Random Time: True\n")){
             ui->checkBox_2->setCheckState(Qt::Checked);
             ui->spinBox->setEnabled(false);
             ui->spinBox_2->setEnabled(false);
         }

    char trayxa[128];
    int languagei85;
       FILE *file45 = fopen ( home4, "r" );
       for ( languagei85 = 0; languagei85 < 7; ++languagei85 )
       {
       if(fgets ( trayxa, sizeof trayxa, file45 )==NULL)
           cout << "Error reading ~/.config/Wallch/Settings, which is an essential file for the functionality of the program!\n";
       }
       fclose ( file45 );

       if (!strcmp (trayxa,"Minimize to tray: True\n")) minimize_to_tray=1;

    //Setting up the tray  Icon.
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/Pictures/wallpaper.png"));
    if(_language_==2)
    trayIcon->setToolTip(QString::fromUtf8("<table width='300'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Αριστερό κλικ: Εμφάνιση του παραθύρου αν η διεργασία δεν τρέχει/Επόμενη εικόνα αν η διεργασία τρέχει.</td></tr> <tr><td nowrap align='left'>Μεσαίο κλικ: Έξοδο εφαρμογής.</td> </tr> <tr><td nowrap align='left'>Δεξί κλικ: Δείξε μερικές επιλογές.</td> </tr></table>"));
    else
    trayIcon->setToolTip("<table width='280'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Left click: Show window if process is not running/Next image if process is running.</td></tr> <tr><td nowrap align='left'>Middle click: Exit the application.</td> </tr> <tr><td nowrap align='left'>Right click: Show some options.</td> </tr></table>");
          connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(clickSysTrayIcon(QSystemTrayIcon::ActivationReason)));
    connect(this,SIGNAL(minimized()),this,SLOT(hide()),Qt::QueuedConnection);
    QMenu *changer_menu = new QMenu;
    if(_language_==2)
    {
        Show_action = new QAction(QString::fromUtf8("Επ&αναφορά"),this);
    }else{
        Show_action = new QAction("S&how",this);
    }
    Show_action->setIconVisibleInMenu(true);
    connect(Show_action, SIGNAL(triggered()), this, SLOT(showClicked()));
    changer_menu->addAction(Show_action);
    changer_menu->addSeparator();
    if(_language_==2){
        Start_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_play.png"), QString::fromUtf8("Έ&ναρξη"), this);
    }
    else
    {
        Start_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_play.png"), tr("&Start"), this);
    }
    Start_action->setIconVisibleInMenu(true);
    connect(Start_action, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    changer_menu->addAction(Start_action);
    if(_language_==2){
        Pause_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_pause.png"), QString::fromUtf8("&Πάυση"), this);
    }
    else
    {
        Pause_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_pause.png"), tr("&Pause"), this);
    }
    Pause_action->setIconVisibleInMenu(true);
    connect(Pause_action, SIGNAL(triggered()), this, SLOT(on_pauseButton_clicked()));
    changer_menu->addAction(Pause_action);
    if(_language_==2){
        Stop_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_stop.png"), QString::fromUtf8("&Σταμάτημα"), this);
    }
    else
    {
        Stop_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_stop.png"), tr("&Stop"), this);
    }
    Stop_action->setIconVisibleInMenu(true);
    connect(Stop_action, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked()));
    changer_menu->addAction(Stop_action);
    if(_language_==2){
        Next_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_fastforward.png"), QString::fromUtf8("Επόμεν&η εικόνα"), this);
    }
    else
    {
        Next_action = new QAction(QIcon(":/Buttons/Pictures/Buttons/button_blue_fastforward.png"), tr("&Next Image"), this);
    }


    Next_action->setIconVisibleInMenu(true);
    connect(Next_action, SIGNAL(triggered()), this, SLOT(on_nextButton_clicked()));
    changer_menu->addAction(Next_action);
    changer_menu->addSeparator();
    if(_language_==2){
        Settings_action = new QAction(QString::fromUtf8("&Ρυθμίσεις"),this);
    }
    else
    {
        Settings_action = new QAction("P&references",this);
    }
    Settings_action->setIconVisibleInMenu(true);
    connect(Settings_action, SIGNAL(triggered()), this, SLOT(ShowPreferences()));
    changer_menu->addAction(Settings_action);
    if(_language_==2){
        About_action = new QAction(QString::fromUtf8("Περί του προ&γράμματος"), this);
    }
    else
    {
        About_action = new QAction("&About", this);
    }
    About_action->setIconVisibleInMenu(true);;
    connect(About_action, SIGNAL(triggered()), this, SLOT(menushowabout()));
    changer_menu->addAction(About_action);
    changer_menu->addSeparator();
    if(_language_==2)
    {
        Quit_action = new QAction(QString::fromUtf8("Έξ&οδος"), this);
    }
    else
    {
        Quit_action = new QAction("&Quit", this);
    }
    Quit_action->setIconVisibleInMenu(true);;
    connect(Quit_action, SIGNAL(triggered()), this, SLOT(close_minimize()));
    changer_menu->addAction(Quit_action);

    trayIcon->setContextMenu(changer_menu);

    ui->menuAdd_Files->menuAction()->setIconVisibleInMenu(true);
    //Disabling the Next button (nothing is running, so!?)
    ui->nextButton->setEnabled(false); ui->action_Next_Image->setEnabled(false); if ( minimize_to_tray ) Next_action->setEnabled(false);

   if(_language_==2){
   translatetogreek();
   }
   else
   {
   ui->startButton->setToolTip("Start Operation");
   ui->pauseButton->setToolTip("Pause Operation");
   ui->stopButton->setToolTip("Stop Operation");
   ui->nextButton->setToolTip("Proceed to next image...");
   }
   //SIGNAL-SLOT action here ;)
   connect(ui->action_About, SIGNAL(triggered()), this, SLOT(menushowabout()));
   connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(ShowPreferences()));
   connect(ui->action_Check_For_Updates, SIGNAL(triggered()), this, SLOT(checkForUpdate()));
   connect(ui->actionQuit_Ctrl_Q, SIGNAL(triggered()), this, SLOT(close_minimize()));
   connect(ui->action_Start, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
   connect(ui->actionS_top, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked()));
   connect(ui->action_Load, SIGNAL(triggered()), this, SLOT(load()));
   connect(ui->action_Pause, SIGNAL(triggered()), this, SLOT(on_pauseButton_clicked()));
   connect(ui->action_Remove_History, SIGNAL(triggered()), this, SLOT(remove_history()));
   connect(ui->actionRemove_list, SIGNAL(triggered()), this, SLOT(remove_list()));
   connect(ui->action_Next_Image, SIGNAL(triggered()), this, SLOT(on_nextButton_clicked()));
   connect(ui->actionReport_A_Bug, SIGNAL(triggered()), this, SLOT(bug()));
   connect(ui->actionGet_Help_Online, SIGNAL(triggered()), this, SLOT(Get_Help_Online()));
   connect(ui->actionAdd_single_images, SIGNAL(triggered()), this, SLOT(on_addButton_clicked()));
   connect(ui->actionAdd_Folder, SIGNAL(triggered()), this, SLOT(on_addfolder_clicked()));

//setting up the shortcut keys!
(void) new QShortcut(Qt::CTRL + Qt::Key_Q, this, SLOT(close_minimize()));
(void) new QShortcut(Qt::CTRL + Qt::Key_P, this, SLOT(ShowPreferences()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_S, this, SLOT(on_startButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_O, this, SLOT(on_stopButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_P, this, SLOT(on_pauseButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N, this, SLOT(on_nextButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::Key_R, this, SLOT(remove_history()));
(void) new QShortcut(Qt::CTRL + Qt::Key_U, this, SLOT(checkForUpdate()));
(void) new QShortcut(Qt::CTRL + Qt::Key_O, this, SLOT(load()));
(void) new QShortcut(Qt::CTRL + Qt::Key_I, this, SLOT(on_addButton_clicked()));
(void) new QShortcut(Qt::CTRL + Qt::Key_F, this, SLOT(on_addfolder_clicked()));
(void) new QShortcut(Qt::Key_Delete, this, SLOT(on_removeButton_clicked()));
(void) new QShortcut(Qt::Key_Return, this, SLOT(doubleonrightclick()));
(void) new QShortcut(Qt::Key_Alt + Qt::Key_Return, this, SLOT(showProperties()));
(void) new QShortcut(Qt::Key_F1, this, SLOT(on_actionContents_triggered()));

//creating the about dialog
About = new about( this );

if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->stopButton->setEnabled(false);ui->pauseButton->setEnabled(false);ui->action_Start->setEnabled(false);ui->actionS_top->setEnabled(false);ui->action_Pause->setEnabled(false);if (minimize_to_tray==1 ){Stop_action->setEnabled(false);Pause_action->setEnabled(false);  Start_action->setEnabled(false);}}
else {ui->stopButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->actionS_top->setEnabled(false);
    ui->action_Pause->setEnabled(false);
    if (minimize_to_tray==1 )
    {
    Stop_action->setEnabled(false);
    Pause_action->setEnabled(false);}}
//checking to see if constant_on_boot is running, and if so, disable the start button and enable the stop in order to stop it
if(system("pidof -x constant_on_boot > /home/$USER/.config/Wallch/Checks/concurrent; launched=`cat /home/$USER/.config/Wallch/Checks/concurrent`; if [ X\"$launched\" != X\"\" ]; then echo \"runs\" > /home/$USER/.config/Wallch/Checks/concurrent; else echo \"doesnt run\" > /home/$USER/.config/Wallch/Checks/concurrent; fi"))
    cout << "Error while trying to check the constant_on_boot process. Check the permissions of the file ~/.config/Wallch/Checks/concurrent\n";

char home5[150]="/home/";
strcat(home5,p);
char other5[100]="/.config/Wallch/Checks/concurrent";
strcat(home5,other5);
char linepid1 [ 128 ];
FILE *filepid1 = fopen ( home5, "r" );
if(fgets ( linepid1, sizeof linepid1, filepid1 )==NULL)
       cout << "Error reading ~/.config/Wallch/Checks/concurrent\n";
fclose ( filepid1 );
if(!strcmp (linepid1,"runs\n")){
    ui->pauseButton->setEnabled(false); ui->action_Start->setEnabled(false); ui->action_Pause->setEnabled(false); ui->actionS_top->setEnabled(true); ui->stopButton->setEnabled(true); ui->nextButton->setEnabled(false); ui->action_Next_Image->setEnabled(false); ui->checkBox->setEnabled(false); ui->checkBox_2->setEnabled(false); ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false); ui->actionAdd_Folder->setEnabled(false);  ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false); ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false); ui->startButton->setEnabled(false); if (minimize_to_tray==1 ) {Start_action->setEnabled(false); Pause_action->setEnabled(false); Stop_action->setEnabled(true); Next_action->setEnabled(false);}
} if(system("rm -rf /home/$USER/.config/Wallch/Checks/concurrent"))
    cout << "Error removing ~/.config/Wallch/Checks/randomimage. Check file's permissions.\n";
    if(ui->listWidget->count()<=2){
        ui->checkBox->setEnabled(false);
        if(ui->listWidget->count()==0 || ui->listWidget->count()==1) ui->checkBox_2->setEnabled(false);
}
QRect rect = QApplication::desktop()->availableGeometry();
//Moving the window to the center of screen!
this->move(rect.center() - this->rect().center());
//enabling dragendrop
setAcceptDrops(true);
//checking for command line arguments (to open wallch albums)
if(QCoreApplication::arguments().count()>1){
    for(int i=1;i<QCoreApplication::arguments().count();i++){
        QString Qpath = QCoreApplication::arguments().at(i);
        char *path = Qpath.toUtf8().data();

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
                                        if(_language_==2)
                                            item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                                        else
                                            item->setToolTip("Double-click to set this item as Background...");
                       ui->listWidget->addItem(item);
                   }

               }
                   fclose ( file );
                   if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
                   else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
    }
}

}

void MainWindow::closeEvent( QCloseEvent * )
{
    if (!minimize_to_tray){
    if(system("echo > /home/$USER/.config/Wallch/DesktopWallpaperChanger.conf"))
        cout << "Error writing to ~/.config/Wallch/DesktopWallpaperChanger.conf. Please check file's permissions.\n";
    if(ui->preview->isChecked()){
        if(system("sed -i '3 d' /home/$USER/.config/Wallch/Settings; sed -i \"3i\\Preview: True\n\" /home/$USER/.config/Wallch/Settings"))
            cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
    }
   else {
       if(system("sed -i '3 d' /home/$USER/.config/Wallch/Settings; sed -i \"3i\\Preview: False\n\" /home/$USER/.config/Wallch/Settings"))
            cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
   }

    QApplication::setQuitOnLastWindowClosed(1);
    QSettings //Saving the settings to the configuration file.
       settings( "Wallch", "DesktopWallpaperChanger" );
    settings.beginWriteArray("listwidgetitem");
    for (int i = 0; i < ui->listWidget->count(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("item", ui->listWidget->item(i)->text() );
    }
    settings.endArray();
       settings.setValue( "spinBox", ui->spinBox->value() );
       settings.setValue( "spinBox_2", ui->spinBox_2->value() );
       settings.sync();
       //killing the script(s) and all the sounds...
       if(system("killall -v starter notify-osd starter_on_launch mpg321 mpg123 canberra-gtk-play 2> /dev/null")!=256)
           cout << "There was probably an error while trying to kill starter, notify-osd, starter_on_launch, mpg321 mpg123 and canberra-gtk-play\n";
       //checking to see if the earth wallpaper is activated
       if(system("pidof -x setupwallpaper > /home/$USER/.config/Wallch/Checks/earth; launched=`cat /home/$USER/.config/Wallch/Checks/earth`; if [ X\"$launched\" != X\"\" ]; then echo \"runs\" > /home/$USER/.config/Wallch/Checks/earth; else echo \"doesnt run\" > /home/$USER/.config/Wallch/Checks/earth; fi"))
           cout << "Could not get pidof setupwallpaper or could not write to ~/.config/Wallch/Checks/earth. Please check file's permissions\n";


       char home6[150]="/home/";
       strcat(home6,p);
       char other6[100]="/.config/Wallch/Checks/earth";
       strcat(home6,other6);
       char linepidclose [ 128 ];
       FILE *filepidclose = fopen ( home6, "r" );
       if(fgets ( linepidclose, sizeof linepidclose, filepidclose )==NULL)
           cout << "Error reading ~/.config/Wallch/Checks/earth\n";
       fclose ( filepidclose );
       if(system("rm -rf /home/$USER/.config/Wallch/Checks/earth"))
           cout << "Couldn't delete ~/.config/Wallch/Checks/earth. Please check file's permissions.\n";

       if(!strcmp (linepidclose,"runs\n")){
       QMessageBox msgBox2; QPushButton *Stop; QPushButton *Nostop; if(_language_==2){msgBox2.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό Background της Γης"));msgBox2.setInformativeText(QString::fromUtf8("Θέλετε να σταματήσετε την διεργασία ή να την αφήσετε να τρέχει;"));msgBox2.setText(QString::fromUtf8("<b>Το Ζωντανό Background της Γης τρέχει.</b>"));Stop = msgBox2.addButton(QString::fromUtf8("Σταμάτησε την διεργασία"), QMessageBox::ActionRole);Nostop = msgBox2.addButton(QString::fromUtf8("Άφησέ την να τρέχει"), QMessageBox::ActionRole);}else{msgBox2.setWindowTitle("Wallch | Live Earth Wallpaper");msgBox2.setInformativeText("Do you want to stop the process or let it run?");msgBox2.setText("<b>Live Earth Wallpaper is running.</b>");Stop = msgBox2.addButton("Stop the process", QMessageBox::ActionRole);Nostop = msgBox2.addButton("Let it run", QMessageBox::ActionRole);}msgBox2.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(60,60)));msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox2.exec();

       if (msgBox2.clickedButton() == Stop){
           if(system("killall -v setupwallpaper 2> /dev/null"))
               cout << "Error, probably couldn't stop setupwallpaper process";
       }
       }
       //Writing the dela to the delay file. This is done in the on_startbutton_clicked() normally but it is needed here because
       //because of the constant_on_boot script (to take the time that existed on close, and not in the last on_start_button_clicked()
       int qw = ui->spinBox->text().toInt();
       int er = qw * 60; //1 minute = 60 secs
       int ty = ui->spinBox_2->text().toInt();
       d = er+ty;
       char file[80]= "/home/";
       char file2[70]="/.config/Wallch/Checks/delay";
       strcat(file,p);
       strcat(file,file2);
       ofstream newFile;
       newFile.open(file);
       newFile << d;
       newFile.close();

       if (ui->checkBox->isChecked()) {
       if(system(" sed -i '8 d' /home/$USER/.config/Wallch/Settings;sed -i \"8i\\Random Images: True\n\" /home/$USER/.config/Wallch/Settings"))
           cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
       }
       else{
           if(system(" sed -i '8 d' /home/$USER/.config/Wallch/Settings;sed -i \"8i\\Random Images: False\n\" /home/$USER/.config/Wallch/Settings"))
               cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
       }
       if (ui->checkBox_2->isChecked()){
           if(system("sed -i '9 d' /home/$USER/.config/Wallch/Settings; sed -i \"9i\\Random Time: True\n\" /home/$USER/.config/Wallch/Settings"))
               cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
       }
       else{
           if(system(" sed -i '9 d' /home/$USER/.config/Wallch/Settings;sed -i \"9i\\Random Time: False\n\" /home/$USER/.config/Wallch/Settings"))
               cout << "FATAL ERROR: Couldn't write to ~/.config/Wallch/Settings, which is essential for the functionality of the program.\nPlease check the file's permissions.\n";
       }
   }
    else {
        trayIcon->show();
        emit minimized();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    if(pref || add)MainWindow::showNormal();
    if ( minimize_to_tray!=1){ //Means that minimized isn't selected.
        QMainWindow::changeEvent(e);
        switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
        }}
    else{ //Means that minimized is selected

    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    //see if the event is changing the window state

    case QEvent::WindowStateChange:
        //if it is, we need to see that this event is minimize

        if (isMinimized()) {
            trayIcon->show();
           emit minimized();}

    default:
        break;

        QMainWindow::changeEvent(e); }}
}

void MainWindow::on_startButton_clicked()
{    //Error handling
    if (!ui->startButton->isEnabled()) return; //this is the case it is launched from a Shortcut hotkey
    if(_live_earth_running_){
    QMessageBox msgBox;
    if(_language_==2)
        {
        msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό  Wallpaper της Γης"));
        msgBox.setText(QString::fromUtf8("Το ζωντανό  wallpaper της γης τρέχει, αν θέλετε να συνεχίσετε, σταματήστε την διεργασία!"));
    }
    else{
        msgBox.setWindowTitle("Wallch | Live Earth Wallpaper");
        msgBox.setText("Live Earth Wallpaper is running, if you want to continue, stop the process!");
    }
    msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
    msgBox.exec(); return;
    }
    if(!ui->checkBox_2->isChecked()){if ( ui->spinBox->text() == "0" && ui->spinBox_2->text() == "0" )
        {QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Χρόνος"));msgBox.setText(QString::fromUtf8("Παρακαλώ ορίστε τον χρόνο σωστά!"));} else{msgBox.setWindowTitle("Wallch | Time"); msgBox.setText("Please Specify the time correctly!");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png")); msgBox.exec(); return;}}

    if(system("killall -v setupwallpaper constant_on_boot starter mpg321 mpg123 canberra-gtk-play 2> /dev/null")!=256)
        cout << "There was probably an error while trying to kill setupwallpaper, constant_on_boot, starter, mpg321, mpg123 and canberra-gtk-play\n";
    char home7[100]="/home/";
    char other7[100]="/.config/Wallch/Checks/to_be_fixed";
    strcat (home7,p);
    strcat (home7,other7);
    QFile file8( home7 );
    if( file8.open( QIODevice::WriteOnly ) ) {
      // file opened and is overwriten with WriteOnly
      QTextStream textStream( &file8 );
      for( int i=0; i < ui->listWidget->count(); ++i ){
         textStream << ui->listWidget->item(i)->text();
         textStream << '\n';
      }
      file8.close();
    }
    if(system("sed '/^$/d' /home/$USER/.config/Wallch/Checks/to_be_fixed > /dev/null"))
        cout << "Error writing to ~/.config/Wallch/Checks/to_be_fixed. Check file's permissions.\n";


    //enable the buttons that can perform action during the operation of changing wallpapers
    if(ui->checkBox->isChecked() && ui->checkBox->isEnabled()){ //Random image is selected. start the timer.
        int r = (rand() % ui->listWidget->count()) + 1; //random number till the numbers of selected pictures
        //The random image (r) will be written to a file, then the script will read this file in its startup.
        char home8[130]="/home/";
        strcat(home8,p);
        char other8[100]="/.config/Wallch/Checks/randomimage";
        strcat(home8,other8);
        if ( r==1 ) r=2; //I don't want to start from the first image!!!
        //Now we do the job that timer does once, because the timer will start doing this job after some time, and the script will find 0 into the file and will "think" that randomimage is disabled
        QFile filw(home8);
        if (!filw.open(QIODevice::WriteOnly)) {
            cerr << "Cannot open file" << home8 << " for writing. The reason: "
                 << qPrintable(filw.errorString()) << endl;
    }
        QTextStream out(&filw);
        out << r;
        filw.close();
        int a = ui->spinBox->text().toInt();
        int b = a * 60;
        int c = ui->spinBox_2->text().toInt();
        d = b+c;
        int timerint=d * 1000;
        if ( ui->listWidget->count() != 2 )
        timer->start(timerint);
        }
    if(ui->checkBox_2->isChecked() && ui->checkBox_2->isEnabled()){ //same things with randomtime
            srand(time(0));
            int rtime = (rand() % 1200) + 121; //Random number till 120
            //The random time will be written to a file, then the starter will read this file in its startup.
            //If the file contains a number
            char home9[130]="/home/";
            strcat(home9,p);
            char other9[100]="/.config/Wallch/Checks/randomtime";
            strcat(home9,other9);
            ofstream randomtime;
            randomtime.open(home9);
            randomtime << rtime;
            randomtime.close();
            ti2->start(120000); //120 seconds
    }

        int qw = ui->spinBox->text().toInt();
        int er = qw * 60; //1 minute = 60 secs
        int ty = ui->spinBox_2->text().toInt();
        d = er+ty;
        char file[80]= "/home/";
        char file2[70]="/.config/Wallch/Checks/delay";
        strcat(file,p);
        strcat(file,file2);
        ofstream newFile;
        newFile.open(file);
        newFile << d;
        newFile.close();
        if(system("/usr/share/wallch/files/Scripts/starter normal&")){  //The script starts here
            cout << "FATAL: Error executing ~/.config/Wallch/Scripts/starter.\na)Check file existence\nb)If exists, place the executable bit\nc)Check for permission to execute the file\n";
        }
        ui->pauseButton->setEnabled(true); ui->action_Start->setEnabled(false); ui->action_Pause->setEnabled(true); ui->actionS_top->setEnabled(true); ui->stopButton->setEnabled(true); ui->nextButton->setEnabled(true); ui->action_Next_Image->setEnabled(true); ui->checkBox->setEnabled(false); ui->checkBox_2->setEnabled(false); ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false); ui->actionAdd_Folder->setEnabled(false);  ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false); ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false); ui->startButton->setEnabled(false); if (minimize_to_tray==1 ) {Start_action->setEnabled(false); Pause_action->setEnabled(true); Stop_action->setEnabled(true); Next_action->setEnabled(true);}
        _start_running_=1; //start pressed. If live earth wallpaper is running, it must know that start is pressed and show a warning
}

void MainWindow::on_pauseButton_clicked()
{   if (!ui->pauseButton->isEnabled()) return;

    if(system("killall -v notify-osd starter mpg321 mpg123 canberra-gtk-play 2> /dev/null")!=256)
        cout << "Error while trying to kill notify-osd, starter, mpg321, mpg123 and canberra-gtk-play";

    if(ui->checkBox->isChecked()) {
        random_image=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomimage"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomimage. Check file's permissions.";
        timer->stop();
    }
    if(ui->checkBox_2->isChecked()){
        random_time=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomtime"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomtime. Check file's permissions.\n";
        ti2->stop();
    }
    ui->pauseButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->action_Next_Image->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->addButton->setEnabled(false); ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop ); ui->addfolder->setEnabled(false); ui->action_Load->setEnabled(false); ui->actionAdd_single_images->setEnabled(false); ui->actionAdd_Folder->setEnabled(false);  ui->removeButton->setEnabled(false); ui->removeallButton->setEnabled(false); ui->moveupButton->setEnabled(false); ui->movedownButton->setEnabled(false);
    ui->startButton->setEnabled(true);
    if (minimize_to_tray==1 )
    {
    Start_action->setEnabled(true);
    Pause_action->setEnabled(false);
    Stop_action->setEnabled(false);
    Next_action->setEnabled(false);
    }
    ui->action_Pause->setEnabled(false);
    ui->action_Start->setEnabled(true);
    ui->actionS_top->setEnabled(false);
    ui->spinBox->setEnabled(true); ui->spinBox_2->setEnabled(true);
    if(ui->listWidget->count() != 0){ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true);}
    if(ui->checkBox_2->isChecked()) {ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false);}
    _start_running_=0; //start stopped
    //The starter script keeps his current picture number to a file, so when it is killed and re-starter
    //(means Start button is pressed) it reads the current image from there (this_to_show_next_time)
    //So, pushing the pause button this value doesn't change, and so the operation starts from where it stopped
}

void MainWindow::on_nextButton_clicked()
{
    if (!ui->nextButton->isEnabled()) return;
    if(system("killall -v starter 2> /dev/null"))
        cout << "Error killing starter\n";

    if(system("/usr/share/wallch/files/Scripts/nextpressed&")){ //starting the nextpressed script.
        cout << "Error while trying to execute ~/.config/Wallch/Scripts/nextpressed. Check file existence and permissions.\n";
    }
    //killing the starter. this is done because we have to update the time. If this is wasn't done, then the next image would be shown with the time left.
    //e.g. if images were changing every 10 secs and we pushed the Next Button at the 6 secs, then the next image would appear for only 4 secs. Killing the starter and then restarting
    //it updates the time (is you see the end of this function you'll see that the starter is started again.
    if(ui->checkBox->isChecked()) {
        random_image=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomimage"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomimage. Check file's permissions.";
        timer->stop();
    }
    if(ui->checkBox_2->isChecked()){
        random_time=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomtime"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomtime. Check file's permissions.\n";
        ti2->stop();
    }
    if(ui->checkBox->isChecked()){
        srand(time(0));
        int r = (rand() % ui->listWidget->count()) + 1;
        //The random image (r) will be written to a file, then the starter will read this file in its startup.
        //If the file contains a number
        char home10[130]="/home/";
        strcat(home10,p);
        char other10[100]="/.config/Wallch/Checks/randomimage";
        strcat(home10,other10);
        if ( r==1 ) r=2;
        ofstream randomimage;
        randomimage.open(home10);
        randomimage << r;
        randomimage.close();
        int a = ui->spinBox->text().toInt();
        int b = a * 60;
        int c = ui->spinBox_2->text().toInt();
        d = b+c;
        int timerint=d * 1000;
        if ( ui->listWidget->count() != 2 )
        timer->start(timerint);}
    if(ui->checkBox_2->isChecked()){
        srand(time(0));
        int rtime = (rand() % 1200) + 121; ////1200 + 121
        //The random time will be written to a file, then the starter will read this file in its startup.
        char home11[130]="/home/";
        strcat(home11,p);
        char other11[100]="/.config/Wallch/Checks/randomtime";
        strcat(home11,other11);
        ofstream randomtime;
        randomtime.open(home11);
        randomtime << rtime;
        randomtime.close();
        ti2->start(120000);}
    int qw = ui->spinBox->text().toInt();
    int er = qw * 60;
    int ty = ui->spinBox_2->text().toInt();
    d = er+ty;
    char file[80]= "/home/";
    char file2[70]="/.config/Wallch/Checks/delay";
    strcat(file,p);
    strcat(file,file2);
    ofstream newFile;
    newFile.open(file);
    newFile << d;
    newFile.close();
    if(system("/usr/share/wallch/files/Scripts/starter normal&")){ //the starter is started again
        cout << "FATAL: Error executing ~/.config/Wallch/Scripts/starter.\na)Check file existence\nb)If exists, place the executable bit\nc)Check for permission to execute the file\n";
    }
}

void MainWindow::on_checkBox_clicked()
{
    if (ui->checkBox->isChecked())
       random_image=1;
    else
    {random_image=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomimage"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomimage. Check file's permissions\n";
    }
}

void MainWindow::write_random_image()
{
    srand(time(0));
    int r = (rand() % ui->listWidget->count()) + 1;
    //The random image (r) will be written to a file, then the starter will read this file in its startup.
    //If the file contains a number
    char home12[130]="/home/";
    strcat(home12,p);
    char other12[100]="/.config/Wallch/Checks/randomimage";
    strcat(home12,other12);
    ofstream randomimage;
    randomimage.open(home12);
    randomimage << r;
    randomimage.close();
}
void MainWindow::write_random_time()
{
    srand(time(0));
    int rtime = (rand() % 1200) + 121;  //1200 + 121
    //The random time will be written to a file, then the starter will read this file in its startup.
    //If the file contains a number
    char home13[130]="/home/";
    strcat(home13,p);
    char other13[100]="/.config/Wallch/Checks/randomtime";
    strcat(home13,other13);
    ofstream randomtime;
    randomtime.open(home13);
    randomtime << rtime;
    randomtime.close();
}

void MainWindow::on_checkBox_2_clicked()
{
    if (ui->checkBox_2->isChecked())
    {random_time=1; ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false);}
    else
    {random_time=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomtime"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomtime. Check file's permissions.\n";
    ui->spinBox->setEnabled(true); ui->spinBox_2->setEnabled(true); }
}

void MainWindow::ShowPreferences()
{
    Preferences = new preferences(this);
    pref=1;
    Preferences->setObjectName("gamatos"); // set an object name so the object can be handily found later
    connect(Preferences,SIGNAL(togreek()),this,SLOT(translatetogreek()));
    connect(Preferences,SIGNAL(toenglish()),this,SLOT(translatetoenglish()));
    connect(Preferences,SIGNAL(traytoenglish()),this,SLOT(translatetraytoenglish()));
    connect(Preferences,SIGNAL(traytogreek()),this,SLOT(translatetraytogreek()));

    Preferences->exec(); //executing the preferences dialog.
    pref=0;
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
        if(!pref){//hide the icon
    trayIcon->hide();
    //show the main window
    this->showNormal();
    if(_language_==2){  //translate.....
        translatetogreek();
        translatetraytogreek();
    }
    else{
        translatetoenglish();
        translatetraytoenglish();
    }
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
    if(_language_==2){
        translatetogreek();
        translatetraytogreek();
    }
    else{
        translatetoenglish();
        translatetraytoenglish();
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (!ui->stopButton->isEnabled()) return;
    if(system("killall -v starter notify-osd constant_on_boot mpg321 mpg123 canberra-gtk-play 2> /dev/null; echo 1 > /home/$USER/.config/Wallch/Checks/this_to_show_next_time")){ //Killing the starter and writing 1 to the file. So, the next time Start is pressed the images will start from the beginning
        cout << "Error while trying to kill starter, notify-osd, constant_on_boot, mpg321, mpg123 and canberra-gtk-play\n";
    }

    if(ui->checkBox->isChecked()) {
        random_image=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomimage"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomimage. Check file's permissions.";
        timer->stop();
    }
    if(ui->checkBox_2->isChecked()){
        random_time=0;
        if(system("echo \"0\" > /home/$USER/.config/Wallch/Checks/randomtime"))
            cout << "Error writing to ~/.config/Wallch/Checks/randomtime. Check file's permissions.\n";
        ti2->stop();
    }
    ui->pauseButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->action_Next_Image->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(true);
    ui->action_Start->setEnabled(true);
    ui->action_Pause->setEnabled(false);
    ui->actionS_top->setEnabled(false);
    if (minimize_to_tray==1 )
    {
    Start_action->setEnabled(true);
    Pause_action->setEnabled(false);
    Stop_action->setEnabled(false);
    Next_action->setEnabled(false);
    }
    ui->addButton->setEnabled(true); ui->listWidget->setDragDropMode(QAbstractItemView::DragDrop ); ui->addfolder->setEnabled(true); ui->action_Load->setEnabled(true); ui->actionAdd_single_images->setEnabled(true); ui->actionAdd_Folder->setEnabled(true);  ui->removeallButton->setEnabled(true); ui->removeButton->setEnabled(true); ui->moveupButton->setEnabled(true); ui->movedownButton->setEnabled(true); ui->spinBox->setEnabled(true); ui->spinBox_2->setEnabled(true);
    if(ui->listWidget->count()!=0){ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true);}
    if(ui->checkBox_2->isChecked()) {ui->spinBox->setEnabled(false); ui->spinBox_2->setEnabled(false);}
    _start_running_=0; //start stopped
}

void MainWindow::menushowabout(){
    if( About->isVisible() )
    {About->raise();
    About->activateWindow();}
      else
      About->show();
}

void MainWindow::checkForUpdate(){
    if(system("ping -c 1 www.google.com > /dev/null")){
        qApp->processEvents();
                    QMessageBox *msgBox = new QMessageBox;
                    if(_language_==2){
                        msgBox->setWindowTitle(QString::fromUtf8("Wallch | Αναβάθμιση"));
                        msgBox->setText(QString::fromUtf8("Υπήρξε ένα πρόβλημα καθώς γινόταν η προσπάθεια <br>σύνδεσης με τον διακομιστή.<br> Παρακαλώ ελέγξτε την σύνδεσή σας στο διαδύκτιο ή δοκιμάστε ξανά σε μερικά λεπτά."));
                    }
                    else
                    {
                        msgBox->setWindowTitle("Wallch | Update");
                        msgBox->setText("There was an error while trying to<br>connect to the server.<br>Please check your internet connection<br> or try again in some minutes.");
                    }
                    msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));
                    msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                    msgBox->setWindowModality(Qt::NonModal);
                    msgBox->show();


                    msgBox->exec();
                    return;
                }
                if(system("cd /home/$USER/.config/Wallch/; wget https://dl.dropbox.com/u/11379868/1 2> /dev/null")){
                    QMessageBox *msgBox = new QMessageBox;if(_language_==2){msgBox->setWindowTitle(QString::fromUtf8("Wallch | Αναβάθμιση"));msgBox->setText(QString::fromUtf8("Υπήρξε ένα πρόβλημα καθώς γινόταν η προσπάθεια <br>σύνδεσης με τον διακομιστή.<br> Παρακαλώ ελέγξτε την σύνδεσή σας στο διαδύκτιο ή δοκιμάστε ξανά σε μερικά λεπτά."));}else{msgBox->setWindowTitle("Wallch | Update");msgBox->setText("There was an error while trying to<br>connect to the server.<br>Please check your internet connection<br> or try again in some minutes.");}msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80))); msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox->setWindowModality(Qt::NonModal);


                    msgBox->exec();
                    return;
                }

                char update[200]="";
                strcat(update,getenv("HOME"));
                strcat(update,"/.config/Wallch/1");
                struct stat stFileInfo;
                int intStat;
                // Attempt to get the file attributes
                intStat = stat(update,&stFileInfo);
                if(intStat != 0) {  //file doesn't exist, an error occured.
                    QMessageBox *msgBox = new QMessageBox;if(_language_==2){msgBox->setWindowTitle(QString::fromUtf8("Wallch | Αναβάθμιση"));msgBox->setText(QString::fromUtf8("Άγνωστο λάθος, προσπαθήστε ξανά σε μερικά λεπτά."));}else{msgBox->setWindowTitle("Wallch | Update");msgBox->setText("Unknown error, try again in some minutes.");}msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80))); msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox->setWindowModality(Qt::NonModal);


                    msgBox->exec();
                    return;
                }
                char update_c[10];
                FILE *update_f = fopen ( update, "r" );
                    if(fgets ( update_c, sizeof update_c, update_f )==NULL)
                        cout << "Error reading ~/.config/Wallch/1, failed to check for new version.\n";
                fclose ( update_f );
                remove(update);
                char version_check[4]="";
                strcat(version_check,version);
                strcat(version_check,"\n");
                if(!strcmp (update_c,version_check)){
                   QMessageBox *msgBox = new QMessageBox; if(_language_==2){ msgBox->setWindowTitle(QString::fromUtf8("Wallch | Αναβάθμιση"));msgBox->setText(QString::fromUtf8("Η τρέχουσα έκδοση του προγράμματος <br>είναι η τελευταία διαθέσιμη.<br> Οπότε το πρόγραμμά σας είναι πλήρης ανανεωμένο")); }else{ msgBox->setWindowTitle("Wallch | Update");msgBox->setText("The current version of this program<br>is the last version available.<br>So, your program is up to date!");}msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox->setWindowModality(Qt::NonModal);


                   msgBox->exec();
                   return;
                }
                QString qoutput = QVariant( update_c ).toString();

                if(system("canberra-gtk-play -f /usr/share/wallch/files/NewVersionOnline.ogg&"))
                    cout << "Could't play ~/.config/Wallch/NewVersionOnline.ogg. Check file existence.\n";
                QMessageBox *msgBox = new QMessageBox;
                if(_language_==2){
                    msgBox->setWindowTitle(QString::fromUtf8("Wallch | Αναβάθμιση"));
                    msgBox->setInformativeText(QString::fromUtf8("Μία καινούργια έκδοση του προγράμματος είναι διαθέσιμη! Αναβάθμιση τώρα;"));
                    msgBox->setText(QString::fromUtf8("<b>Μία καινούργια έκδοση είναι τώρα διαθέσιμη!\n</b>"));
                }
                else
                {
                    msgBox->setWindowTitle("Wallch | Update");
                    msgBox->setInformativeText("A new version of this program is available! Download the new version?");
                    msgBox->setText("<b>Version" + qoutput + ".0 is available!\n</b>");
                }msgBox->setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(100,100)));
                msgBox->setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                QPushButton *updateButton;
                if(_language_==2){
                    updateButton = msgBox->addButton(QString::fromUtf8("Ναι ανανεώστε το πρόγραμμα!"), QMessageBox::ActionRole);
                    msgBox->addButton(QString::fromUtf8("Όχι προτιμώ να μείνω με την παλιά έκδοση..."), QMessageBox::ActionRole);
                }
                else
                {
                    updateButton = msgBox->addButton(tr("Yes, update my app!"), QMessageBox::ActionRole);
                    msgBox->addButton(tr("No, I'll stay with the old one..."), QMessageBox::ActionRole);
                }msgBox->setWindowModality(Qt::NonModal);


                msgBox->exec();
                if (msgBox->clickedButton() == updateButton)
                {
                    if(system("xdg-open http://wallch.t35.com/"))
                        cout << "Error opening http://wallch.t35.com/\n";
                }
                else
                {
                    return;
                }
}

void MainWindow::remove_history(){
    int i = system("history_path=`cat /home/$USER/.config/Wallch/Checks/history_custom_path`; if [ X\"$history_path\" = X\"\" ] ; then ls -l /home/$USER/.config/Wallch/History/History_*days 2> /dev/null | egrep -c '^-' > /dev/null; if [ $? -eq 1 ]; then a=1; else a=0; fi; echo \"/home/$USER/.config/Wallch/History/History_*days\" > /home/$USER/.config/Wallch/Checks/hp; exit $a; else if [ -s \"$history_path\" ]; then ls -l \"$history_path\"/History_*days 2> /dev/null | egrep -c '^-' > /dev/null; if [ $? -eq 1 ]; then a=1; else a=0; fi; echo \"$history_path\"/History_*days > /home/$USER/.config/Wallch/Checks/hp; exit $a; else ls -l /home/$USER/.config/Wallch/History/History_*days 2> /dev/null | egrep -c '^-' > /dev/null; if [ $? -eq 1 ]; then a=1; else a=0; fi; echo \"/home/$USER/.config/Wallch/History/History_*days\" > /home/$USER/.config/Wallch/Checks/hp; exit $a; fi; fi");
    if(i){ //there are no files into the history's directory
    QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ιστορία"));msgBox.setText(QString::fromUtf8("<b>Δεν βρέθηκε ιστορικό.</b>"));msgBox.setInformativeText(QString::fromUtf8("Έτσι δεν διαγράφτηκε κανένα αρχείο. Αν θέλετε να ενεργοποιήσετε την αποθήκευση ιστορικού μπορείτε να το κάνετε από τις ρυθμίσεις."));}else{msgBox.setWindowTitle("Wallch | History");msgBox.setText("<b>No history was found.</b>");msgBox.setInformativeText("So no files have been deleted. If you want to enable history logging you can enable it from the preferences.");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/time_machine.png").pixmap(QSize(100,100))); msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();return;
    }
if(system("a=\"`cat /home/$USER/.config/Wallch/Checks/hp`\"; rm -rf \"$a\"; rm -rf /home/$USER/.config/Wallch/Checks/hp"))
    cout << "Error removing history(~/.config/Wallch/History/History_*days). Check folder's permissions.\n";
QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ιστορία"));msgBox.setText(QString::fromUtf8("<b>Η ιστορία έχει διαγραφτεί.</b>"));msgBox.setInformativeText(QString::fromUtf8("Εάν δεν θέλετε να αποθηκεύετε την ιστορία, μπορείτε να την απενεργοποιήσετε από τις ρυθμίσεις.."));}else{msgBox.setWindowTitle("Wallch | History");msgBox.setText("<b>History has been deleted.</b>");msgBox.setInformativeText("If you don't want to save history, you can disable it from the preferences.");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/kalarm_disabled.png").pixmap(QSize(100,100)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();
}

void MainWindow::on_addButton_clicked()
{
    if (!ui->addButton->isEnabled()) return;
    if (minimize_to_tray) add=1;
    QStringList path;
    if(_language_==2)
        path = QFileDialog::getOpenFileNames(this, QString::fromUtf8("Wallch | Διάλεξε εικόνες"), QDir::homePath(), tr("*png *gif *bmp *jpg *jpeg"));
    else
        path = QFileDialog::getOpenFileNames(this, tr("Wallch | Choose Pictures"), QDir::homePath(), tr("*png *gif *bmp *jpg *jpeg"));
    add=0;
    if(path.count() == 0) return;
    int count=0;
    while (count < path.count()){
        QString qstr = path[count];
                   QListWidgetItem *item = new QListWidgetItem;
                   item->setText(qstr);
                   if(_language_==2)
                      item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                   else
                      item->setToolTip("Double-click to set this item as Background...");
                   ui->listWidget->addItem(item);
                   count++;
                            }

           if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->checkBox_2->setEnabled(false); ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
           else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
           if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);

}

void MainWindow::on_removeButton_clicked()
{
     if (ui->listWidget->count()==1) ui->listWidget->clear(); else delete ui->listWidget->currentItem();
     if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 )Start_action->setEnabled(false);}
     else {ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
     if ( ui->listWidget->count() == 0) ui->label_9->clear();
     if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);
     if(ui->listWidget->count()==0 || ui->listWidget->count()==1) ui->checkBox_2->setEnabled(false);

}

void MainWindow::on_removeallButton_clicked()
{
    ui->listWidget->clear();
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->action_Start->setEnabled(false);
    ui->actionS_top->setEnabled(false);
    ui->action_Pause->setEnabled(false);
    if (minimize_to_tray==1 )
    {Start_action->setEnabled(false);
    Stop_action->setEnabled(false);
    Pause_action->setEnabled(false);}
    ui->label_9->clear();
    ui->checkBox->setEnabled(false);
    ui->checkBox_2->setEnabled(false);
    ui->checkBox_2->setEnabled(false);
}

void MainWindow::on_moveupButton_clicked()
{
    if( ui->listWidget->selectionModel()->hasSelection() ){
        if( ui->listWidget->currentIndex().row() != 0 ){
            int row = ui->listWidget->currentIndex().row();
            QModelIndex currentIndex = ui->listWidget->currentIndex();
            QModelIndex indexAbove = ui->listWidget->model()->index( row-1, 0, QModelIndex() );

            QString selectedString = ui->listWidget->currentIndex().data().toString();
            ui->listWidget->model()->setData( currentIndex, indexAbove.data().toString() );
            ui->listWidget->model()->setData( indexAbove, selectedString );
            ui->listWidget->setCurrentIndex( indexAbove );
        }
    }
}

void MainWindow::on_movedownButton_clicked()
{
    if( ui->listWidget->selectionModel()->hasSelection() ){
        if( ui->listWidget->currentIndex().row() != ui->listWidget->model()->rowCount()-1 ){
            int row = ui->listWidget->currentIndex().row();
            QModelIndex currentIndex = ui->listWidget->currentIndex();
            QModelIndex indexBelow = ui->listWidget->model()->index( row+1, 0, QModelIndex() );

            QString selectedString = ui->listWidget->currentIndex().data().toString();
            ui->listWidget->model()->setData( currentIndex, indexBelow.data().toString() );
            ui->listWidget->model()->setData( indexBelow, selectedString );
            ui->listWidget->setCurrentIndex( indexBelow );
        }
    }
}

void MainWindow::on_listWidget_itemSelectionChanged()
{
    if(preview){
        QString filename = QString::fromUtf8(ui->listWidget->item(ui->listWidget->currentRow())->text().toUtf8());
        ui->label_9->setBackgroundRole(QPalette::Base);
        ui->label_9->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QImage image(filename);
        ui->label_9->setPixmap(QPixmap::fromImage(image));
        ui->label_9->setScaledContents(true);
    }
    }

void MainWindow::on_listWidget_itemDoubleClicked()
{
    if (ui->startButton->isEnabled() || ui->listWidget->count() == 1)
    { QString filename2 = ui->listWidget->item(ui->listWidget->currentRow())->text();
    char home16[100]="/home/";
    char other16[40]="/.config/Wallch/Checks/double";
    strcat (home16,p);
    strcat (home16,other16);
    QFile file9( home16 );

    if( file9.open( QIODevice::WriteOnly ) ) {
      // file opened and is overwriten with WriteOnly
      QTextStream textStream( &file9 );
      textStream << filename2;
      file9.close();
    }
    if(system("/usr/share/wallch/files/Scripts/double_click&"))
        cout << "Error executing ~/.config/Wallch/Scripts/double_click. Check file existence and permissions.\n";
}
    else errorstopprocess();
}

void MainWindow::doubleonrightclick()
{
    if(!ui->listWidget->currentItem()->isSelected())return;
    if (ui->startButton->isEnabled() || ui->listWidget->count() == 1)
    { QString filename2 = ui->listWidget->item(ui->listWidget->currentRow())->text();
    char home17[100]="/home/";
    char other17[100]="/.config/Wallch/Checks/double";
    strcat (home17,p);
    strcat (home17,other17);
    QFile file9( home17 );

    if( file9.open( QIODevice::WriteOnly ) ) {
      // file opened and is overwriten with WriteOnly
      QTextStream textStream( &file9 );
      textStream << filename2;
      file9.close();
    }
    if(system("/usr/share/wallch/files/Scripts/double_click&"))
        cout << "Error executing ~/.config/Wallch/Scripts/double_click. Check file existence and permissions.\n";
}
else errorstopprocess();
}

void MainWindow::on_screenshotButton_clicked()
{
    if(screen_is_on){ //screenshot dialog is running so point to it
        Screenshot->raise();
        Screenshot->activateWindow();
        return;
        }
    screen_is_on=1; //Tell that screenshot starts running
         Screenshot = new screenshot(this);
         Screenshot->exec();
         screen_is_on=0; //Tell that screenshot stops running
}

void MainWindow::bug(){
    if(system("xdg-open https://bugs.launchpad.net/wallpaper-changer"))
        cout << "Error opening https://bugs.launchpad.net/wallpaper-changer\n";
}

void MainWindow::remove_list(){
    //if the pic exists then we would be able to get its attributes, don't we? ;)
    if (ui->startButton->isEnabled() || ui->listWidget->count() == 1 ){
    struct stat stFileInfo;
    int intStat;
    int i=0;
    while ( i < ui->listWidget->count()) {
    int pics = ui->listWidget->count();
    QString qpic = ui->listWidget->item(i)->text().toUtf8();
    const char *pic= qpic.toLatin1();
    // Attempt to get the file attributes
    intStat = stat(pic,&stFileInfo);
    if(intStat != 0) { //image doesn't exist. Delete the item of it.
    delete ui->listWidget->item(i);
    }
    if(pics!=ui->listWidget->count()) i=0; else ++i; //If e.g. the first file has been deleted (i=0) then someother file will become item(0) but the i will be 1 and the 0 file will not be checked, so, if a file was removed do the i again 0
    //in order to check it as well and not skip it!
    if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);
}
}else {QMessageBox msgBox;
    if(_language_==2){
        msgBox.setText(QString::fromUtf8("Για να το κάνετε αυτό σταματήστε την διαδικασία"));}
    else{msgBox.setText("In order to do that stop the process!");}
msgBox.setWindowTitle(QString::fromUtf8("Error!"));
msgBox.setIconPixmap(QIcon(":/icons/Pictures/info.png").pixmap(QSize(56,56)));
msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
msgBox.exec(); }
    if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
    if(ui->listWidget->count()==0 || ui->listWidget->count()==1) ui->checkBox_2->setEnabled(false);

}

void MainWindow::remove_disk(){
    if (ui->startButton->isEnabled() || ui->listWidget->count() == 1 ){QMessageBox msgBox2;
    QPushButton *Yes;
    QPushButton *No;
    if(_language_==2){
        msgBox2.setWindowTitle(QString::fromUtf8("Wallch | Διαγραφή εικόνας"));
        msgBox2.setInformativeText(QString::fromUtf8("Η εικόνα που έχετε επιλέξει θα διαγραφτεί από τον δίσκο σας. Είστε σίγουροι;"));
        msgBox2.setText(QString::fromUtf8("<b>Διαγραφή εικόνας.</b>"));
        Yes = msgBox2.addButton(QString::fromUtf8("Ναι"), QMessageBox::ActionRole);
        No = msgBox2.addButton(QString::fromUtf8("Όχι"), QMessageBox::ActionRole);}
    else
    {
        msgBox2.setWindowTitle("Wallch | Image Deletion");
        msgBox2.setInformativeText("The image you selected will be permanently deleted from your hard disk. Are you sure?");
        msgBox2.setText("<b>Image Deletion.</b>");
        Yes = msgBox2.addButton("Yes", QMessageBox::ActionRole);
        No = msgBox2.addButton("No", QMessageBox::ActionRole);}
    msgBox2.setIconPixmap(QIcon(":/icons/Pictures/Trash.png").pixmap(QSize(100,100)));
    msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
        msgBox2.exec();
        if (msgBox2.clickedButton() == Yes){
            int curr_ind = ui->listWidget->currentRow();
            QString qto_remove = ui->listWidget->item(curr_ind)->text().toUtf8();
            const char *to_remove = qto_remove.toLatin1().data();
                       if( remove( to_remove ) != 0 ){
                QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Διαγραφή εικόνας"));msgBox.setText(QString::fromUtf8("Η διαγραφή της εικόνας απέτυχε! Πιθανοί λόγοι είναι: α) Η εικόνα δεν υπάρχει ή β) Δεν έχετε την άδεια να διαγράψετε την εικόνα"));}else{msgBox.setWindowTitle("Wallch | Image Deletion");msgBox.setText("Image deletion failed! possible reasons are: a) Image doesn't exist or b) You don't have the permission to delete the image");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();
                } else {
                    if(!ui->listWidget->count()) return;
                    if(ui->listWidget->count()==1) {ui->listWidget->clear(); ui->label_9->clear(); return;}
                    if(ui->listWidget->currentItem()->isSelected()) delete ui->listWidget->currentItem();
                }
        } else return;
    }
        else
    {QMessageBox msgBox; if(_language_==2){ msgBox.setText(QString::fromUtf8("Για να το κάνετε αυτό σταματήστε την διαδικασία"));}else{msgBox.setText("In order to do that stop the process!");} msgBox.setWindowTitle(QString::fromUtf8("Error!")); msgBox.setIconPixmap(QIcon(":/icons/Pictures/info.png").pixmap(QSize(56,56)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec(); }

        if(ui->listWidget->count()==1 || ui->listWidget->count()==0 ) {ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 )Start_action->setEnabled(false);}
        if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);
        if(ui->listWidget->count()==0 || ui->listWidget->count()==1) ui->checkBox_2->setEnabled(false);
}

void MainWindow::load(){
    if (!ui->action_Load->isEnabled()) return;
    QString Qpath;

    if(_language_==2)
        Qpath = QFileDialog::getOpenFileName(this, QString::fromUtf8("Wallch | Διάλεξε Άλμπουμ"), QDir::homePath(), tr("*.wallch"));
    else
        Qpath = QFileDialog::getOpenFileName(this, tr("Wallch | Choose Album"), QDir::homePath(), tr("*.wallch"));

    if(Qpath.count() == 0) return; else ui->label_9->clear();
    char *path = Qpath.toUtf8().data();

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
                                    if(_language_==2)
                                        item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                                    else
                                        item->setToolTip("Double-click to set this item as Background...");
                   ui->listWidget->addItem(item);
               }

           }
               fclose ( file );
               if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
               else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
}

void MainWindow::on_listWidget_customContextMenuRequested()
{
    if(!ui->listWidget->currentIndex().isValid()) return;
    QMenu menu;
    if(_language_==2){
        menu.addAction(QString::fromUtf8("Ορισμός εικόνας ως Background"),this,SLOT(doubleonrightclick()));
        menu.addAction(QString::fromUtf8("Απομάκρυνση εικόνων που δεν υπάρχουν"),this,SLOT(remove_list()));
        menu.addAction(QString::fromUtf8("Διαγραφή εικόνας από τον δίσκο"),this,SLOT(remove_disk()));
        menu.addAction(QString::fromUtf8("Ιδιότητες"),this,SLOT(showProperties()));}
    else{
        menu.addAction("Set this item as Background",this,SLOT(doubleonrightclick()));
        menu.addAction("Remove non-existent Pictures",this,SLOT(remove_list()));
        menu.addAction("Delete image from disk",this,SLOT(remove_disk()));
        menu.addAction("Properties",this,SLOT(showProperties()));}
    menu.exec(QCursor::pos());

}

void MainWindow::showProperties()
{
        if(!ui->listWidget->currentItem()->isSelected())return;
        struct stat stFileInfo;
        int intStat;
        char home18[200]="/home/";
        strcat(home18,p);
        char oth[70]="/.config/Wallch/Checks/image_path";
        strcat(home18,oth);
        QFile file9 ( home18 );

        if( file9.open( QIODevice::WriteOnly ) ) {
          // file opened and is overwriten with WriteOnly
          QTextStream textStream( &file9 );
          textStream << QString::fromUtf8(ui->listWidget->currentItem()->text().toUtf8());
          file9.close();
        }

        QString qpic1 = ui->listWidget->currentItem()->text().toUtf8();
        const char *pic1= qpic1.toLatin1();
        char wholepic[300]="";
        strcat(wholepic,pic1);

        // Attempt to get the file attributes
        intStat = stat(wholepic,&stFileInfo);
        if(intStat != 0 && !properties_times) { //Dublicate the check! Sometimes turns buggy
            //properties_times is 0 at start. After the first check it comes here and turns 1 then the function reruns and if again the image is not found it displays an error. This achieves a dublicate check.
            exists=0;
           properties_times=1; MainWindow::showProperties();}
        if(intStat != 0 && properties_times) { //image doesn't exist. Display Error
            exists=0;
            properties_times=0;
            QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ιδιότητες"));msgBox.setText(QString::fromUtf8("Το αρχείο αυτό μάλλον δεν υπάρχει. Παρακαλώ κάντε έναν έλεγχο για το αρχείο και δοκιμάστε ξανά."));}else{msgBox.setWindowTitle("Wallch | Properties");msgBox.setText("This file maybe doesn't exist. Please perform a check for the file and try again.");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/wallpaper.png").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();
                if(system("rm -rf /home/$USER/.config/Wallch/Checks/image_property_path"))
                    cout << "Error removing ~/.config/Wallch/Checks/image_property_path. Check file's permissions.\n";
                return;
            } else{ //image exists, open the properties window.
                if(exists){
                if( Properties->isVisible() )
                               {Properties->close();
                    Properties = new properties( this );
                                                         Properties->show();}
                                 else
                               {Properties = new properties( this );
                                     Properties->show();}
                             }
}
            exists=1;
        }

void MainWindow::on_preview_clicked()
{
    if(ui->preview->isChecked()){
        preview=1;
        if(ui->listWidget->count()!=0 && ui->listWidget->currentRow()!=-1){
        QString filename = ui->listWidget->item(ui->listWidget->currentRow())->text().toAscii();
        ui->label_9->setBackgroundRole(QPalette::Base);
        ui->label_9->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QImage image(filename);
        ui->label_9->setPixmap(QPixmap::fromImage(image));
        ui->label_9->setScaledContents(true);}
    }
    else
    {
        preview=0;
        ui->label_9->clear();
    }
}

void MainWindow::on_webcamButton_clicked()
{
    QMessageBox msgBox;if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Wallch | Webcam"));msgBox.setText(QString::fromUtf8("<b>Διαθέσιμο στην επόμενη<br>έκδοση του προγράμματος</b>")); }else{msgBox.setWindowTitle("Wallch | Webcam");msgBox.setText("<b>This function will be available<br>at the next version</b>");}msgBox.setIconPixmap(QIcon(":/icons/Pictures/Webcam.png").pixmap(QSize(64,64)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();return;
}

void MainWindow::Get_Help_Online()
{
    if(system("xdg-open https://answers.launchpad.net/wallpaper-changer/+addquestion"))
        cout << "Error opening https://answers.launchpad.net/wallpaper-changer/+addquestion\n";
}

void MainWindow::on_spinBox_2_valueChanged(int )
{
    if(ui->spinBox_2->value() >= 60){
        ui->spinBox_2->setValue(0);
        ui->spinBox->setValue(ui->spinBox->value()+1);
    }
    if(ui->spinBox_2->value() < 0){
        if(!ui->spinBox->value()){
            ui->spinBox_2->setValue(0);
            return;
        }
        ui->spinBox_2->setValue(59);
        ui->spinBox->setValue(ui->spinBox->value()-1);
    }
    if(ui->spinBox_2->value() < 10 && ui->spinBox->value()==0){
            ui->spinBox_2->setMaximum(10);
            ui->spinBox_2->setValue(10);
    }else
        ui->spinBox_2->setMaximum(60);
}


void MainWindow::on_spinBox_valueChanged(int )
{
    if(ui->spinBox->value()==0 && ui->spinBox_2->value() < 10){
       ui->spinBox_2->setMaximum(10);
       ui->spinBox_2->setValue(10);
 }
    else
        ui->spinBox_2->setMaximum(60);
}

void MainWindow::translatetogreek()
{ui->menu_File->setTitle(QString::fromUtf8("&Αρχείο"));
    ui->menu_Edit->setTitle(QString::fromUtf8("&Επεξεργασία"));
    ui->menuHelp->setTitle(QString::fromUtf8("&Βοήθεια"));
    ui->menuAdd_Files->setTitle(QString::fromUtf8("Πρόσθεσε αρχεία"));
    ui->label_2->setText(QString::fromUtf8("Άλλαξε background κάθε"));
    ui->label_3->setText(QString::fromUtf8("λεπτά και"));
    ui->label_4->setText(QString::fromUtf8("δευτερόλεπτα"));
    ui->checkBox->setText(QString::fromUtf8("Άλλαξε Wallpaper επιλέγοντας τυχαία εικόνες..."));
    ui->checkBox_2->setText(QString::fromUtf8("Άλλαξε Wallpaper σε τυχαίο χρόνο (2-20 λεπτά)"));
    ui->preview->setText(QString::fromUtf8("Προεπισκόπηση"));
    ui->nextButton->setText(QString::fromUtf8("Προχώρα στην επόμε&νη εικόνα"));
    ui->startButton->setText(QString::fromUtf8("Έ&ναρξη"));
    ui->pauseButton->setText(QString::fromUtf8("&Παύση"));
    ui->stopButton->setText(QString::fromUtf8("&Σταμάτημα"));
    ui->moveupButton->setText(QString::fromUtf8("Πάνω"));
    ui->movedownButton->setText(QString::fromUtf8("Κάτω"));
    ui->removeallButton->setText(QString::fromUtf8("Καθαρισμός"));
    ui->removeButton->setText(QString::fromUtf8("Διαγραφή"));
    ui->addButton->setText(QString::fromUtf8("Πρόσθεση"));
    ui->addfolder->setText(QString::fromUtf8("Φάκελος"));
    ui->screenshotButton->setText(QString::fromUtf8("Βγάλε Screenshot"));
    ui->webcamButton->setText(QString::fromUtf8("Βγάλε Webcam-Εικόνα"));
    ui->action_Preferences->setText(QString::fromUtf8("Ρυθμίσεις                                   Ctrl+P"));
    ui->action_Remove_History->setText(QString::fromUtf8("Διαγραφή ιστορικού           Ctrl+R"));
    ui->actionQuit_Ctrl_Q->setText(QString::fromUtf8("Έξοδος                                       Ctrl+Q"));
    ui->action_Start->setText(QString::fromUtf8("Έναρξη                          Shift+Ctrl+S"));
    ui->actionS_top->setText(QString::fromUtf8("Σταμάτημα                 Shift+Ctrl+Ο"));
    ui->action_Load->setText(QString::fromUtf8("Πρόσθεσε ένα Wallch album     Ctrl+O"));
    ui->actionAdd_single_images->setText(QString::fromUtf8("Πρόσθεσε εικόνες                           Ctrl+I"));
    ui->actionAdd_Folder->setText(QString::fromUtf8("Πρόσθεσε φάκελο                           Ctrl+F"));
    ui->action_Pause->setText(QString::fromUtf8("Παύση                           Shift+Ctrl+P"));
    ui->action_Next_Image->setText(QString::fromUtf8("Επόμενη εικόνα        Shift+Ctrl+Ν"));
    ui->action_Check_For_Updates->setText(QString::fromUtf8("Έλεγχος Για Ενημερώσεις       Ctrl+U"));
    ui->actionContents->setText(QString::fromUtf8("Περιεχόμενα                                   F1"));
    ui->actionReport_A_Bug->setText(QString::fromUtf8("Στείλε Bug"));
    ui->action_About->setText(QString::fromUtf8("Σχετικά"));
    ui->actionGet_Help_Online->setText(QString::fromUtf8("Διαδυκτιακή Βοήθεια"));
    ui->startButton->setToolTip(QString::fromUtf8("Ξεκίνημα λειτουργίας"));
    ui->pauseButton->setToolTip(QString::fromUtf8("Παύση λειτουργίας"));
    ui->stopButton->setToolTip(QString::fromUtf8("Σταμάτημα λειτουργίας"));
    ui->nextButton->setToolTip(QString::fromUtf8("Προχώρα στην επόμενη εικόνα..."));
    ui->addButton->setToolTip(QString::fromUtf8("Πρόσθεσε εικόνες"));
    ui->removeButton->setToolTip(QString::fromUtf8("Απομάκρυνση εικόνας"));
    ui->removeallButton->setToolTip(QString::fromUtf8("Απομάκρυνση όλων των εικόνων"));
    ui->movedownButton->setToolTip(QString::fromUtf8("Μετακίνηση εικόνας προς τα κάτω"));
    ui->moveupButton->setToolTip(QString::fromUtf8("Μετακίνηση εικόνας προς τα πάνω"));
    ui->preview->setToolTip(QString::fromUtf8("Προβολή επιλεγμένης εικόνας στην οθόνη"));
    ui->screenshotButton->setToolTip(QString::fromUtf8("Πάρε Screenshot του Desktop σου"));
    ui->addfolder->setToolTip(QString::fromUtf8("Προσθήκη όλων των εικόνων ενός φακέλου και των υποφακέλων του"));
    if(minimize_to_tray)
        trayIcon->setToolTip(QString::fromUtf8("<table width='300'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Αριστερό κλικ: Εμφάνιση του παραθύρου αν η διεργασία δεν τρέχει/Επόμενη εικόνα αν η διεργασία τρέχει.</td></tr> <tr><td nowrap align='left'>Μεσαίο κλικ: Έξοδο εφαρμογής.</td> </tr> <tr><td nowrap align='left'>Δεξί κλικ: Δείξε μερικές επιλογές.</td> </tr></table>"));
}

void MainWindow::translatetoenglish()
{ui->addButton->setText("Add");
    ui->addButton->setToolTip("Add images");
    ui->addfolder->setText("Add Folder");
    ui->removeButton->setText("Remove");
    ui->removeButton->setToolTip("Remove images");
    ui->removeallButton->setText("Clear");
    ui->removeallButton->setToolTip("Remove all images from list");
    ui->moveupButton->setText("Move up");
    ui->moveupButton->setToolTip("Move image up");
    ui->movedownButton->setText("Move down");
    ui->movedownButton->setToolTip("Move image down");
    ui->label_2->setText("Change Background Every:");
    ui->label_3->setText("minutes and");
    ui->label_4->setText("seconds");
    ui->checkBox->setText("Change Wallpaper choosing images randomly...");
    ui->checkBox_2->setText("Change Wallpaper image in random time (2-20 minutes)");
    ui->nextButton->setText("Move to &Next image");
    ui->startButton->setText("&Start");
    ui->startButton->setToolTip("Start Operation");
    ui->pauseButton->setText("&Pause");
    ui->pauseButton->setToolTip("Pause Operation");
    ui->stopButton->setText("St&op");
    ui->stopButton->setToolTip("Stop Operation");
    ui->preview->setText("Preview");
    ui->preview->setToolTip("Preview of selected image on screen");
    ui->webcamButton->setText("Take Webcam-Image");
    ui->screenshotButton->setText("Take Screenshot");
    ui->screenshotButton->setToolTip("Take Desktop Screenshot");
    ui->addfolder->setToolTip("Add all images from a folder and its subfolders");
    for (int i = 0; i < ui->listWidget->count(); ++i)
        ui->listWidget->item(i)->setToolTip("Double-click to set this item as Background");
    if(minimize_to_tray)
        trayIcon->setToolTip("<table width='280'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Left click: Show window if process is not running/Next image if process is running.</td></tr> <tr><td nowrap align='left'>Middle click: Exit the application.</td> </tr> <tr><td nowrap align='left'>Right click: Show some options.</td> </tr></table>");
    ui->menu_File->setTitle("&File");
    ui->menu_Edit->setTitle("&Edit");
    ui->menuHelp->setTitle("&Help");
    ui->menuAdd_Files->setTitle("Add files");
    ui->action_Start->setText("&Start                        Shift+Ctrl+S");
    ui->action_Pause->setText("&Pause                      Shift+Ctrl+P");
    ui->actionS_top->setText("S&top                         Shift+Ctrl+O");
    ui->action_Next_Image->setText("&Next Image         Shift+Ctrl+N");
    ui->action_Load->setText("Add a Wallch Album         Ctrl+O");
    ui->actionAdd_single_images->setText("Add single images             Ctrl+I");
    ui->actionAdd_Folder->setText("Add Folder                             Ctrl+F");
    ui->actionQuit_Ctrl_Q->setText("Quit                                       Ctrl+Q");
    ui->action_Preferences->setText("&Preferences                             Ctrl+P");
    ui->action_Remove_History->setText("&Remove Saved History      Ctrl+R");
    ui->action_Check_For_Updates->setText("Check For &Updates          Ctrl+U");
    ui->actionContents->setText("Contents                                F1");
    ui->actionGet_Help_Online->setText("Get Help Online");
    ui->actionReport_A_Bug->setText("Report A Bug");
    ui->action_About->setText("&About");


}

void MainWindow::translatetraytoenglish(){
trayIcon->setToolTip("<table width='280'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Left click: Show window if process is not running/Next image if process is running.</td></tr> <tr><td nowrap align='left'>Middle click: Exit the application.</td> </tr> <tr><td nowrap align='left'>Right click: Show some options.</td> </tr></table>");
Show_action->setText("S&how");
Start_action->setText("&Start");
Pause_action->setText("&Pause");
Stop_action->setText("&Stop");
Next_action->setText("&Next Image");
Settings_action->setText("P&references");
About_action->setText("&About");
Quit_action->setText("&Quit");
}

void MainWindow::translatetraytogreek(){
trayIcon->setToolTip(QString::fromUtf8("<table width='300'><tr><td nowrap align='left'> <b>Wallch</b> </td> </tr><tr> <td nowrap align='left'>Αριστερό κλικ: Εμφάνιση του παραθύρου αν η διεργασία δεν τρέχει/Επόμενη εικόνα αν η διεργασία τρέχει.</td></tr> <tr><td nowrap align='left'>Μεσαίο κλικ: Έξοδο εφαρμογής.</td> </tr> <tr><td nowrap align='left'>Δεξί κλικ: Δείξε μερικές επιλογές.</td> </tr></table>"));
Show_action->setText(QString::fromUtf8("Επ&αναφορά"));
Start_action->setText(QString::fromUtf8("&Έναρξη"));
Pause_action->setText(QString::fromUtf8("&Παύση"));
Stop_action->setText(QString::fromUtf8("&Σταμάτα"));
Next_action->setText(QString::fromUtf8("Επόμεν&η εικόνα"));
Settings_action->setText(QString::fromUtf8("&Ρυθμίσεις"));
About_action->setText(QString::fromUtf8("Περί του προ&γράμματος"));
Quit_action->setText(QString::fromUtf8("Έξ&οδος"));
}

void MainWindow::errorstopprocess()
{ if(_language_==2){ QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText(QString::fromUtf8("Για να aλλάξετε το τρέχον Wallpaper της Επιφάνειας Εργασίας σας, παρακαλώ σταματήστε την διεργασία."));msgBox.setIconPixmap(QIcon(":/icons/Pictures/info.png").pixmap(QSize(56,56)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();}
    else{QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText("In order to change the current Desktop Wallpaper, please stop the current process.");msgBox.setIconPixmap(QIcon(":/icons/Pictures/info.png").pixmap(QSize(56,56)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));msgBox.exec();}
}

void MainWindow::on_addfolder_clicked()
{
    if (!ui->addfolder->isEnabled()) return;
    if (minimize_to_tray) add=1;
    QString qpath;
    if(_language_==2)
        qpath = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Wallch | Διάλεξε Φάκελο"), QDir::homePath());
    else
        qpath = QFileDialog::getExistingDirectory(this, tr("Wallch | Choose Folder"), QDir::homePath());
    add=0;
    if(qpath.count() == 0) return;
    char home17[100]="/home/";
    strcat(home17,p);
    char other14[40]="/.config/Wallch/Checks/add_folder";
    strcat(home17,other14);
    QFile file9( home17 );

    if( file9.open( QIODevice::WriteOnly ) ) {
      // file opened and is overwriten with WriteOnly
      QTextStream textStream( &file9 );
      textStream << qpath;
      file9.close();
    }
    //find pictures...
    if(system("find \"$(cat ~/.config/Wallch/Checks/add_folder)\" \\( -name \"*.png\" -o -name \"*.gif\" -o -name \"*.bmp\" -o -name \"*jpg\" -o -name \"*jpeg\"  -o -name \"*PNG\" -o -name \"*.GIF\" -o -name \"*.BMP\" -o -name \"*JPG\" -o -name \"*JPEG\" \\) -print > ~/.config/Wallch/Checks/add_folder_pics"))
        cout << "Error while trying to use command 'find' into the specified folder! Check folder's permissions etc.\n";
    strcat(home17,"_pics");
    //add files to listwidget
    FILE *file = fopen ( home17, "r" );
           if ( file != NULL )
           {
               ifstream file( home17 ) ;
               string line ;
               while( std::getline( file, line ) )
               {
                   QString qstr = QString::fromUtf8(line.c_str());
                   QListWidgetItem *item = new QListWidgetItem;
                                    item->setText(qstr);
                                    if(_language_==2)
                                        item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                                    else
                                        item->setToolTip("Double-click to set this item as Background...");
                   ui->listWidget->addItem(item);
               }

           }
               fclose ( file );
               if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
               else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
               //remove temp files used...
               if(system("rm -rf ~/.config/Wallch/Checks/add_fodler*"))
                   cout << "Error while trying to remove ~/.config/Wallch/Checks/add_fodler* .Please check file(s) permissions!";
}

void MainWindow::close_minimize(){
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
                                           if(_language_==2)
                                               item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                                           else
                                               item->setToolTip("Double-click to set this item as Background...");
                          ui->listWidget->addItem(item);
                      }
                  }
                  fclose ( file );
       }
    }
    if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->checkBox_2->setEnabled(false); ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
    else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
    if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);
        event->acceptProposedAction();
}

void MainWindow::on_actionContents_triggered()
{
    if(system("yelp file:///usr/share/gnome/help/wallch/C/wallch.xml 2> /dev/null&"))
        cout << "ERROR opening /usr/share/gnome/help/wallch/C/wallch.xml with yelp! Check for file existence and/or for /usr/bin/yelp\n";
}

void MainWindow::album_slot(const QString &album){//QDBus signal handling
    if(album=="FOCUS"){ //another application instance has come, so focus to the already running process!
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
    else if(album=="NEW_ALBUM"){
        if (!ui->addButton->isEnabled()) return;
        char new_albums[100]="/home/";
        char other_new_albums[100]="/.config/Wallch/Checks/new_albums";
        strcat(new_albums,p);
        strcat(new_albums,other_new_albums);
        FILE *file = fopen ( new_albums, "r" );
               if ( file != NULL )
               {
                   ifstream file( new_albums ) ;
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
                                                       if(_language_==2)
                                                           item->setToolTip(QString::fromUtf8("Διπλό κλικ για ορισμό εικόνας ως Background..."));
                                                       else
                                                           item->setToolTip("Double-click to set this item as Background...");
                                      ui->listWidget->addItem(item);
                                  }

                              }
                   }
               }
               if ( ui->listWidget->count() == 0 || ui->listWidget->count() == 1 ){ui->checkBox_2->setEnabled(false); ui->startButton->setEnabled(false); ui->action_Start->setEnabled(false); if (minimize_to_tray==1 ) Start_action->setEnabled(false);}
               else {ui->checkBox->setEnabled(true); ui->checkBox_2->setEnabled(true); ui->startButton->setEnabled(true); ui->action_Start->setEnabled(true); if (minimize_to_tray==1 )Start_action->setEnabled(true);}
               if(ui->listWidget->count()<=2) ui->checkBox->setEnabled(false);
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
}
