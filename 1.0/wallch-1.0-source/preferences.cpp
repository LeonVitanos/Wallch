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

#include "preferences.h"
#include "ui_preferences.h"
#include "glob.h"

#include <QSettings>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QShortcut>

#include <sys/stat.h>
#include <iostream>
#include <fstream>

using namespace std;

int minimize_already_checked=0;
int album=0;

char *py=getenv("USER");
char other[35]="/.config/Wallch/Settings";


preferences::preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preferences)
{
    ui->setupUi(this);
    QSettings settings( "Wallch", "PreferencesWallpaperChanger" );
                    ui->browsesoundButton->setText( settings.value( "browsesoundButton" ).toString() );
                    ui->linehistory->setText( settings.value( "linehistory" ).toString());
                    ui->lineEdit->setText( settings.value( "lineEdit" ).toString());

    //checking to see if the checkbox referring to the wallpaper change on boot is enabled
    struct stat stFileInfo;
    int intStat;
    char home1[150]="/home/";
    char other1[100]="/.config/autostart/WallpaperChangerOnBoot.desktop";
    strcat(home1,py);
    strcat(home1,other1);
    intStat = stat(home1,&stFileInfo);
    if(intStat == 0)
        ui->radioButton->setChecked(true);
    //checking to see if the checkbox referring to the constant wallpaper changing on boot will be checked or not
    struct stat stFileInfo2;
    int intStat2;
    char home2[150]="/home/";
    char other2[100]="/.config/autostart/WallpaperChangerOnBootConstantApp.desktop";
    strcat(home2,py);
    strcat(home2,other2);
    intStat2 = stat(home2,&stFileInfo2);
    if(intStat2 == 0)
        ui->radioButton_2->setChecked(true);

    //checking either the minimize to tray checkbox will be checked or not.
    char home3[150]="/home/";
    strcat(home3,py);
    strcat(home3,other);

    char tray[128];
    int languagei35;
       FILE *file45 = fopen ( home3, "r" );
       for ( languagei35 = 0; languagei35 < 7; ++languagei35 )
       {
       if(fgets ( tray, sizeof tray, file45 )==NULL)
           cout << "FATAL: Error reading ~/.config/Wallch/Settings, which is essential for the functionality of the program!\n";
       }
       fclose ( file45 );

       if (!strcmp (tray,"Minimize to tray: True\n"))ui->checkBox_3->setCheckState(Qt::Checked);

    char sound[128];
    int sound2;
       FILE *file35 = fopen ( home3, "r" );
       for ( sound2 = 0; sound2 < 4; ++sound2 ) //getting the fourth line...
       {
       if(fgets ( sound, sizeof sound, file35 )==NULL)
             cout << "FATAL: Error reading ~/.config/Wallch/Settings, which is essential for the functionality of the program!\n";
       }
       fclose ( file35 );

   if(!strcmp (sound,"Play Sound: True\n")){ ui->playsoundcheckBox->setCheckState(Qt::Checked); ui->customsoundcheckBox->setEnabled(true); ui->browsesoundButton->setEnabled(false);}
   if(!strcmp (sound,"Play Sound: False\n")){ ui->playsoundcheckBox->setCheckState(Qt::Unchecked); ui->customsoundcheckBox->setEnabled(false); ui->browsesoundButton->setEnabled(false);}
   if(!strcmp (sound,"Play Sound: True + Custom Sound\n")){ ui->playsoundcheckBox->setCheckState(Qt::Checked); ui->customsoundcheckBox->setEnabled(true); ui->customsoundcheckBox->setCheckState(Qt::Checked); ui->browsesoundButton->setEnabled(true);}



    if(ui->playsoundcheckBox->isChecked() && !ui->customsoundcheckBox->isChecked()) ui->browsesoundButton->setEnabled(false);

    //checking to see if the earth wallpaper is activated
    if(system("pidof -x setupwallpaper > /home/$USER/.config/Wallch/Checks/earth; launched=`cat /home/$USER/.config/Wallch/Checks/earth`; if [ X\"$launched\" != X\"\" ]; then echo \"runs\" > /home/$USER/.config/Wallch/Checks/earth; else echo \"doesnt run\" > /home/$USER/.config/Wallch/Checks/earth; fi"))
        cout << "Error while trying to write to ~/.config/Wallch/Checks/earth. Please check file's permissions.\n";

    char home5[150]="/home/";
    strcat(home5,py);
    char other5[100]="/.config/Wallch/Checks/earth";
    strcat(home5,other5);
    char linepid2 [ 128 ];
    FILE *filepid2 = fopen ( home5, "r" );
       if(fgets ( linepid2, sizeof linepid2, filepid2 )==NULL)
           cout << "Error reading ~/.config/Wallch/Checks/earth\n";
    fclose ( filepid2 );
    if(!strcmp (linepid2,"runs\n")){
        ui->pushButton_6->setEnabled(false); ui->pushButton_7->setEnabled(true);
    } else {ui->pushButton_6->setEnabled(true); ui->pushButton_7->setEnabled(false);}
    if(system("rm -rf /home/$USER/.config/Wallch/Checks/earth"))
        cout << "Error while trying to delete to ~/.config/Wallch/Checks/earth. Please check file's permissions.\n";
    //checking to see if there is any option available for on pc-start wall
    int oneboot=0;
    int constantboot=0;
    int liveearthboot=0;

    /*once*/
    struct stat stFileInfoa;
    int intStata;
    char home6[150]="/home/";
    strcat(home6,py);
    char other6[70]="/.config/autostart/WallpaperChangerOnBoot.desktop";
    strcat(home6,other6);
    intStata = stat(home6,&stFileInfoa);
    if(intStata == 0) oneboot=1;
    if (!oneboot){ //if oneboot is one it is impossible to exist constant boot, so skip this check
        /*constant*/
        struct stat stFileInfob;
        int intStatb;
        char home7[150]="/home/";
        strcat(home7,py);
        char other7[70]="/.config/autostart/WallpaperChangerOnBootConstantApp.desktop";
        strcat(home7,other7);
        intStatb = stat(home7,&stFileInfob);
        if(intStatb == 0) constantboot=1;
    }
    if(!oneboot && !constantboot){ //finally, checking to see whether live earth wallpaper on boot is enabled...
        /*live earth*/
        struct stat stFileInfoc;
        int intStatc;
        char home_liveearth[150]="/home/";
        strcat(home_liveearth,py);
        char other_liveearth[70]="/.config/autostart/WallpaperChangerLiveEarthOnBoot.desktop";
        strcat(home_liveearth,other_liveearth);
        intStatc = stat(home_liveearth,&stFileInfoc);
        if(intStatc == 0) liveearthboot=1;
    }

    if(oneboot){ui->checkBox_5->setChecked(true); ui->radioButton->setEnabled(true); ui->radioButton->setChecked(true);}
    if(constantboot){ui->checkBox_5->setChecked(true); ui->radioButton_2->setEnabled(true); ui->radioButton_2->setChecked(true);}
    if(liveearthboot){ui->checkBox_5->setChecked(true); ui->radioButton_3->setEnabled(true); ui->radioButton_3->setChecked(true);}


       //checking to see if notification on image change is enabled, if it is, check the checkbox_4

        char home8[150]="/home/";
           strcat(home8,py);
           strcat(home8,other);
           char lini1[128];

           int languagei24;
              FILE *file5 = fopen ( home8, "r" );
              for ( languagei24 = 0; languagei24 < 2; ++languagei24 )

              {
              if(fgets ( lini1, sizeof lini1, file5 )==NULL)
                  cout << "FATAL: Error, possibly cannot read/write to ~/.config/Wallch/Settings which is essential for the program. Please check the file's permissions.\n";
              }
              fclose ( file5 );
              if(!strcmp (lini1,"Show notification: True\n")) ui->checkBox_4->setCheckState(Qt::Checked);

         //checking to see if and what history is selected
         char s3[128];
         int languagei25;
            FILE *file9 = fopen ( home8, "r" );
            for ( languagei25 = 0; languagei25 < 5; ++languagei25 )

            {
            if(fgets ( s3, sizeof s3, file9 )==NULL)
                cout << "FATAL: Error, possibly cannot read/write to ~/.config/Wallch/Settings which is essential for the program. Please check the file's permissions.\n";
            }
            fclose ( file9 );
               if (!strcmp(s3,"Keep history of: monthenday\n")) ui->checkmonthenday->setChecked(true);
               if (!strcmp(s3,"Keep history of: monthendayentime\n")) ui->checkmonthdaytime->setChecked(true);
               if (!strcmp(s3,"Keep history of: monthendayentimeensecs\n")) ui->checksecs->setChecked(true);
               if(ui->checkmonthenday->isChecked() || ui->checkmonthdaytime->isChecked() || ui->checksecs->isChecked()) ui->checkhistory->setEnabled(true); else
               {
                   ui->checknone->setChecked(true);ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false); ui->checkhistory->setEnabled(false);
               }
               if(ui->checkhistory->isChecked()) {ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);} else {ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false);}
               if(ui->checknone->isChecked()){ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false); ui->checkhistory->setEnabled(false);}
    if(ui->checkBox_3->isChecked()) minimize_already_checked=1; else minimize_already_checked=0;

       if (ui->checkBox_5->isChecked())
       {
           ui->radioButton->setEnabled(true);
           ui->radioButton_2->setEnabled(true);
           ui->radioButton_3->setEnabled(true);
       }
       else
       {
           ui->radioButton->setEnabled(false);
           ui->radioButton_2->setEnabled(false);
           ui->radioButton_3->setEnabled(false);
       }

          if(_language_==1) {
              if(ui->browsesoundButton->text() == "") ui->browsesoundButton->setText("(None)");
              ui->EnglishradioButton->setChecked(true);
          }
      else
          {
          ui->GreekradioButton->setChecked(true);
          translatetogreek();;

      }

      if(ui->checkmonthdaytime->isChecked() || ui->checkmonthdaytime->isChecked() || ui->checksecs->isChecked()){ui->checkhistory->setEnabled(true);}


      char home10[150]="/home/";
      strcat(home10,py);
      char other10[210]="/.config/Wallch/Checks/history_custom_path";
      strcat(home10,other10);
      char historiacline[128];
      int historyi4;
         FILE *file6 = fopen ( home10, "r" );
         for ( historyi4 = 0; historyi4 < 1; ++historyi4 ) //getting the first line...
         {
         if(fgets ( historiacline, sizeof historiacline, file6 )==NULL)
            cout << "Error reading ~/.config/Wallch/Checks/history_custom_path\n";
         }
         fclose ( file6 );

         if(!strcmp (historiacline,"\n"))ui->checkhistory->setCheckState(Qt::Unchecked); else ui->checkhistory->setCheckState(Qt::Checked);
         if(ui->checkhistory->isChecked()) {ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);}
         if(system("a=`gconftool-2 --get /desktop/gnome/background/picture_options`; sed -i '6 d' /home/$USER/.config/Wallch/Settings; sed -i \"6i\\Style: $a\n\" /home/$USER/.config/Wallch/Settings"))
             cout << "FATAL: Error, possibly cannot read/write to ~/.config/Wallch/Settings which is essential for the program. Please check the file's permissions.\n";
         char home11[150]="/home/";
         strcat(home11,py);
         strcat(home11,other);
         char historiaclini[128];
         int histori;
            FILE *fili = fopen ( home11, "r" );
            for ( histori = 0; histori < 6; ++histori ) //getting the 6th line...
            {
            if(fgets ( historiaclini, sizeof historiaclini, fili )==NULL)
                cout << "FATAL: Error, unable to read ~/.config/Wallch/Settings, which is essential for the functionality of the program!\n";
            }

            if(!strcmp (historiaclini,"Style: wallpaper\n") || !strcmp (historiaclini,"wallpaper\n")) ui->combostyle->setCurrentIndex(0);
            if(!strcmp (historiaclini,"Style: zoom\n")) ui->combostyle->setCurrentIndex(1);
            if(!strcmp (historiaclini,"Style: centered\n")) ui->combostyle->setCurrentIndex(2);
            if(!strcmp (historiaclini,"Style: scaled\n")) ui->combostyle->setCurrentIndex(3);
            if(!strcmp (historiaclini,"Style: stretched\n")) ui->combostyle->setCurrentIndex(4);
            if(!strcmp (historiaclini,"Style: spanned\n")) ui->combostyle->setCurrentIndex(5);
            fclose ( fili );

         if(!ui->radioButton->isChecked() && !ui->radioButton_2->isChecked() && ui->checkBox_3->isChecked() && ui->playsoundcheckBox->isChecked() && !ui->customsoundcheckBox->isChecked() && ui->checkBox_4->isChecked() && !ui->checkmonthenday->isChecked() && !ui->checkmonthdaytime->isChecked() && ui->checksecs->isChecked() && !ui->checkhistory->isChecked())


(void) new QShortcut(Qt::Key_Delete, this, SLOT(delete_image_P()));
         if (ui->checkBox_5->isChecked() && ui->radioButton_3->isChecked()) { ui->pushButton_8->setEnabled(false); ui->lineEdit->setEnabled(false);}
          if(!ui->checkBox_5->isChecked()){ui->pushButton_8->setEnabled(false); ui->lineEdit->setEnabled(false);}
}


void preferences::closeEvent( QCloseEvent * )
{
    //writing current values
        QSettings
           settings( "Wallch", "PreferencesWallpaperChanger" );
           settings.setValue( "browsesoundButton", ui->browsesoundButton->text() );
           settings.setValue( "linehistory", ui->linehistory->text() );
           settings.setValue( "lineEdit", ui->lineEdit->text() );
           settings.sync();
}

preferences::~preferences()
{
    delete ui;
}

void preferences::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void preferences::on_checkBox_5_clicked()
{
    if (ui->checkBox_5->isChecked())
    {
        if (ui->radioButton_3->isChecked()) { ui->pushButton_8->setEnabled(false); ui->lineEdit->setEnabled(false); }
        if (ui->radioButton->isChecked()) { ui->pushButton_8->setEnabled(true); ui->lineEdit->setEnabled(true);}
        if (ui->radioButton_2->isChecked()) { ui->pushButton_8->setEnabled(true); ui->lineEdit->setEnabled(true);}
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->radioButton_3->setEnabled(true);
    }
    else
    {
        ui->pushButton_8->setEnabled(false); ui->lineEdit->setEnabled(false);
        ui->radioButton->setEnabled(false);ui->radioButton_2->setEnabled(false);ui->radioButton_3->setEnabled(false);
    }
}

void preferences::on_radioButton_clicked()
{
    ui->pushButton_8->setEnabled(true);
    ui->lineEdit->setEnabled(true);
}

void preferences::on_radioButton_2_clicked()
{
    ui->pushButton_8->setEnabled(true);
    ui->lineEdit->setEnabled(true);
}

void preferences::on_radioButton_3_clicked()
{
    ui->pushButton_8->setEnabled(false);
    ui->lineEdit->setEnabled(false);
}


void preferences::on_closeButton_clicked()
{
    close();
}

void preferences::on_aplyButton_clicked()
{//checking if something is wrong
    if(ui->checkhistory->isEnabled() && ui->checkhistory->isChecked() && ui->linehistory->text()==""){ui->checkhistory->setChecked(false); ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false);}
    if(ui->checkBox_5->isChecked() && ui->radioButton->isChecked()){ if (ui->lineEdit->text()!="") {
            if(system("/usr/share/wallch/files/Scripts/enable_on_boot"))
                cout << "Error executing ~/.config/Wallch/Scripts/enable_on_boot. Please check file's permissions.\n";
            char home12[200]="/home/";
            strcat(home12,py);
            char other12[100]="/.config/Wallch/Checks/path_to_album_boot";
            strcat(home12,other12);
            QFile file(home12);
            if (!file.open(QIODevice::WriteOnly)) {
                cerr << "Cannot open file" << home12 << " for writing. The reason: "
                     << qPrintable(file.errorString()) << endl;
        }
            QTextStream out(&file);
            out << ui->lineEdit->text().toAscii();
            file.close();
        }
     else
        {

         if (!album)
         {QMessageBox msgBox;
        if(_language_==2){
            msgBox.setWindowTitle(QString::fromUtf8("Wallch | Αλλαγή Wallpaper στο start-up"));
            msgBox.setText(QString::fromUtf8("Παρακαλώ επιλέξτε ένα άλμπουμ για<br>την αλλαγή εικόνων στο start-up!"));
        }
        else{
        msgBox.setWindowTitle("Wallch | Wallpaper Change on Start-Up");
        msgBox.setText("Please choose an album for changing<br>wallpaper on PC start-up!");}
            msgBox.setIconPixmap(QIcon(":/icons/Pictures/folder_images.png").pixmap(QSize(80,80)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
            msgBox.exec(); return;}}

    }
    else {if(system("if [ -s /home/$USER/.config/autostart/WallpaperChangerOnBoot.desktop ]; then rm /home/$USER/.config/autostart/WallpaperChangerOnBoot.desktop; fi"))
        cout << "Error writing to ~/.config/autostart/WallpaperChangerOnBoot.desktop, please check path's permissions.\n";
    }
        if(ui->checkBox_5->isChecked() && ui->radioButton_2->isChecked()) { if (ui->lineEdit->text()!=""){
                if(system("/usr/share/wallch/files/Scripts/enable_constant_on_boot"))
                    cout << "Error executing ~/.config/Wallch/Scripts/enable_constant_on_boot. Please check file existnce and permissions.\n";
                char home13[200]="/home/";
                strcat(home13,py);
                char other13[100]="/.config/Wallch/Checks/path_to_album_boot";
                strcat(home13,other13);
                QFile file(home13);
                if (!file.open(QIODevice::WriteOnly)) {
                    cerr << "Cannot open file" << home13 << " for writing. The reason: "
                         << qPrintable(file.errorString()) << endl;
            }
                QTextStream out(&file);
                out << ui->lineEdit->text().toAscii();
                file.close();
            }else
            {
                QMessageBox msgBox;
                if(_language_==2){if (!album){
                          msgBox.setWindowTitle(QString::fromUtf8("Wallch | Αλλαγή Wallpaper στο start-up"));
                          msgBox.setText(QString::fromUtf8("Παρακαλώ επιλέξτε ένα άλμπουμ για<br>την αλλαγή εικόνων στο start-up!"));
                      }
                      else{
                      msgBox.setWindowTitle("Wallch | Wallpaper Changing on Start-Up");
                      msgBox.setText("Please choose an album for changing<br>wallpaper on PC start-up!");}
                          msgBox.setIconPixmap(QIcon(":/icons/Pictures/folder_images.png").pixmap(QSize(80,80)));
                          msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                          msgBox.exec(); return;}
            }
        }
        else {
            if(system("if [ -s /home/$USER/.config/autostart/WallpaperChangerOnBootConstantApp.desktop ]; then rm /home/$USER/.config/autostart/WallpaperChangerOnBootConstantApp.desktop; fi"))
                cout << "Error removing ~/.config/autostart/WallpaperChangerOnBootConstantApp.desktop. Check path's+file's permissions.\n";
        }

        if(ui->checkBox_5->isChecked() && ui->radioButton_3->isChecked()) {
                if(system("/usr/share/wallch/files/Scripts/enable_constant_on_boot live-earth"))
                    cout << "Error executing ~/.config/Wallch/Scripts/enable_constant_on_boot. Please check file existnce and permissions.\n";
        }
        else {
            if(system("if [ -s /home/$USER/.config/autostart/WallpaperChangerLiveEarthOnBoot.desktop ]; then rm /home/$USER/.config/autostart/WallpaperChangerLiveEarthOnBoot.desktop; fi"))
                cout << "Error removing ~/.config/autostart/WallpaperChangerLiveEarthOnBoot.desktop. Check path's+file's permissions.\n";
        }
        if(ui->checkBox_3->isChecked()){ if (!minimize_already_checked) {
                minimize_to_tray=1;
                if(system("sed -i '7 d' /home/$USER/.config/Wallch/Settings; sed -i \"7i\\Minimize to tray: True\n\" /home/$USER/.config/Wallch/Settings"))
                    cout << "FATAL: Error, could not write to ~/.config/Wallch/Settings, which is an essential file for the prorgam. Please check file's permissions.\n";
                }
        }
        else
        {
            minimize_to_tray=0;
            if(system("sed -i '7 d' /home/$USER/.config/Wallch/Settings; sed -i \"7i\\Minimize to tray: False\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
             }

        if(ui->checkBox_4->isChecked()){
            if(system("sed -i '2 d' /home/$USER/.config/Wallch/Settings; sed -i \"2i\\Show notification: True\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
        }
        else {
            if(system("sed -i '2 d' /home/$USER/.config/Wallch/Settings; sed -i \"2i\\Show notification: False\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
        }
        if(ui->checknone->isChecked()){
            if(system("sed -i '5 d' /home/$USER/.config/Wallch/Settings; sed -i \"5i\\Keep history of: none\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
            }
        if(ui->checkmonthenday->isChecked()){
            if(system("sed -i '5 d' /home/$USER/.config/Wallch/Settings; sed -i \"5i\\Keep history of: monthenday\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
            }
        if(ui->checkmonthdaytime->isChecked()){
            if(system("sed -i '5 d' /home/$USER/.config/Wallch/Settings; sed -i \"5i\\Keep history of: monthendayentime\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
            }
        if(ui->checksecs->isChecked()){
            if(system("sed -i '5 d' /home/$USER/.config/Wallch/Settings; sed -i \"5i\\Keep history of: monthendayentimeensecs\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
            }
if(ui->checkhistory->isChecked()){
    char home15[150]="/home/";
    strcat(home15,py);
    char other15[100]="/.config/Wallch/Checks/history_custom_path";
    strcat(home15,other15);
    QFile filw(home15);
    if (!filw.open(QIODevice::WriteOnly)) {
        cerr << "Cannot open file" << home15 << " for writing. The reason: "
             << qPrintable(filw.errorString()) << endl;
}
    QTextStream out(&filw);
    out << ui->linehistory->text();
    filw.close();
}
else {if(system("echo \"\" > /home/$USER/.config/Wallch/Checks/history_custom_path"))
    cout << "Error writing to ~/.config/Wallch/Checks/history_custom_path. Check file's permissions.\n";
    if(ui->GreekradioButton->isChecked() && ui->checkBox_3->isChecked()){
        emit traytogreek();
    }
    if(ui->EnglishradioButton->isChecked() && ui->checkBox_3->isChecked()){
        emit traytoenglish();
    }
}


if(ui->playsoundcheckBox->isChecked()){if(ui->customsoundcheckBox->isChecked() && ui->browsesoundButton->text() != "(None)" && ui->browsesoundButton->text() != QString::fromUtf8("(Κανένα)") ){
            if(system("sed -i '4 d' /home/$USER/.config/Wallch/Settings; sed -i \"4i\\Play Sound: True + Custom Sound\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
        }
    else
    {
        ui->customsoundcheckBox->setChecked(false);
        ui->browsesoundButton->setEnabled(false);
    }
        if(ui->browsesoundButton->text() == "(None)" || !ui->customsoundcheckBox->isChecked()) {
            if(system("sed -i '4 d' /home/$USER/.config/Wallch/Settings; sed -i \"4i\\Play Sound: True\n\" /home/$USER/.config/Wallch/Settings"))
                cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
        }
    }
    else {
        if(system("sed -i '4 d' /home/$USER/.config/Wallch/Settings; sed -i \"4i\\Play Sound: False\n\" /home/$USER/.config/Wallch/Settings"))
            cout << "FATAL: Error, could not read/write ~/.config/Wallch/Settings, which is essential for program's functionality, please check file's' permissions.\n";
    }
    //in any occassion, if it is not checked do what you have to do...

    if(ui->combostyle->currentIndex()==0){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options wallpaper&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}
    if(ui->combostyle->currentIndex()==1){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options zoom&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}
    if(ui->combostyle->currentIndex()==2){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options centered&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}
    if(ui->combostyle->currentIndex()==3){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options scaled&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}
    if(ui->combostyle->currentIndex()==4){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options stretched&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}
    if(ui->combostyle->currentIndex()==5){if(system("gconftool-2 --type string --set /desktop/gnome/background/picture_options spanned&")) cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";}

    close();
}

void preferences::on_checkBox_3_clicked()
{


}

void preferences::on_playsoundcheckBox_clicked()
{


    if (ui->playsoundcheckBox->isChecked()){ui->customsoundcheckBox->setEnabled(true);}
    if (!ui->playsoundcheckBox->isChecked()){ui->customsoundcheckBox->setEnabled(false); ui->browsesoundButton->setEnabled(false); }
    if (ui->customsoundcheckBox->isChecked() && ui->playsoundcheckBox->isChecked()){ui->browsesoundButton->setEnabled(true);}
}


void preferences::on_customsoundcheckBox_clicked()
{


        if (ui->customsoundcheckBox->isChecked()){ui->browsesoundButton->setEnabled(true);}
        if (!ui->customsoundcheckBox->isChecked()){ui->browsesoundButton->setEnabled(false);}
}



void preferences::on_browsesoundButton_clicked()
{
    QString path;
    if(_language_==2)
        path = QFileDialog::getOpenFileName(this, QString::fromUtf8("Wallch | Διάλεξε ήχο"), QDir::homePath(), tr("*.mp3 *.wav *.ogg"));
    else
        path = QFileDialog::getOpenFileName(this, tr("Wallch | Choose Custom Sound"), QDir::homePath(), tr("*.mp3 *.wav *.ogg"));
           if ( path != ""){


               if(system("echo "" > /home/$USER/.config/Wallch/Checks/text_to_button"))
                   cout << "Error writing to ~/.config/Wallch/Checks/text_to_button. Check file's permissions.\n";
               //QString realpath = path.toUtf8();
               char *realpath = path.toUtf8().data();
               char home16[150]="/home/";
               strcat(home16,py);
               char other16[100]="/.config/Wallch/Checks/text_to_button";
               strcat(home16,other16);
               QFile file(home16);
               if (!file.open(QIODevice::WriteOnly)) {
                   cerr << "Cannot open file" << home16 << " for writing. The reason: "
                        << qPrintable(file.errorString()) << endl;
                   cout << "Try again.";
                   if(system("echo "" > /home/$USER/.config/Wallch/Checks/text_to_buton"))
                       cout << "Error, could not write to ~/.config/Wallch/Checks/text_to_buton. Check file permissions.\n";
          }
               QTextStream out(&file);
               out << QString::fromUtf8(realpath);
               file.close();
               if(system("cat /home/$USER/.config/Wallch/Checks/text_to_button; a=`cat /home/$USER/.config/Wallch/Checks/text_to_button`; echo ${a##*/} > /home/$USER/.config/Wallch/Checks/text_to_button_name_only;"))
                   cout << "Error reading/writing ~/.config/Wallch/Checks/text_to_button*. Please check files' permissions.\n";
               char morename[20]="_name_only";
               strcat(home16,morename);
                  fstream infile;
                  infile.open(home16); // open file
                  string s="";
                  if(infile) {
                  while(getline(infile, s)) {
                  QString qw = QString::fromUtf8(s.c_str());
                  ui->browsesoundButton->setText(qw);
                 }
               }
       }
           else
               ui->browsesoundButton->setText(ui->browsesoundButton->text().toAscii());

}

void preferences::on_pushButton_clicked()
{
    if(system("killall -v mpg321 mpg123 canberra-gtk-play 2> /dev/null")!=256){ //stopping all the sounds
        cout << "Error while trying to kill mpg321 mpg123 and canberra-gtk-play\n";
    }
}

void preferences::on_checkBox_4_clicked()
{


}

void preferences::on_checknone_clicked()
{


    ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false); ui->checkhistory->setEnabled(false);
}

void preferences::on_checkmonthenday_clicked()
{
    ui->checkhistory->setEnabled(true);
    if(ui->checkhistory->isChecked()){
    ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);}


}

void preferences::on_checkmonthdaytime_clicked()
{
    ui->checkhistory->setEnabled(true);
    if(ui->checkhistory->isChecked()){
    ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);}


}

void preferences::on_checksecs_clicked()
{
    ui->checkhistory->setEnabled(true);
    if(ui->checkhistory->isChecked()){
    ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);}


}

void preferences::on_checkhistory_clicked()
{
    if(ui->checkhistory->isChecked()){ui->linehistory->setEnabled(true); ui->buttonhistory->setEnabled(true);}
    else {ui->linehistory->setEnabled(false); ui->buttonhistory->setEnabled(false);}



}



void preferences::on_EnglishradioButton_clicked()
{
    if(system("sed -i '1 d' /home/$USER/.config/Wallch/Settings; sed -i \"1i\\Language: English\n\" /home/$USER/.config/Wallch/Settings"))
        cout << "FATAL: Error, unable to read/write to ~/.config/Wallch/Settings, which is an essential file for the functionality of the program. Please check the file's permissions.\n";
    translatetoenglish();
    _language_=1;
    emit toenglish();
    if(minimize_to_tray)
        emit traytoenglish();
}

void preferences::on_GreekradioButton_clicked()
{
    if(system("sed -i '1 d' /home/$USER/.config/Wallch/Settings; sed -i \"1i\\Language: Greek\n\" /home/$USER/.config/Wallch/Settings"))
        cout << "Error passing value to ~/.gconf/desktop/gnome/background/picture_options. Please check path's' and file's permissions.\n";
    translatetogreek();
    _language_=2;
    emit togreek();
    if(minimize_to_tray)
        emit traytogreek();
}

void preferences::on_buttonhistory_clicked()
{
    QString path;
    if(_language_==2)
        path = QFileDialog::getExistingDirectory( this, QString::fromUtf8("Wallch | Διάλεξε φάκελο για αποθήκευση ιστορικού"), QString::null);
    else
        path = QFileDialog::getExistingDirectory( this, "Wallch | Choose Folder For Keeping History", QString::null);
    if(path!=""){
    ui->linehistory->setText(path);
    }
}

void preferences::on_combostyle_activated(QString )
{


}

void preferences::on_pushButton_2_clicked()
{
                QMessageBox msgBox2;
                QPushButton *cancel;
                QPushButton *ok;
                if(_language_==2){
                    msgBox2.setWindowTitle(QString::fromUtf8("Wallch | Επαναφορά ρυθμίσεων"));
                    msgBox2.setInformativeText(QString::fromUtf8("Σίγουρα θέλετε να επαναφέρετε τις ρυθμίσεις στην αρχική τους κατάσταση;"));
                    msgBox2.setText(QString::fromUtf8("Ζητήθηκε επαναφορά ρυθμίσεων"));
                    ok = msgBox2.addButton(QString::fromUtf8("Εντάξει"), QMessageBox::ActionRole);
                    cancel = msgBox2.addButton(QString::fromUtf8("Άκυρο"), QMessageBox::ActionRole);}else{
                    msgBox2.setWindowTitle("Wallch | Reset Preferences");
                    msgBox2.setInformativeText("Are you sure you want to reset your preferences?");
                    msgBox2.setText("You requested preferences reset.");
                    ok = msgBox2.addButton("Ok", QMessageBox::ActionRole);
                    cancel = msgBox2.addButton("Cancel", QMessageBox::ActionRole);}
                msgBox2.setIconPixmap(QIcon(":/icons/Pictures/settings.png").pixmap(QSize(100,100)));
                msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                    msgBox2.exec();
                    if (msgBox2.clickedButton() == ok) {

                        ui->radioButton->setChecked(false);
                        ui->radioButton_2->setChecked(false);
                        ui->checkBox_3->setChecked(true);
                        ui->playsoundcheckBox->setChecked(true);
                        ui->customsoundcheckBox->setChecked(false);
                        ui->checkBox_4->setChecked(true);
                        ui->checkmonthenday->setChecked(false);
                        ui->checkmonthdaytime->setChecked(false);
                        ui->checknone->setChecked(false);
                        ui->checkhistory->setChecked(false);
                        ui->checkBox_5->setChecked(false);
                        ui->radioButton->setChecked(true);
                        ui->radioButton->setEnabled(false);
                        ui->radioButton_2->setEnabled(false);
                        ui->radioButton_3->setChecked(false);
                        ui->radioButton_3->setEnabled(false);
                        on_aplyButton_clicked();
                        QMessageBox msgBox;
                        if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Οι ρυθμίσεις επανήλθαν"));
                            msgBox.setInformativeText(QString::fromUtf8("Οι ρυθμίσεις επανήλθαν στην αρχική τους κατάσταση."));
                            msgBox.setText(QString::fromUtf8("<b>Οι ρυθμίσεις επανήλθαν.</b>"));
                        }
                        else{
                        msgBox.setWindowTitle("Preferences Reseted");
                        msgBox.setInformativeText("Preferences had been reseted to the default ones.");
                        msgBox.setText("<b>Preferences Reseted.</b>");}
                            //set the icon of the message box to a custom pixmap of size 64x64
                            msgBox.setIconPixmap(QIcon(":/icons/Pictures/settings.png").pixmap(QSize(100,100)));
                            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                            msgBox.exec();




                    } else return;
}

void preferences::on_pushButton_4_clicked()
{
    ui->listWidget->clear();
}

void preferences::on_pushButton_3_clicked()
{
    QStringList path;
    if(_language_==2)
        path = QFileDialog::getOpenFileNames(this, QString::fromUtf8("Wallch | Διάλεξε εικόνες"), QDir::homePath(), tr("*png *gif *bmp *jpg *jpeg"));
        else
        path = QFileDialog::getOpenFileNames(this, tr("Wallch | Choose Pics"), QDir::homePath(), tr("*png *gif *bmp *jpg *jpeg"));
    if(path.count() == 0) return;
    int count=0;
    while (count < path.count()){
        QString qstr = path[count];
                   QListWidgetItem *item = new QListWidgetItem;
                   item->setText(qstr);
                   if(_language_==2)
                      item->setToolTip(QString::fromUtf8("Αν αποθηκεύσεις αυτό το άλμπουμ μπορείς να το φορτώσεις αργότερα από το μενού \"Αρχείο\""));
                   else
                      item->setToolTip("If you save this album you can later load it via the File menu");
                   ui->listWidget->addItem(item);
                   count++;
                            }
}

void preferences::on_pushButton_5_clicked()
{
    if(ui->listWidget->count() <= 1) {
        QMessageBox msgBox;
        if(_language_==2){msgBox.setWindowTitle(QString::fromUtf8("Οι εικόνες δεν είναι αρκετές"));
            msgBox.setInformativeText(QString::fromUtf8("Ο αριθμός των εικόνων δεν είναι αρκετός. Πρέπει να υπάρχουν τουλάχιστον 2 εικόνες"));
            msgBox.setText(QString::fromUtf8("<b>Οι εικόνες δεν είναι αρκετές!</b>"));
        }
        else{
        msgBox.setWindowTitle("Not enough images!");
        msgBox.setInformativeText("You must select up to 2 pictures in order to save an album!");
        msgBox.setText("<b>The images are not enough.</b>");
        }
            //set the icon of the message box to a custom pixmap of size 64x64
            msgBox.setIconPixmap(QIcon(":/icons/Pictures/folder_images.png").pixmap(QSize(100,100)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
            msgBox.exec();
            return;
    }
    QString format = "wallch";
    QString initialPath = QDir::currentPath() + tr("/album.") + format;
    QString fileName;
    if(_language_==2)
        fileName = QFileDialog::getSaveFileName(this, QString::fromUtf8("Αποθήκευση ως"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(format.toUpper())
                               .arg(format));
    else
        fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(format.toUpper())
                               .arg(format));
    if(!fileName.isEmpty()){
        QFile file8( fileName ); // #include <QFile>

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

void preferences::on_pushButton_6_clicked()
{
    if(_start_running_){
    //Start is running, in order to avoid conflicts display an error message!
        QMessageBox msgBox;
        if(_language_==2){
            msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό  Wallpaper της Γης"));
            msgBox.setText(QString::fromUtf8("Παρακαλώ σταματήστε την τρέχουσα διεργασία και προσπαθήστε ξανά."));
        }
        else{
        msgBox.setWindowTitle("Wallch | Live Earth Wallpaper");
        msgBox.setText("Please stop the current process and try again.");}
            msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
            msgBox.exec();
            return;
    }
    //try to ping google
    int online = system("ping -c 1 google.com > /dev/null");
        if ( online != 0 ){ //Unable to reach google
            QMessageBox msgBox;
            if(_language_==2){
                msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό  Wallpaper της Γης"));
                msgBox.setText(QString::fromUtf8("Η σύνδεση με τον διακομιστή ήταν αδύνατη!<br>Ελέγξτε την σύνδεσή σας στο διαδύκτιο!"));
            }
            else{
            msgBox.setWindowTitle("Wallch | Live Earth Wallpaper");
            msgBox.setText("<b>Unable to connect.</b><br>Check your internet connection!");}
                msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
                msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
                msgBox.exec();
                return;
        }
        if(system("/usr/share/wallch/files/Scripts/setupwallpaper&"))
            cout << "Error while trying to execute ~/.config/Wallch/Scripts/setupwallpaper. Check for file existence and permissions.\n";
        _live_earth_running_=1;
        ui->pushButton_6->setEnabled(false); ui->pushButton_7->setEnabled(true);
        QMessageBox msgBox;
        if(_language_==2){
            msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό  Wallpaper της Γης"));
            msgBox.setText(QString::fromUtf8("Το ζωντανό Wallpaper της Γης ενεργοποιήθηκε<br>και θα αλλάζει κάθε ½ ώρα!"));
        }
        else{
        msgBox.setWindowTitle("Wallch | Live Earth Wallpaper");
        msgBox.setText(QString::fromUtf8("Live Earth Wallpaper is active<br>and it will change every ½ hour!"));}
            msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
            msgBox.exec();
}

void preferences::on_pushButton_7_clicked()
{
    _live_earth_running_=0;
    QMessageBox msgBox;
    if(_language_==2){
        msgBox.setWindowTitle(QString::fromUtf8("Wallch | Ζωντανό  Wallpaper της Γης"));
        msgBox.setText(QString::fromUtf8("Το ζωντανό Wallpaper της Γης απενεργοποιήθηκε!"));
    }
    else{
    msgBox.setWindowTitle("Wallch | Live Earth Wallpaper");
    msgBox.setText(QString::fromUtf8("Live Earth Wallpaper deactivated!"));}
        msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
        msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallpaper.png"));
        msgBox.exec();
    if(system("killall -v setupwallpaper 2> /dev/null; if [ -s /home/$USER/.config/Wallch/world_sunlight_Wallpaper.jpg ]; then rm -rf /home/$USER/.config/Wallch/world_sunlight_Wallpaper.jpg; fi"))
        cout << "Error killing some processes or removing ~/.config/Wallch/world_sunlight_Wallpaper.jpg. Please check file's permissions.\n";
    ui->pushButton_7->setEnabled(false); ui->pushButton_6->setEnabled(true);
}

void preferences::on_pushButton_8_clicked()
{
    QString Qpath;
    if(_language_==2)
        Qpath = QFileDialog::getOpenFileName(this, QString::fromUtf8("Wallch | Διάλεξε άλμπουμ για αλλαγή στο PC-start"), QDir::homePath(), tr("*.wallch"));
    else
        Qpath = QFileDialog::getOpenFileName(this, tr("Wallch | Choose Album for Start-Up Changing"), QDir::homePath(), tr("*.wallch"));
    if(Qpath.count() == 0) return; else ui->listWidget->clear();
    ui->lineEdit->setText(Qpath);

}

void preferences::delete_image_P()
{
    if(!ui->listWidget->count()) return;
    if(ui->listWidget->count()==1) {ui->listWidget->clear(); return;}
    if(ui->listWidget->currentItem()->isSelected()) delete ui->listWidget->currentItem();
}

void preferences::translatetogreek()
{preferences::setWindowTitle(QString::fromUtf8("Ρυθμίσεις"));
    if(ui->browsesoundButton->text() == "") ui->browsesoundButton->setText(QString::fromUtf8("(Κανένα)"));
    ui->toolBox->setItemText(0, QString::fromUtf8("Γενικά"));
    ui->toolBox->setItemText(1, QString::fromUtf8("Ειδοποιήσεις"));
    ui->toolBox->setItemText(2, QString::fromUtf8("Ιστορία"));
    ui->toolBox->setItemText(3, QString::fromUtf8("Γλώσσες"));
    ui->toolBox->setItemText(4, QString::fromUtf8("Ζωντανό Background Της Γης"));
    ui->closeButton->setText(QString::fromUtf8("Κλείσιμο"));
    ui->pushButton_2->setText(QString::fromUtf8("Επαναφορά ρυθμίσεων"));
    ui->pushButton_6->setText(QString::fromUtf8("Ενεργοποίησε το Live Earth Wallpaper"));
    ui->pushButton_7->setText(QString::fromUtf8("Απενεργοποίησε το Live Earth Wallpaper"));
    ui->aplyButton->setText(QString::fromUtf8("Εφαρμογή"));
 ui->radioButton->setText(QString::fromUtf8("Άλλαξε Wallpaper (Μια φορά)"));
 ui->radioButton_2->setText(QString::fromUtf8("Άλλαζε συνεχόμενα Wallpapers"));
 ui->label_14->setText(QString::fromUtf8("Χρησιμοποιώντας:"));
 ui->pushButton_8->setText(QString::fromUtf8("Διάλεξε άλμπουμ"));
 ui->checkBox_5->setText(QString::fromUtf8("Άλλαξε Wallpapers στο start-up του PC"));
 ui->checkBox_3->setText(QString::fromUtf8("Ελαχιστοποίηση στο tray"));
 ui->label_19->setText(QString::fromUtf8("         Στυλ των εικόνων"));
 ui->label_12->setText(QString::fromUtf8("Το Live earth wallpaper αυτόματα κάνει το desktop wallpaper σας\nνα είναι ένα ζωντανό background της Γης που αλλάζει κάθε ½ ώρα!\nΗ σύνδεση στο Διαδίκτυο είναι απαραίτητη."));
 ui->playsoundcheckBox->setText(QString::fromUtf8("Παίξε ήχο κάθε φορά που αλλάζει εικόνα"));
 ui->customsoundcheckBox->setText(QString::fromUtf8("Χρησιμοποίησε δικό σου ήχο"));
 ui->pushButton->setText(QString::fromUtf8("Σταμάτα όλους τους ήχους"));
 ui->checkBox_4->setText(QString::fromUtf8("Ειδοποίηση στην αλλαγή εικόνας"));
 ui->label_5->setText(QString::fromUtf8("Η αποθήκευση της history γίνεται στο ~/.config/Wallch/History"));
 ui->checknone->setText(QString::fromUtf8("Να μη κρατάει ιστορικό"));
 ui->checkmonthenday->setText(QString::fromUtf8("Εξειδικευμένο ιστορικό (μήνας και ημέρα του μήνα)"));
 ui->checkmonthdaytime->setText(QString::fromUtf8("Πιο εξειδικευμένο ιστορικό (μήνας, ημέρα του μήνα και ώρα)"));
 ui->checksecs->setText(QString::fromUtf8("Πολύ αναλυτικό ιστορικό (μήνας, ημέρα του μήνα, ώρα και δεύτερα)"));
 ui->checkhistory->setText(QString::fromUtf8("Αποθήκευσε το ιστορικό σε έναν φάκελο της προτίμησής σου:"));
 ui->label_18->setText(QString::fromUtf8("Ελληνικά"));
 ui->label_8->setText(QString::fromUtf8("Αγγλικά"));
 ui->label_9->setText(QString::fromUtf8("Φτιάξε 1 Wallch άλμπουμ:"));
 ui->pushButton_3->setText(QString::fromUtf8("Προσθήκη εικόνων"));
 ui->p_addfolder->setText(QString::fromUtf8("Φάκελος"));
 ui->pushButton_4->setText(QString::fromUtf8("Καθαρισμός"));
 ui->pushButton_5->setText(QString::fromUtf8("Αποθήκευσε το Wallch άλμπουμ ώς"));
 ui->label_4->setPixmap(QPixmap(":/icons/Pictures/notification-greek.png"));
 ui->pushButton_8->setToolTip(QString::fromUtf8("Διάλεξε άλμπουμ για να χρησιμοποιηθεί με τις Pc-start up επιλογές"));
 ui->pushButton_3->setToolTip(QString::fromUtf8("Πρόσθεσε εικόνες στην λίστα"));
 ui->pushButton_4->setToolTip(QString::fromUtf8("Καθάρισε την λίστα"));
 ui->pushButton_5->setToolTip(QString::fromUtf8("Αποθήκευσε το άλμπουμ σου"));
 ui->browsesoundButton->setToolTip(QString::fromUtf8("Κλικ για επιλογή δικού σου ήχου"));
 ui->pushButton->setToolTip(QString::fromUtf8("Σταμάτημα όλων των πιθανών ήχων"));
 ui->aplyButton->setToolTip(QString::fromUtf8("Εφαρμογή των αλλαγών και κλείσιμο παραθύρου"));
 ui->pushButton_2->setToolTip(QString::fromUtf8("Επαναφορά ρυθμίσεων στις αρχικές"));
 ui->label_18->setText(QString::fromUtf8("Ελληνικά"));
 ui->label_8->setText(QString::fromUtf8("Αγγλικά"));
 ui->p_addfolder->setToolTip(QString::fromUtf8("Προσθήκη όλων των εικόνων ενός φακέλου και των υποφακέλων του"));
 if(ui->browsesoundButton->text()=="(None)")
     ui->browsesoundButton->setText(QString::fromUtf8("(Κανένα)"));
 ui->radioButton_3->setText(QString::fromUtf8("Eνεργοποίηση Ζωντανού Wallpaper της Γης"));
}

void preferences::translatetoenglish()
{preferences::setWindowTitle(QString::fromUtf8("Preferences"));
    ui->toolBox->setItemText(0, "General");
    ui->toolBox->setItemText(1, "Notifications");
    ui->toolBox->setItemText(2, "History");
    ui->toolBox->setItemText(3, "Languages");
    ui->toolBox->setItemText(4, "Live Earth Wallpaper");
    ui->checkBox_5->setText("Change Wallpapers on Pc-Start up");
    ui->radioButton->setText("Change Desktop Wallpaper ( Once )");
    ui->radioButton_2->setText("Start Changing Desktop Wallpapers");
    ui->checkBox_3->setText("Minimize to tray");
    ui->label_19->setText("Default Style of wallpaper images:");
    ui->label_9->setText("Create a Wallch album:");
    ui->pushButton_3->setText("Add images");
    ui->pushButton_3->setToolTip("Add images to the list");
    ui->p_addfolder->setText("Add Folder");
    ui->pushButton_4->setText("Clear");
    ui->pushButton_4->setToolTip("Clear the list");
    ui->pushButton_5->setText("Save your Wallch Album as");
    ui->pushButton_5->setToolTip("Save your album");
    ui->label_14->setText("Using:");
    ui->pushButton_8->setText("Select Album");
    ui->pushButton_8->setToolTip("Select album to be used with the Pc-start up options");
    ui->playsoundcheckBox->setText("Play sound everytime a wallpaper changes");
    ui->customsoundcheckBox->setText("Use custom sound");
    if(!ui->customsoundcheckBox->isChecked())
        ui->browsesoundButton->setText("(None)");
    ui->browsesoundButton->setToolTip("Click to select your own sound");
    ui->pushButton->setText("Stop All Sounds");
    ui->pushButton->setToolTip("Stop all possible sounds");
    ui->checkBox_4->setText("Notification on Wallpaper change");
    ui->label_5->setText("Default directory for keeping history is ~/.config/Wallch/History");
    ui->checknone->setText("Do not keep history");
    ui->checkmonthenday->setText("Simple history (month and day of month)");
    ui->checkmonthdaytime->setText("Advanced history (month, day of month and time)");
    ui->checksecs->setText("Very analytical history (month, day of month, time and secs)");
    ui->checkhistory->setText("Keep history on a directory of your preference:");
    ui->buttonhistory->setText("Browse");
    ui->pushButton_6->setText("Activate Live Earth Wallpaper");
    ui->pushButton_7->setText("Deactivate Live Earth Wallpaper");
    ui->pushButton_2->setText("Reset Preferences");
    ui->pushButton_2->setToolTip("Reset preferences to the default onces");
    ui->closeButton->setText("&Close");
    ui->aplyButton->setText("&Save");
    ui->aplyButton->setToolTip("Save and close the dialog");
    ui->label_18->setText("Greek");
    ui->label_8->setText("English");
    ui->p_addfolder->setToolTip("Add all images from a folder and its subfolders");
    if(ui->browsesoundButton->text()=="(Κανένα)")
        ui->browsesoundButton->setText("(None)");
    ui->radioButton_3->setText("Auto-enable Live Earth Wallpaper");
    ui->label_4->setPixmap(QPixmap(":/icons/Pictures/notification.png"));
}

void preferences::on_p_addfolder_clicked()
{
    QString qpath;
    if(_language_==2)
        qpath = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Wallch | Διάλεξε Φάκελο"), QDir::homePath());
    else
        qpath = QFileDialog::getExistingDirectory(this, tr("Wallch | Choose Folder"), QDir::homePath());
    if(qpath.count() == 0) return;
    char home17[100]="/home/";
    strcat(home17,py);
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
               //remove temp files used...
               if(system("rm -rf ~/.config/Wallch/Checks/add_fodler*"))
                   cout << "Error while trying to remove ~/.config/Wallch/Checks/add_fodler* .Please check file(s) permissions!";
}
