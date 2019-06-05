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

#include "properties.h"
#include "ui_properties.h"
#include "glob.h"

#include <fstream>
#include <iostream>

using namespace std;
char *user=getenv("USER");

properties::properties(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::properties)
{
    ui->setupUi(this);

    if(_language_==2){
       properties::setWindowTitle(QString::fromUtf8("Ιδιότητες"));
       ui->label_6->setText(QString::fromUtf8("Ύψος:"));
       ui->label_7->setText(QString::fromUtf8("Πλάτος:"));
       ui->label_5->setText(QString::fromUtf8("Τοποθεσία:"));
       ui->label_2->setText(QString::fromUtf8("Όνομα:"));
       ui->label_4->setText(QString::fromUtf8("Μέγεθος:"));
       ui->label_3->setText(QString::fromUtf8("Τύπος:"));
   }

    //getting some info
        if(system("image=`cat /home/$USER/.config/Wallch/Checks/image_path`; identify -format '%h\n' \"$image\" > /home/$USER/.config/Wallch/Checks/image_h; identify -format '%w\n' \"$image\" > /home/$USER/.config/Wallch/Checks/image_w; basename \"$image\" > /home/$USER/.config/Wallch/Checks/image_name; dirname \"$image\" > /home/$USER/.config/Wallch/Checks/image_dir;"))
            cout << "Error, unable to read/write ~/.config/Wallch/image_path or image_h or image_w or image_name or image_dir. Please check these files for permission issues.\n";
        //getting size and type of image
        if(system("image=`cat /home/$USER/.config/Wallch/Checks/image_path`; a=$(stat -c %s \"$image\"); inMB=`echo \"scale=3;$a/1048576\" | bc`; firstdigit=`echo $inMB | cut -c1`; if [ X\"$firstdigit\" = X\".\" ]; then echo 1 > /home/$USER/.config/Wallch/Checks/starts_with_point; else echo 0 > /home/$USER/.config/Wallch/Checks/starts_with_point; fi; echo $inMB > /home/$USER/.config/Wallch/Checks/image_size; file -b \"$image\" > /home/$USER/.config/Wallch/Checks/image_type"))
            cout << "Error, unable to read/write ~/.config/Wallch/image_path or Checks/starts_with_point Checks/image_size or Checks/image_type. Please check these files for permission issues.\n";
        /*Setting up the name of the image!*/
        char home1[150]="/home/";
        strcat(home1,user);
        char other1[100]="/.config/Wallch/Checks/image_name";
        strcat(home1,other1);
        char line1 [ 300 ];
       FILE *file1 = fopen ( home1, "r" );
           if(fgets ( line1, sizeof line1, file1 )==NULL)
               cout << "Error: Either file's name  contains error or unable to read ~/.config/Wallch/Checks/image_name\n";
       fclose ( file1 );
       string name = string(line1);
       name.erase(name.length()-1);
       QString image_name = QString::fromUtf8(name.c_str());
       ui->image_name->setText(image_name);
       /*Setting up the type of the image!*/
       char home2[150]="/home/";
       strcat(home2,user);
       char other2[100]="/.config/Wallch/Checks/image_type";
       strcat(home2,other2);
       char line2 [ 128 ];
      FILE *file2 = fopen ( home2, "r" );
          if(fgets ( line2, sizeof line2, file2 )==NULL)
              cout << "Error reading ~/.config/Wallch/Checks/image_type\n";
      fclose ( file2 );
      string type = string(line2);
      type.erase(type.length()-2);
      QString image_type = QString::fromStdString(type);
      ui->image_type->setText(image_type);
      /*Setting up the size of the image!*/
      //If the size is lower than 1 MB, it starts with "." Checking if this is happening.
      int has_point=0;
      char homepoint[150]="/home/";
      strcat(homepoint,user);
      char otherpoint[100]="/.config/Wallch/Checks/starts_with_point";
      strcat(homepoint,otherpoint);
      char linepoint [ 128 ];
     FILE *filepoint = fopen ( homepoint, "r" );
         if(fgets ( linepoint, sizeof linepoint, filepoint )==NULL)
             cout << "Error reading ~/.config/Wallch/Checks/starts_with_point\n";
     fclose ( filepoint );
     if(!strcmp(linepoint,"1\n")) has_point=1;
     //checking the actual size
      char home3[150]="/home/";
      strcat(home3,user);
      char other3[100]="/.config/Wallch/Checks/image_size";
      strcat(home3,other3);
      char line3 [ 128 ];
     FILE *file3 = fopen ( home3, "r" );
         if(fgets ( line3, sizeof line3, file3 )==NULL)
             cout << "Error reading ~/.config/Wallch/Checks/image_size\n";
     fclose ( file3 ); //removing the \n :
     string size = string(line3);
     size.erase(size.length()-2);
     QString image_size = QString::fromStdString(size);
     if(has_point)
     ui->image_size->setText("0" + image_size + " MB");
     else
       ui->image_size->setText(image_size + " MB");
     /*Setting up the location*/
     char home4[150]="/home/";
     strcat(home4,user);
     char other4[100]="/.config/Wallch/Checks/image_dir";
     strcat(home4,other4);
     char line4 [ 128 ];
    FILE *file4 = fopen ( home4, "r" );
        if(fgets ( line4, sizeof line4, file4 )==NULL)
            cout << "Error, invalid image path or could not read ~/.config/Wallch/Checks/image_dir\n";
    fclose ( file4 );
    string location = string(line4);
    location.erase(location.length()-1);
    QString image_location = QString::fromUtf8(location.c_str());
    ui->image_location->setText(image_location);
    /*Setting up the width and height*/
    //width
    char home5[150]="/home/";
    strcat(home5,user);
    char other5[100]="/.config/Wallch/Checks/image_h";
    strcat(home5,other5);
    char line5 [ 128 ];
    FILE *file5 = fopen ( home5, "r" );
       if(fgets ( line5, sizeof line5, file5 )==NULL)
           cout << "Error, could not read ~/.config/Wallch/Checks/image_h\n";
    fclose ( file5 );

    string height = string(line5);
    height.erase(height.length()-1);
    QString image_height = QString::fromStdString(height);
    ui->image_height->setText(image_height + " px");
    //height
    char home6[150]="/home/";
    strcat(home6,user);
    char other6[100]="/.config/Wallch/Checks/image_w";
    strcat(home6,other6);
    char line6 [ 128 ];
    FILE *file6 = fopen ( home6, "r" );
       if(fgets ( line6, sizeof line6, file6 )==NULL)
           cout << "Error, could not read ~/.config/Wallch/Checks/image_w\n";
    fclose ( file6 );
    string width = string(line6);
    width.erase(width.length()-1);
    QString image_width = QString::fromStdString(width);
    ui->image_width->setText(image_width + " px");
    if(system("rm -rf /home/$USER/.config/Wallch/Checks/image_* /home/$USER/.config/Wallch/Checks/starts_with_point"))
        cout << "Error while trying to remove ~/.config/Wallch/Checks/image_* or starts_with_point. Please check these files for permission issues.\n";
}

properties::~properties()
{
    delete ui;
}



void properties::on_pushButton_clicked()
{
    close();
}
