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

#include "extras.h"
#include "ui_extras.h"

#include "glob.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

#include <iostream>
#include <fstream>
using namespace std;

Extras::Extras(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Extras)
{
    ui->setupUi(this);
    QSettings settings( "Wallch", "Extras" );
                    ui->use_current_radioButton->setChecked(settings.value("use_current_lock", false).toBool());
                    ui->image_from_disk_lock->setChecked(settings.value("use_image_from_disk_lock", false).toBool());

    if(ui->image_from_disk_lock->isChecked()) {ui->pushButton_10->setEnabled(true); ui->lineEdit_2->setEnabled(true);}
    else {ui->pushButton_11->setEnabled(true); set_current();}
    ui->toolBox->setItemIcon(0, QIcon::fromTheme("emblem-web"));
    ui->toolBox->setItemIcon(1, QIcon::fromTheme("system-lock-screen"));
    if(_live_earth_running_)
        ui->pushButton_6->setEnabled(false);
    else
        ui->pushButton_7->setEnabled(false);
}

Extras::~Extras()
{
    delete ui;
}

void Extras::closeEvent( QCloseEvent * )
{
    //writing current values
    QSettings
       settings( "Wallch", "Extras" );
       settings.setValue("use_current_lock", ui->use_current_radioButton->isChecked());
       settings.setValue("use_image_from_disk_lock", ui->image_from_disk_lock->isChecked());
       settings.sync();
}

void Extras::on_pushButton_11_clicked()
{
    if (ui->use_current_radioButton->isChecked())
         {
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

             if(!QFile(current_wallpaper).exists() || QImage(current_wallpaper).isNull()){
                 QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText(tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec();
                 return;
             }
                 if(system("gksudo --description 'Change Lock Screen Wallpaper' \"gconftool-2 --direct --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults --set /desktop/gnome/background/picture_filename --type string '$(gconftool-2 --get /desktop/gnome/background/picture_filename)'\""))
                     cerr << "Error while trying to use command 'gconftool-2'\n";
         }
    else {
        if(!QFile(ui->lineEdit_2->text().toAscii()).exists() || QImage(ui->lineEdit_2->text()).isNull()){
            QMessageBox msgBox;msgBox.setWindowTitle("Error!");msgBox.setText(tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(80,80)));msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));msgBox.exec();
        return;
        }
        QString qcommand;
            qcommand="gksudo --description 'Change Lock Screen Wallpaper' \"gconftool-2 --direct --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults --set /desktop/gnome/background/picture_filename --type string '"+ui->lineEdit_2->text()+"'\"";
        //cout << qcommand.toLocal8Bit().data();
        if(system(qcommand.toLocal8Bit().data()))
            cerr << "Error while trying to use command 'gconftool-2'\n";
    }
    //this will restart it so as to work
    if(system("killall -v gconfd-2 gnome-screensaver 2> /dev/null"))
        cerr << "Killing command 'gconftool-2' and 'gnome-screensaver' returned code different from 0\n";
}

void Extras::on_pushButton_10_clicked()
{
    QString Qpath;
    Qpath = QFileDialog::getOpenFileName(this, tr("Choose Image"), QDir::homePath(), "*png *gif *bmp *jpg *jpeg");
    if(Qpath.count() == 0) return;
    ui->lineEdit_2->setText(Qpath);
    ui->pushButton_11->setEnabled(true);
    QString filename = ui->lineEdit_2->text();
    QImage image(filename);
    ui->preview_lock->setPixmap(QPixmap::fromImage(image));
}

void Extras::on_use_current_radioButton_clicked()
{
    ui->pushButton_10->setEnabled(false);
    ui->lineEdit_2->setEnabled(false);
    ui->pushButton_11->setEnabled(true);
    set_current();
}

void Extras::on_image_from_disk_lock_clicked()
{
    ui->pushButton_10->setEnabled(true); ui->lineEdit_2->setEnabled(true);
    if(ui->lineEdit_2->text()!="") {ui->pushButton_11->setEnabled(true); ui->preview_lock->setPixmap(ui->lineEdit_2->text());}
    else {ui->pushButton_11->setEnabled(false); ui->preview_lock->clear();}
}

void Extras::on_pushButton_12_clicked()
{
    if(system("gksudo --description 'Change Lock Screen Wallpaper' \"gconftool-2 --direct --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults --unset /desktop/gnome/background/picture_filename\""))
        cerr << "Error while trying to use command 'gconftool-2'\n";

    if(system("killall -v gconfd-2 gnome-screensaver 2> /dev/null"))
            cerr << "Error while trying to use command 'gconftool-2'\n";

}

void Extras::on_closeButton_clicked()
{
    close();
}

void Extras::set_current()
{
    FILE* pipe= popen("gconftool-2 --get /desktop/gnome/background/picture_filename", "r");
    char buffer[128];
    string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
        }
    pclose(pipe);

    QString current_wallpaper = QString::fromStdString(result);
    current_wallpaper.replace(QString("\n"),QString(""));
    if(!QFile(current_wallpaper).exists() || QImage(current_wallpaper).isNull()) return;

    ui->preview_lock->setPixmap(QPixmap::fromImage(QImage(current_wallpaper)));
}

void Extras::on_pushButton_6_clicked()
{
    if(_start_running_){
    //Start is running, in order to avoid conflicts display an error message!
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Live Earth Wallpaper"));
        msgBox.setText(tr("Please stop the current process and try again."));
            msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
            msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
            msgBox.exec();
            return;
    }
    Q_EMIT start_live_earth();
    ui->pushButton_7->setEnabled(true);
    ui->pushButton_6->setEnabled(false);

}

void Extras::on_pushButton_7_clicked()
{
    _live_earth_running_=0;
    Q_EMIT stop_live_earth();
    ui->pushButton_7->setEnabled(false); ui->pushButton_6->setEnabled(true);
}

void Extras::fix_buttons(){
    ui->pushButton_6->setEnabled(false);
    ui->pushButton_7->setEnabled(true);
}
