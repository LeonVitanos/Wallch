/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define QT_NO_KEYWORDS
#define CUSTOM_ENTRY_COLOR 59, 89, 152

#include "history.h"
#include "ui_history.h"
#include "glob.h"
#include "wallpapermanager.h"

#include <QDir>
#include <QMessageBox>
#include <QShortcut>
#include <QClipboard>
#include <QDate>
#include <QSettings>
#include <QDesktopServices>

History::History(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::history)
{
    propertiesShown_=false;
    ui->setupUi(this);
    readHistoryFiles();
    (void) new QShortcut(Qt::Key_Return, this, SLOT(on_info_doubleClicked()));
}

History::~History()
{
    delete ui;
}

void History::on_closeButton_clicked()
{
    this->close();
}

void History::readHistoryFiles(){

    QSettings historySettings(HISTORY_SETTINGS);
    int totalWallpapers=0;
    QStringList years = historySettings.childGroups();
    Q_FOREACH (QString year, years) {
        historySettings.beginGroup(year);
        QStringList months = historySettings.childGroups();
        Q_FOREACH (QString month, months) {
            //add top level item, eg. December (2014)
            QTreeWidgetItem *month_year_item = new QTreeWidgetItem;
            month_year_item->setText(0, QDate::longMonthName(month.toInt())+" ("+year+")");
            month_year_item->setData(0,12, year);
            month_year_item->setData(1,12, month);
            ui->treeWidget->addTopLevelItem(month_year_item);

            historySettings.beginGroup(month);
            QStringList days = historySettings.childGroups();
            int index=0;
            Q_FOREACH (QString day, days) {
                //add day of month child (1-31)
                QTreeWidgetItem *day_item = new QTreeWidgetItem;
                if(day.toInt()<10){
                    day_item->setText(0, day.right(1));
                }
                else{
                    day_item->setText(0, day);
                }
                ui->treeWidget->topLevelItem(ui->treeWidget->topLevelItemCount()-1)->insertChild(index, day_item);

                index++;
                totalWallpapers+=historySettings.beginReadArray(day);
                historySettings.endArray();
            }
            historySettings.endGroup();
        }
        historySettings.endGroup();
    }

    ui->treeWidget->setHeaderLabel(tr("History Entries")+" ("+QString::number(totalWallpapers)+")");
}

void History::on_treeWidget_itemClicked(QTreeWidgetItem* item)
{
    ui->info->clear();
    QSettings historySettings(HISTORY_SETTINGS);
    if(item->childCount()>0)
    {//A parent item has been selected, loop through their children and output to the listwidget their input text
        QFont bold;
        bold.setBold(true); //this fond will be used to color the days, where below of each, there will be the images of each one
        historySettings.beginGroup(item->data(0,12).toString());
        historySettings.beginGroup(item->data(1,12).toString());

        QStringList days = historySettings.childGroups();
        Q_FOREACH (QString day, days) {
            //add days of month(0-31)
            QListWidgetItem *day_item = new QListWidgetItem;
            day_item->setFont(bold);
            day_item->setText(day);
            ui->info->addItem(day_item);

            int size=historySettings.beginReadArray(day);
            for(int i=0; size>i; i++)
            {
                historySettings.setArrayIndex(i);
                addHistoryEntry(historySettings.value("time").toString(), historySettings.value("path").toString(), historySettings.value("type").toInt());
            }
            historySettings.endArray();
        }

        historySettings.endGroup();
        historySettings.endGroup();
    }
    else
    {//a child has been selected... output to the text of this child to the listwidget
        historySettings.beginGroup(item->parent()->data(0,12).toString());
        historySettings.beginGroup(item->parent()->data(1,12).toString());

        int size=historySettings.beginReadArray(numberWithLeadingZero(item->text(0)));
        for(int j=0; size>j; j++)
        {
            historySettings.setArrayIndex(j);
            addHistoryEntry(historySettings.value("time").toString(), historySettings.value("path").toString(), historySettings.value("type").toInt());
        }
        historySettings.endArray();

        historySettings.endGroup();
        historySettings.endGroup();
    }
}

void History::addHistoryEntry(QString time, QString path, short type){
    QListWidgetItem *item = new QListWidgetItem;

    if(type==1)
    {
        item->setData(11, path);
        item->setData(12, "file");
        item->setText(time+" "+path);
        if(QFile::exists(path)){
            item->setToolTip("<img src=\""+path+"\" height=\"150\" width=\"200\"></img>");
        }
        else
        {
            item->setToolTip(tr("This image doesn't seem to exist!"));
            QFont font;
            font.setStrikeOut(true);
            item->setFont(font);
        }
    }
    else if(type!=0)
    {
        item->setToolTip(path);
        item->setForeground(QColor::fromRgb(CUSTOM_ENTRY_COLOR));
        if(type==2)
        {
            item->setText(time+" "+tr("Live Earth Image"));
            item->setData(12, "link");
        }
        else if(type==3)
        {
            item->setText(time+" "+tr("Picture Of The Day Image"));
            item->setData(12, "link");
        }
        else if(type==4)
        {
            item->setText(time+" "+tr("Wallpaper Clock Image"));
            item->setData(12, "clock");
        }
        else if(type==5)
        {
            item->setText(time+" "+tr("Live Website Image"));
            item->setData(12, "link");
        }
    }

    ui->info->addItem(item);
}

void History::on_info_customContextMenuRequested()
{
    if (ui->info->count() > 0){
        if(ui->info->currentIndex().isValid() && ui->info->currentItem()->isSelected() && ui->info->currentItem()->text().count()>2)
        {
            infoMenu_ = new QMenu(this);

            QString data=ui->info->currentItem()->data(12).toString();
            if(data=="file")
            {
                if(!QFile(ui->info->currentItem()->data(11).toString()).exists()){
                    return;
                }
                infoMenu_->addAction(tr("Open Folder"),this,SLOT(openFolder()));
                infoMenu_->addAction(tr("Copy path to clipboard"), this, SLOT(copyPath()));
                infoMenu_->addAction(tr("Properties"),this,SLOT(showProperties()));
            }
            else if(data=="link")
            {
                infoMenu_->addAction(tr("Launch in Browser"), this, SLOT(launchInBrowser()));
                infoMenu_->addAction(tr("Copy link to clipboard"), this, SLOT(copyLink()));
            }
            else{
                return;
            }
            infoMenu_->popup(MENU_POPUP_POS);
        }
    }
}

void History::openFolder(){
    if(ui->info->currentItem()->isSelected()){
        WallpaperManager::openFolderOf(ui->info->currentItem()->data(11).toString());
    }
}

void History::copyPath(){
    if(ui->info->currentItem()->isSelected()){
        QApplication::clipboard()->setText(ui->info->currentItem()->data(11).toString());
    }
}

void History::showProperties(){
    if(propertiesShown_ || !ui->info->currentItem()->isSelected()){
        return;
    }
    QString image_file=ui->info->currentItem()->data(11).toString();

    if(!QFile::exists(image_file) || QImage(image_file).isNull())
    {
        QMessageBox::warning(this, tr("Properties"), tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
        return;
    }

    propertiesShown_=true;
    historyProperties_ = new Properties(image_file, false, 0, this);
    historyProperties_->setModal(true);
    historyProperties_->setAttribute(Qt::WA_DeleteOnClose);
    connect(historyProperties_, SIGNAL(destroyed()), this, SLOT(historyPropertiesDestroyed()));
    historyProperties_->show();
}

void History::historyPropertiesDestroyed(){
    propertiesShown_=false;
}

void History::launchInBrowser(){
    if((ui->info->currentItem()->isSelected() || ui->info->currentItem()->toolTip().isEmpty()) &&
            !QDesktopServices::openUrl(QUrl(ui->info->currentItem()->toolTip()))){
        Global::error("I probably could not show the image to your browser!");
    }
}

void History::copyLink(){
    if(ui->info->currentItem()->isSelected()){
        QApplication::clipboard()->setText(ui->info->currentItem()->toolTip());
    }
}

void History::on_info_doubleClicked()
{
    if (ui->info->count() > 0){
        if(ui->info->currentIndex().isValid() && ui->info->currentItem()->isSelected() && ui->info->currentItem()->text().count()>2)
        {
            QString type=ui->info->currentItem()->data(12).toString();
            if(type=="file")
            {
                if(!QFile(ui->info->currentItem()->data(11).toString()).exists()){
                    return;
                }
                if(!QDesktopServices::openUrl(QUrl("file:///"+ui->info->currentItem()->data(11).toString()))){
                    Global::error("I probably could not open "+ui->info->currentItem()->data(12).toString());
                }
            }
            else if (type=="link"){
                launchInBrowser();
            }
        }
    }
}

void History::on_remove_history_clicked()
{
    QSettings historySettings(HISTORY_SETTINGS);
    historySettings.clear();
    historySettings.sync();
    ui->treeWidget->clear();
    ui->info->clear();
    ui->treeWidget->setHeaderLabel(tr("History Entries")+" (0)");
}

void History::on_treeWidget_customContextMenuRequested()
{
    treeWidgetMenu_ = new QMenu(this);
    treeWidgetMenu_->addAction(tr("Remove this entry"),this,SLOT(removeHistoryEntry()));

    if(!ui->treeWidget->currentIndex().isValid() || !ui->treeWidget->currentItem()->isSelected())
    {
        treeWidgetMenu_->actions().at(0)->setEnabled(false);
    }
    treeWidgetMenu_->popup(MENU_POPUP_POS);
}

void History::removeHistoryEntry(){
    QSettings historySettings(HISTORY_SETTINGS);
    if(ui->treeWidget->currentItem()->childCount()>=1){
        //a parent has been selected for deletion
        historySettings.remove(ui->treeWidget->currentItem()->data(0,12).toString()+"/"+ui->treeWidget->currentItem()->data(1,12).toString());
    }
    else if(ui->treeWidget->currentItem()->parent()->childCount()==1){
        //a child item has been selected for deletion, it is the only child so delete also the parent
        historySettings.remove(ui->treeWidget->currentItem()->parent()->data(0,12).toString()+"/"+ui->treeWidget->currentItem()->parent()->data(1,12).toString());
    }
    else
    {
        //a child item has been selected for deletion
        historySettings.remove(ui->treeWidget->currentItem()->parent()->data(0,12).toString()+"/"+ui->treeWidget->currentItem()->parent()->data(1,12).toString()+"/"+numberWithLeadingZero(ui->treeWidget->currentItem()->text(0)));
    }
    ui->treeWidget->clear();
    readHistoryFiles();
    ui->info->clear();
}

QString History::numberWithLeadingZero(QString number)
{
    if(number.toInt() < 10){
        return "0"+number;
    }
    else{
        return number;
    }
}
