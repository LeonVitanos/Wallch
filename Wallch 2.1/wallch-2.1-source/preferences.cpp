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

preferences::preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preferences)
{
    ui->setupUi(this);
    QSettings settings( "Wallch", "Preferences" );
                    ui->browsesoundButton->setText( settings.value( "browsesoundButton" ).toString() );
                    ui->checkBox_3->setChecked(settings.value("tray", false).toBool());
                    ui->playsoundcheckBox->setChecked(settings.value("sound", false).toBool());
                    ui->customsoundcheckBox->setChecked(settings.value("custom", false).toBool());
                    ui->combostyle_3->setCurrentIndex(settings.value( "history" ).toInt());
                    ui->checkBox_4->setChecked(settings.value("notification", true).toBool());
                    ui->combostyle_2->setCurrentIndex(settings.value( "language" ).toInt());


                    if(!ui->playsoundcheckBox->isChecked()){
                        ui->customsoundcheckBox->setEnabled(false);
                        ui->customsoundcheckBox->setChecked(false);
                        ui->browsesoundButton->setEnabled(false);
                    }
                    if(!ui->customsoundcheckBox->isChecked())
                        ui->browsesoundButton->setEnabled(false);

    //checking to see if there is any option available for on pc-start wallpaper change
    if(QFile(QDir::homePath()+"/.config/autostart/WallchLiveEarthOnBoot.desktop").exists())
    {
        ui->checkBox_5->setChecked(true); ui->radioButton_3->setEnabled(true); ui->radioButton_3->setChecked(true);
    }
    else if(QFile(QDir::homePath()+"/.config/autostart/WallchOnBoot.desktop").exists())
    {
        ui->checkBox_5->setChecked(true); ui->radioButton->setEnabled(true); ui->radioButton->setChecked(true);
    }
    else if(QFile(QDir::homePath()+"/.config/autostart/WallchOnBootConstantApp.desktop").exists())
    {
        ui->checkBox_5->setChecked(true); ui->radioButton_2->setEnabled(true); ui->radioButton_2->setChecked(true);
    }
    else
    {
        ui->checkBox_5->setChecked(false); ui->radioButton->setEnabled(false); ui->radioButton_2->setEnabled(false); ui->radioButton_3->setEnabled(false);
    }

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
       if(ui->browsesoundButton->text().isEmpty())
           ui->browsesoundButton->setText(tr("(None)"));

         FILE* pipe;
         pipe= popen("gconftool-2 --get /desktop/gnome/background/picture_options;", "r");

         char buffer[128];
         string result = "";

         while(!feof(pipe)) {
             if(fgets(buffer, 128, pipe) != NULL)
                 result += buffer;
             }
         pclose(pipe);
         QString res = QString::fromStdString(result);
         res.replace(QString("\n"),QString(""));
         //res.replace(QString("'"),QString(""));

         if (res=="wallpaper") ui->combostyle->setCurrentIndex(0);
         if (res=="zoom") ui->combostyle->setCurrentIndex(1);
         if (res=="centered") ui->combostyle->setCurrentIndex(2);
         if (res=="scaled") ui->combostyle->setCurrentIndex(3);
         if (res=="stretched") ui->combostyle->setCurrentIndex(4);
         if (res=="spanned") ui->combostyle->setCurrentIndex(5);

            ui->pushButton->setIcon(QIcon::fromTheme("audio-volume-muted"));
            ui->browsesoundButton->setIcon(QIcon::fromTheme("audio-x-generic"));
}


void preferences::closeEvent( QCloseEvent * )
{
    //writing current values
        QSettings
           settings( "Wallch", "Preferences" );
           settings.setValue( "browsesoundButton", ui->browsesoundButton->text() );
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
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->radioButton_3->setEnabled(true);
    }
    else
    {
        ui->radioButton->setEnabled(false);ui->radioButton_2->setEnabled(false);ui->radioButton_3->setEnabled(false);
    }
}

void preferences::on_closeButton_clicked()
{
    close();
}

void preferences::on_aplyButton_clicked()
{//checking if something is wrong
    if(ui->checkBox_5->isChecked() && (ui->radioButton->isChecked() || ui->radioButton_2->isChecked())){
            if (ui->radioButton->isChecked()){
                if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --once\nTerminal=false\nIcon=wallch\nComment=Change desktop background once\nCategories=Utility;Application;\" > ~/.config/autostart/WallchOnBoot.desktop"))
                    cerr << "Error creating ~/.config/autostart/WallchOnBoot.desktop. Please check file existnce and permissions.\n";
                //removing other .desktop files
                if(system("if [ -s ~/.config/autostart/WallchLiveEarthOnBoot.desktop ]; then rm ~/.config/autostart/WallchLiveEarthOnBoot.desktop; fi"))
                    cerr << "Error removing ~/.config/autostart/WallchLiveEarthOnBoot.desktop. Check path's+file's permissions.\n";
                if(system("if [ -s ~/.config/autostart/WallchOnBootConstantApp.desktop ]; then rm ~/.config/autostart/WallchOnBootConstantApp.desktop; fi"))
                        cerr << "Error writing to ~/.config/autostart/WallchOnBootConstantApp.desktop, please check path's permissions.\n";
            }
            else if(ui->radioButton_2->isChecked()){
                if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --constant\nTerminal=false\nIcon=wallch\nComment=Start changing desktop background after a certain time\nCategories=Utility;Application;\" > ~/.config/autostart/WallchOnBootConstantApp.desktop"))
                    cerr << "Error creating ~/.config/autostart/WallchOnBootConstantApp.desktop. Please check file existnce and permissions.\n";
                //removing other .desktop files
                if(system("if [ -s ~/.config/autostart/WallchLiveEarthOnBoot.desktop ]; then rm ~/.config/autostart/WallchLiveEarthOnBoot.desktop; fi"))
                    cerr << "Error removing ~/.config/autostart/WallchLiveEarthOnBoot.desktop. Check path's+file's permissions.\n";
                if(system("if [ -s ~/.config/autostart/WallchOnBoot.desktop ]; then rm ~/.config/autostart/WallchOnBoot.desktop; fi"))
                        cerr << "Error writing to ~/.config/autostart/WallchOnBoot.desktop, please check path's permissions.\n";
            }
        else
        {
            if (!album){
                QMessageBox msgBox;
                    msgBox.setWindowTitle(tr("Wallpaper Change on Start-Up"));
                    msgBox.setText(tr("Please choose an album for changing<br>wallpaper on PC start-up!"));
                    msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(80,80)));
                    msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
                    msgBox.exec();
                    return;
           }
        }
}

        if(ui->checkBox_5->isChecked() && ui->radioButton_3->isChecked()) {
                if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --earth\nTerminal=false\nIcon=wallch\nComment=Enable live earth wallpaper\nCategories=Utility;Application;\" > ~/.config/autostart/WallchLiveEarthOnBoot.desktop"))
                    cerr << "Error creating ~/.config/autostart/WallchLiveEarthOnBoot.desktop. Please check file existnce and permissions.\n";
                //removing other .desktop files
                if(system("if [ -s ~/.config/autostart/WallchOnBootConstantApp.desktop ]; then rm ~/.config/autostart/WallchOnBootConstantApp.desktop; fi"))
                    cerr << "Error removing ~/.config/autostart/WallchOnBootConstantApp.desktop. Check path's+file's permissions.\n";
                if(system("if [ -s ~/.config/autostart/WallchOnBoot.desktop ]; then rm ~/.config/autostart/WallchOnBoot.desktop; fi"))
                        cerr << "Error writing to ~/.config/autostart/WallchOnBoot.desktop, please check path's permissions.\n";
        }

        //removing .desktop files because Change Wallpapers on Pc-Start up checkbox is not checked
        if(!ui->checkBox_5->isChecked()){
        if(system("if [ -s ~/.config/autostart/WallchOnBootConstantApp.desktop ]; then rm ~/.config/autostart/WallchOnBootConstantApp.desktop; fi"))
            cerr << "Error removing ~/.config/autostart/WallchOnBootConstantApp.desktop. Check path's+file's permissions.\n";
        if(system("if [ -s ~/.config/autostart/WallchLiveEarthOnBoot.desktop ]; then rm ~/.config/autostart/WallchLiveEarthOnBoot.desktop; fi"))
            cerr << "Error removing ~/.config/autostart/WallchLiveEarthOnBoot.desktop. Check path's+file's permissions.\n";
        if(system("if [ -s ~/.config/autostart/WallchOnBoot.desktop ]; then rm ~/.config/autostart/WallchOnBoot.desktop; fi"))
                cerr << "Error writing to ~/.config/autostart/WallchOnBoot.desktop, please check path's permissions.\n";}


        QSettings
           settings( "Wallch", "Preferences" );
           settings.setValue("tray", ui->checkBox_3->isChecked());
           settings.setValue("notification", ui->checkBox_4->isChecked());
           settings.setValue("history", ui->combostyle_3->currentIndex());
           settings.setValue("language", ui->combostyle_2->currentIndex());
           settings.sync();

        if(ui->checkBox_3->isChecked()){ if (!minimize_already_checked) minimize_to_tray=1; }
        else minimize_to_tray=0;

        if(ui->checkBox_4->isChecked()) _show_notification_=1;
        else _show_notification_=0;

        if(ui->combostyle_3->currentIndex()==0) _save_history_=0;
        else _save_history_=1;

        history_type=ui->combostyle_3->currentIndex();

if(ui->playsoundcheckBox->isChecked())
{
    if(ui->customsoundcheckBox->isChecked() && (ui->browsesoundButton->text() != "(None)" || ui->browsesoundButton->text() != QString::fromUtf8("(Κανένα)")) )
    {
        _sound_notification_=2;
        settings.setValue("sound", true);
        settings.setValue("custom", true);
    }

    if(ui->browsesoundButton->text() == "(None)" || !ui->customsoundcheckBox->isChecked())
    {
        _sound_notification_=1;
        settings.setValue("sound", true);
        settings.setValue("custom", false);
    }
}
    else
{
        _sound_notification_=0;
        settings.setValue("sound", false);
        settings.setValue("custom", false);
}

    QString type;
    if(ui->combostyle->currentIndex()==0)
        type="wallpaper";
    else if(ui->combostyle->currentIndex()==1)
        type="zoom";
    else if(ui->combostyle->currentIndex()==2)
        type="centered";
    else if(ui->combostyle->currentIndex()==3)
        type="scaled";
    else if(ui->combostyle->currentIndex()==4)
        type="stretched";
    else if(ui->combostyle->currentIndex()==5)
        type="spanned";
    if(system(QString("gconftool-2 --type string --set /desktop/gnome/background/picture_options "+type+"&").toLocal8Bit().data()))
        cerr << "Error while trying to change style. Change your home's permissions.\n";


    close();
}

void preferences::on_playsoundcheckBox_clicked()
{
    if (ui->playsoundcheckBox->isChecked()){
        ui->customsoundcheckBox->setEnabled(true);
        if(ui->customsoundcheckBox->isChecked())
            ui->browsesoundButton->setEnabled(true);
    }
    else
    {
        ui->customsoundcheckBox->setEnabled(false);
        ui->customsoundcheckBox->setChecked(false);
        ui->browsesoundButton->setEnabled(false);

    }
}


void preferences::on_customsoundcheckBox_clicked()
{
        if (ui->customsoundcheckBox->isChecked()){ui->browsesoundButton->setEnabled(true);}
        if (!ui->customsoundcheckBox->isChecked()){ui->browsesoundButton->setEnabled(false);}
}



void preferences::on_browsesoundButton_clicked()
{
    QString path;
        path = QFileDialog::getOpenFileName(this, tr("Choose Custom Sound"), QDir::homePath(), "*.mp3 *.wav *.ogg");
           if ( path != ""){


               if(system("echo \"\" > ~/.config/Wallch/Checks/text_to_button"))
                   cerr << "Error writing to ~/.config/Wallch/Checks/text_to_button. Check file's permissions.\n";
               //QString realpath = path.toUtf8();
               char *realpath = path.toUtf8().data();
               QString home16 = QDir::homePath()+"/.config/Wallch/Checks/text_to_button";
               QFile file(home16);
               if (!file.open(QIODevice::WriteOnly)) {
                   cerr << "Cannot open file ~/.config/Wallch/Checks/text_to_button for writing. The reason: "
                        << qPrintable(file.errorString()) << endl;
                   cerr << "Try again.";
                   if(system("echo "" > ~/.config/Wallch/Checks/text_to_buton"))
                       cerr << "Error, could not write to ~/.config/Wallch/Checks/text_to_buton. Check file permissions.\n";
          }
               QTextStream out(&file);
               out << QString::fromUtf8(realpath);
               file.close();
               if(system("cat ~/.config/Wallch/Checks/text_to_button; a=`cat ~/.config/Wallch/Checks/text_to_button`; echo ${a##*/} > ~/.config/Wallch/Checks/text_to_button_name_only;"))
                   cerr << "Error reading/writing ~/.config/Wallch/Checks/text_to_button*. Please check files' permissions.\n";
                  fstream infile;
                  infile.open(QString(home16+"_name_only").toLocal8Bit().data()); // open file
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
        cerr << "Error while trying to kill mpg321 mpg123 and canberra-gtk-play\n";
    }
}

void preferences::on_pushButton_2_clicked()
{
    QMessageBox msgBox2;
    QPushButton *ok;
    msgBox2.setWindowTitle(tr("Reset Preferences"));
    msgBox2.setText(tr("Are you sure you want to reset your Wallch\npreferences?"));
    ok = msgBox2.addButton(tr("Ok"), QMessageBox::ActionRole);
    msgBox2.addButton(tr("Cancel"), QMessageBox::ActionRole);
    msgBox2.setIconPixmap(QIcon::fromTheme("dialog-question").pixmap(QSize(48,48)));
    msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
    msgBox2.exec();
    if (msgBox2.clickedButton() == ok) {
        ui->radioButton->setChecked(false);
        ui->radioButton_2->setChecked(false);
        ui->checkBox_3->setChecked(false);
        ui->playsoundcheckBox->setChecked(true);
        ui->customsoundcheckBox->setChecked(false);
        ui->checkBox_4->setChecked(true);
        ui->combostyle_3->setCurrentIndex(2);
        ui->checkBox_5->setChecked(false);
        ui->radioButton->setChecked(true);
        ui->radioButton->setEnabled(false);
        ui->radioButton_2->setEnabled(false);
        ui->radioButton_3->setChecked(false);
        ui->radioButton_3->setEnabled(false);
        on_aplyButton_clicked();
    }
    else
        return;
}
