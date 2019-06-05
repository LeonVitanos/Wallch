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

#include "history.h"
#include "ui_history.h"
#include "glob.h"

#include <QDir>
#include <QString>
#include <QRegExp>

#include <fstream>
#include <iostream>
using namespace std;

int image_count=0;

history::history(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::history)
{
    ui->setupUi(this);
    createOptionsGroupBox();
    createButtonsLayout();
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(optionsGroupBox);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
    updater();

}

void history::updater(){
    //taking the number of total images saved in history....
    //this will take the output of the command executed and it will place it to the right place (n)
    FILE* pipe = popen("if [ -s ~/.config/Wallch/History ]; then if [ \"$(ls -A ~/.config/Wallch/History/)\" ]; then for i in ~/.config/Wallch/History/*/*; do a=$(expr $a + $(wc -l < \"$i\")); done; echo $a; else echo 0; fi; else echo 0; fi", "r");
    char buffer[128];
    string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
        }
    pclose(pipe);
    QString res = QString::fromStdString(result);
    res.replace(QString("\n"),QString(""));
    ////
    //Here starts the actual action
    ui->treeWidget->setHeaderLabel(tr("History Entries")+" ("+res+")");

    //Months
    QString Jan = "January";
    QString Feb = "February";
    QString Mar = "March";
    QString Apr = "April";
    QString May = "May";
    QString Jun = "June";
    QString Jul = "July";
    QString Aug = "August";
    QString Sep = "September";
    QString Oct = "October";
    QString Nov = "November";
    QString Dec = "December";

    QDir dir;
    dir.setPath(QString(QDir::homePath() + "/.config/Wallch/History/"));
    int files_count = dir.entryList().count();
    QList<QTreeWidgetItem*> list;
    if(files_count>=3){
        QDir subdir1;
        QString temp,temp2;
        QRegExp sep("^* ");
        int months_and_years[files_count-2];
        for(int i=2; i < files_count; i++){ //i=2 and not 0 in order to avoid the paths .(current) and ..(one back)
            //this for loop fixes the order of the paths so as to be in the right order when imported
            temp=dir.entryList().at(i);
            temp2=temp.section(sep,1);
            if( temp2 == Jan)
                temp.replace(Jan,QString("01"));
            else if(temp2 == Feb)
                temp.replace(Feb,QString("02"));
            else if(temp2 == Mar)
                temp.replace(Mar,QString("03"));
            else if(temp2 == Apr)
                temp.replace(Apr,QString("04"));
            else if(temp2 == May)
                temp.replace(May,QString("05"));
            else if(temp2 == Jun)
                temp.replace(Jun,QString("06"));
            else if(temp2 == Jul)
                temp.replace(Jul,QString("07"));
            else if(temp2 == Aug)
                temp.replace(Aug,QString("08"));
            else if(temp2 == Sep)
                temp.replace(Sep,QString("09"));
            else if(temp2 == Oct)
                temp.replace(Oct,QString("10"));
            else if(temp2 == Nov)
                temp.replace(Nov,QString("11"));
            else if(temp2 == Dec)
                temp.replace(Dec,QString("12"));

            temp.replace(QString(" "), QString(""));
            months_and_years[i-2]=temp.toInt();
        }
        //now months_and_years[] contains all the months and years in this form: 201104 which means 'April of 2011', or 201111 which means 'November of 2011'
        //all this happened in order to do a bubble sort and the dates to be in the accurate order...
        //A bubble sort takes place:

        for(int y=0;y<files_count-2;y++){
            for (int k=0;k<files_count-3-y;k++)
                if(months_and_years[k]>months_and_years[k+1]){
                    int temp = months_and_years[k+1];
                    months_and_years[k+1] = months_and_years[k];
                    months_and_years[k] = temp;
                }
        }
        //now the numbers inside the months_and_years[] are in the correct order, so turning them back to strings...
        QString right_order_string[files_count-2], num_month;
        for ( int j = 0; j < files_count-2; j++ ){
            //turning the integers back to strings...
           right_order_string[j]=QString::number(months_and_years[j]);
           //getting the number of month (i.e. 03 or 12) which is the last two digits of the string
            num_month=right_order_string[j].right(2);
            //choping the last 2 digits of the string...
            right_order_string[j].chop(2);
            if(num_month=="01")
                right_order_string[j].append(QString(" " + Jan));
            else if(num_month=="02")
                right_order_string[j].append(QString(" " + Feb));
            else if(num_month=="03")
                right_order_string[j].append(QString(" " + Mar));
            else if(num_month=="04")
                right_order_string[j].append(QString(" " + Apr));
            else if(num_month=="05")
                right_order_string[j].append(QString(" " + May));
            else if(num_month=="06")
                right_order_string[j].append(QString(" " + Jun));
            else if(num_month=="07")
                right_order_string[j].append(QString(" " + Jul));
            else if(num_month=="08")
                right_order_string[j].append(QString(" " + Aug));
            else if(num_month=="09")
                right_order_string[j].append(QString(" " + Sep));
            else if(num_month=="10")
                right_order_string[j].append(QString(" " + Oct));
            else if(num_month=="11")
                right_order_string[j].append(QString(" " + Nov));
            else if(num_month=="12")
                right_order_string[j].append(QString(" " + Dec));
        }

        QDir his_file;
        int dir1_count;

        for(int i=2; i<files_count; i++){
            QTreeWidgetItem *parnt = new QTreeWidgetItem;
            //this for loop  imports the month and year (yearspacemonth) in the right order
            subdir1.setPath(QString(dir.path() + "/" + right_order_string[i-2]));
            parnt->setText(0,right_order_string[i-2]);
            dir1_count=subdir1.entryList().count();
            for(int j=2;j<dir1_count;j++){
                QTreeWidgetItem *child1 = new QTreeWidgetItem;
                his_file.setPath(QString(subdir1.path() + "/" + subdir1.entryList().at(j)));
                child1->setText(0,his_file.dirName());
                parnt->insertChild(j-2,child1);
            }
            list.insert(i-2,parnt);
        }
        ui->treeWidget->addTopLevelItems(list);
    }
    else{
        ui->info->clear();
        ui->info->addItem("No History available");
    }
}

history::~history()
{
    delete ui;
}

void history::on_closeButton_clicked()
{
     this->close();
}

bool history::removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
                return result;
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

void history::on_pushButton_clicked()
{
    if(!removeDir(QDir::homePath()+"/.config/Wallch/History"))
        cerr << "Error! Could not delete ~/.config/Wallch/History/, check folder's and subfolders' permissions.";
    ui->treeWidget->clear();
    ui->info->clear();
    ui->info->addItem("No History available");
    ui->treeWidget->setHeaderLabel(tr("History Entries")+" (0)");
}

void history::on_treeWidget_itemClicked(QTreeWidgetItem* item)
{
    ui->info->clear();
    if(item->childCount()>0){//A parent item has been selected, loop through their children and output to the listwidget their input text
        QFont bold;
        bold.setBold(true); //this fond will be used to color the days, where below of each, there will be the images
        for(int i=0;i<item->childCount();i++){
            QListWidgetItem *item_to_list = new QListWidgetItem;
            item_to_list->setFont(bold); //making the date bold...
            item_to_list->setText(item->child(i)->text(0));
            ui->info->addItem(item_to_list);
            QString qhistory_file = QDir::homePath() + "/.config/Wallch/History/" + item->text(0) + "/" + item->child(i)->text(0); //taking the history file, it is at ~/.config/Wallch/History folder, and the subfolders come from the parent and the childrens
            char *history_file= qhistory_file.toLatin1().data();
            FILE *file = fopen ( history_file, "r" );
            if (file!=NULL)
                {
                   ifstream file(history_file);
                   string line1;
                   while(getline(file,line1))
                   {
                       QString qstr = QString::fromUtf8(line1.c_str());
                       ui->info->addItem(qstr); //adding each line of the file to the listwidget...
                   }
               }
        }

    }
    else{//a child has been selected... So just print the containings of the file....
        QString qhistory_file = QDir::homePath() + "/.config/Wallch/History/" + item->parent()->text(0) + "/" + item->text(0);
        char *history_file= qhistory_file.toLatin1().data();
        FILE *file = fopen ( history_file, "r" );
               if (file!=NULL)
               {
                   ifstream file(history_file);
                   string line1;
                   while(getline(file,line1))
                   {
                       QString qstr = QString::fromUtf8(line1.c_str());
                       ui->info->addItem(qstr);
                   }
               }
    }
}

void history::on_pushButton_2_clicked()
{
    ui->treeWidget->clear();
    ui->info->clear();
    updater();
}

void history::createOptionsGroupBox()
{
    optionsGroupBox = new QGroupBox();
    optionsGroupBoxLayout = new QGridLayout;
    optionsGroupBoxLayout->addWidget(ui->treeWidget, 0, 0);
    optionsGroupBoxLayout->addWidget(ui->info, 0, 1);
    optionsGroupBox->setLayout(optionsGroupBoxLayout);
}

void history::createButtonsLayout()
{
    buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(ui->pushButton);
    buttonsLayout->addWidget(ui->pushButton_2);
    buttonsLayout->addWidget(ui->closeButton);
}
