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

#include "pictures_locations.h"
#include "ui_pictures_locations.h"
#include "glob.h"

PicturesLocations::PicturesLocations(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::pictures_locations)
{
    ui->setupUi(this);
}

PicturesLocations::~PicturesLocations()
{
    delete ui;
}

void PicturesLocations::on_add_location_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), gv.homePath);

    if(folder.isEmpty()){
        return;
    }

    short count=ui->foldersTreeWidget->topLevelItemCount();
    for(short i=0; i<count; i++)
    {
        if(folder==ui->foldersTreeWidget->topLevelItem(i)->text(1)){
            QMessageBox::warning(this, tr("Error"), tr("Folder")+" \""+folder+"\" "+tr("already exists."));
            return;
        }
    }

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, Global::basenameOf(folder));
    item->setText(1, (folder.endsWith('/') || folder.endsWith('\\')) ? folder.left(folder.count()-1) : folder );
    ui->foldersTreeWidget->addTopLevelItem(item);
}


void PicturesLocations::on_remove_location_clicked()
{
    delete ui->foldersTreeWidget->currentItem();
}

void PicturesLocations::on_foldersTreeWidget_currentItemChanged(QTreeWidgetItem *current)
{
   ui->remove_location->setEnabled(ui->foldersTreeWidget->currentIndex().row()>1 || current->parent());
}

void PicturesLocations::on_cancel_pushButton_clicked()
{
    close();
}

void PicturesLocations::on_save_pushButton_clicked()
{
    //We want to keep the folder that the user was using before the changes
    short cur_fol=settings->value("currentFolder_index", 0).toInt();
    settings->beginReadArray("pictures_locations");
    settings->setArrayIndex(cur_fol);
    bool cur_fol_type=settings->value("type").toBool();
    QString cur_fol_path=settings->value("item").toString();
    settings->endArray();
    short count=ui->foldersTreeWidget->topLevelItemCount();
    bool folder_found=false;
    for(short i=0; i < count; ++i)
    {
        if (cur_fol_path==(cur_fol_type ? ui->foldersTreeWidget->topLevelItem(i)->text(0):ui->foldersTreeWidget->topLevelItem(i)->text(1)))
        {
            settings->setValue("currentFolder_index", i);
            folder_found=true;
            break;
        }
    }
    if (!folder_found){
        settings->setValue("currentFolder_index", 0);
    }

    settings->beginWriteArray("pictures_locations");
    settings->remove("");
    for (short i = 0; i < count; ++i) {
        settings->setArrayIndex(i);
        if(ui->foldersTreeWidget->topLevelItem(i)->childCount()>0){
            //folder set
            short count=ui->foldersTreeWidget->topLevelItem(i)->childCount();
            settings->setValue("item", ui->foldersTreeWidget->topLevelItem(i)->text(0));
            settings->setValue("type", 1);
            settings->setValue("size", count);
            settings->beginWriteArray(QString::number(i));
            for (short j = 0; j < count; ++j) {
                settings->setArrayIndex(j);
                settings->setValue("item", ui->foldersTreeWidget->topLevelItem(i)->child(j)->text(1));
            }
            settings->endArray();
        }
        else{
            settings->setValue("item", ui->foldersTreeWidget->topLevelItem(i)->text(1));
        }
    }
    settings->endArray();
    settings->sync();
    Q_EMIT picturesLocationsChanged();
    close();
}

void PicturesLocations::on_reset_pushButton_clicked()
{
    settings->setValue("currentFolder_index", 0);
    settings->beginWriteArray("pictures_locations");
    settings->remove("");
    settings->setArrayIndex(0);
    settings->setValue("item", gv.currentDeDefaultWallpapersPath);
    settings->setArrayIndex(1);
    settings->setValue("item", gv.defaultPicturesLocation);
    settings->endArray();
    settings->sync();
    Q_EMIT picturesLocationsChanged();
    close();
}

TreeWidgetDrop::TreeWidgetDrop(QWidget *parent) :
    QTreeWidget(parent)
{
    setColumnWidth(0, 220);

    deBackgrounds_item = new QTreeWidgetItem;
    deBackgrounds_item->setText(0, gv.currentOSName+" "+tr("Desktop Backgrounds"));
    deBackgrounds_item->setText(1, gv.currentDeDefaultWallpapersPath);
    addTopLevelItem(deBackgrounds_item);

    pictures_item = new QTreeWidgetItem;
    pictures_item->setText(0, tr("My Pictures"));
    pictures_item->setText(1, gv.defaultPicturesLocation);
    addTopLevelItem(pictures_item);

    short size=settings->beginReadArray("pictures_locations");
    for(short i=2;i<size;i++){
        settings->setArrayIndex(i);
        QString folderPath=settings->value("item").toString();
        if(settings->value("type").toBool()){//folder set
            short setFolderCount=settings->beginReadArray(QString::number(i));

            QTreeWidgetItem *set_item = new QTreeWidgetItem;
            set_item->setText(0, folderPath);

            for(short j=0;j<setFolderCount;j++){
                settings->setArrayIndex(j);
                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(0, Global::basenameOf(settings->value("item", QString()).toString()));
                item->setText(1, settings->value("item", QString()).toString());
                set_item->addChild(item);
            }
            settings->endArray();

            addTopLevelItem(set_item);
            expandItem(set_item);
        }
        else
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, Global::basenameOf(folderPath));
            item->setText(1, folderPath);
            addTopLevelItem(item);
        }
    }
    settings->endArray();
}

void TreeWidgetDrop::dropEvent(QDropEvent *event)
{
    if(indexFromItem(selectedItems().at(0)).row()>1 || selectedItems().at(0)->parent())
    {
        QTreeWidget::dropEvent(event);
        fixPositions();
    }
}

void TreeWidgetDrop::fixPositions()
{
    //1. We want to make sure System Desktop Backgrounds and Pictures Items remain the first two
    short count=topLevelItemCount();
    if(topLevelItem(0)->text(1)!=gv.currentDeDefaultWallpapersPath)
    {
        for(short i=0; i<count; i++)
        {
            if(topLevelItem(i)==deBackgrounds_item)
            {
                takeTopLevelItem(i);
                insertTopLevelItem(0, deBackgrounds_item);
            }
        }
    }

    if(topLevelItem(1)!=pictures_item)
    {
        for(short i=0; i<count; i++)
        {
            if(topLevelItem(i)==pictures_item)
            {
                takeTopLevelItem(i);
                insertTopLevelItem(1, pictures_item);
            }
        }
    }

    //2. We do not want System Desktop Backgrounds and Pictures Items to have child items
    if(deBackgrounds_item->childCount()==1)
    {
        QTreeWidgetItem *temp = new QTreeWidgetItem;
        temp->setText(0, deBackgrounds_item->child(0)->text(0));
        temp->setText(1, deBackgrounds_item->child(0)->text(1));
        count=deBackgrounds_item->child(0)->childCount();
        for(short j=0; j<count; j++)
        {
            QTreeWidgetItem *temp2 = new QTreeWidgetItem;
            temp2->setText(0, deBackgrounds_item->child(0)->child(j)->text(0));
            temp2->setText(1, deBackgrounds_item->child(0)->child(j)->text(1));
            temp->addChild(temp2);
        }
        insertTopLevelItem(2, temp);
        deBackgrounds_item->removeChild(deBackgrounds_item->child(0));
    }

    if(pictures_item->childCount()==1)
    {
        QTreeWidgetItem *temp = new QTreeWidgetItem;
        temp->setText(0, pictures_item->child(0)->text(0));
        temp->setText(1, pictures_item->child(0)->text(1));
        count=pictures_item->child(0)->childCount();
        for(short j=0; j<count; j++)
        {
            QTreeWidgetItem *temp2 = new QTreeWidgetItem;
            temp2->setText(0, pictures_item->child(0)->child(j)->text(0));
            temp2->setText(1, pictures_item->child(0)->child(j)->text(1));
            temp->addChild(temp2);
        }
        insertTopLevelItem(2, temp);
        pictures_item->removeChild(pictures_item->child(0));
    }

    count=topLevelItemCount();
    for(short i=0; i<count; i++)
    {
        if(topLevelItem(i)->childCount()>0)
        {
            //3. We do not want to allow child items to have a child itself
            short count2=topLevelItem(i)->childCount();
            for(short j=0; j<count2; j++)
            {
                short count3=topLevelItem(i)->child(j)->childCount();
                if(count3>0)
                {
                    if(!topLevelItem(i)->child(j)->text(1).isNull())
                    {
                        QTreeWidgetItem *temp = new QTreeWidgetItem;
                        temp->setText(0, topLevelItem(i)->child(j)->text(0));
                        temp->setText(1, topLevelItem(i)->child(j)->text(1));
                        topLevelItem(i)->addChild(temp);

                        short count4=topLevelItem(i)->child(j)->child(0)->childCount();
                        if(count4==0)
                        {
                            QTreeWidgetItem *temp = new QTreeWidgetItem;
                            temp->setText(0, topLevelItem(i)->child(j)->child(0)->text(0));
                            temp->setText(1, topLevelItem(i)->child(j)->child(0)->text(1));
                            topLevelItem(i)->addChild(temp);
                        }
                        else{
                            //a folder set had been dragged to an item of another folder set
                            for(short k=0; k<count4; k++)
                            {
                                QTreeWidgetItem *temp2 = new QTreeWidgetItem;
                                temp2->setText(0, topLevelItem(i)->child(j)->child(0)->child(k)->text(0));
                                temp2->setText(1, topLevelItem(i)->child(j)->child(0)->child(k)->text(1));
                                topLevelItem(i)->addChild(temp2);
                            }
                        }
                    }
                    else{
                        for(short k=0; k<count3; k++)
                        {
                            QTreeWidgetItem *temp = new QTreeWidgetItem;
                            temp->setText(0, topLevelItem(i)->child(j)->child(k)->text(0));
                            temp->setText(1, topLevelItem(i)->child(j)->child(k)->text(1));
                            topLevelItem(i)->addChild(temp);
                        }
                    }
                    topLevelItem(i)->removeChild(topLevelItem(i)->child(j));
                }
            }

            //4. We want the folder set to be named as the list of all the folders it has
            if(!topLevelItem(i)->text(1).isNull())
            {
                QTreeWidgetItem *temp = new QTreeWidgetItem;
                temp->setText(0, topLevelItem(i)->text(0));
                temp->setText(1, topLevelItem(i)->text(1));
                topLevelItem(i)->addChild(temp);
                topLevelItem(i)->setText(1, NULL);
            }

            QString listOfBasenames;
            count2=topLevelItem(i)->childCount();
            for(short j=0; j<count2; j++)
            {
                listOfBasenames.append(topLevelItem(i)->child(j)->text(0));
                if(j<count2-1)
                    listOfBasenames.append("/");
            }
            topLevelItem(i)->setText(0, listOfBasenames);
            topLevelItem(i)->setExpanded(true);
        }

        //5. If a folder set has no folders, remove it
        if(topLevelItem(i)->text(1).isNull() && topLevelItem(i)->childCount()==0)
        {
            takeTopLevelItem(i);
            count--;
            i--;
        }
    }
}
