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

#define DEFAULT_SCALE 0.2
#define DEFAULT_POS 100, 100

#include "lepoint.h"
#include "glob.h"
#include "ui_lepoint.h"

#include <QDebug>
#include <QShortcut>
#include <QSettings>
#include <QFileDialog>
#include <QImageReader>
#include <QScreen>

LEPoint::LEPoint(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lepoint)
{
    ui->setupUi(this);

    scene_ = new QGraphicsScene(this);
    connect(scene_, SIGNAL(focusItemChanged(QGraphicsItem*,QGraphicsItem*,Qt::FocusReason)), this, SLOT(pointItemFocusChanged(QGraphicsItem*,QGraphicsItem*,Qt::FocusReason)));
    connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged()));
    ui->graphicsView->setScene(scene_);

    if (QFile::exists(gv.wallchHomePath+LE_POINT_IMAGE)) {
        // the image has already been fetched
        backgroundImageReady(gv.wallchHomePath+LE_POINT_IMAGE);
    } else {
        // hide the tools
        hideTools();
        // initiate the image fetch
        tryFetch_ = new TryHard(this, lePointImages_);
        connect(tryFetch_, SIGNAL(failed()), this, SLOT(cannotFetchLeImage()));
        connect(tryFetch_, SIGNAL(success(const QByteArray&)), this, SLOT(leImageFetchSuccess(const QByteArray&)));
        tryFetch_->start();
    }

    (void) new QShortcut(Qt::Key_Delete, this, SLOT(removeItem()));
}

LEPoint::~LEPoint()
{
    delete ui;
}

void LEPoint::showTools()
{
    // remove the progressbar
    ui->infoLabel->hide();
    ui->progressBar->hide();

    // make sure everything else is shown
    ui->graphicsView->show();
    ui->label->show();
    ui->scaleSlider->show();
    ui->label_2->show();
    ui->rotationSlider->show();
    ui->iconCombo->show();
    ui->addButton->show();
    ui->ok->show();
}

void LEPoint::hideTools()
{
    // remove the progressbar
    ui->infoLabel->show();
    ui->progressBar->show();

    // make sure everything else is hidden
    ui->graphicsView->hide();
    ui->label->hide();
    ui->scaleSlider->hide();
    ui->label_2->hide();
    ui->rotationSlider->hide();
    ui->iconCombo->hide();
    ui->addButton->hide();
    ui->ok->hide();
}

void LEPoint::enableTools()
{
    ui->scaleSlider->setEnabled(true);
    ui->rotationSlider->setEnabled(true);
    ui->iconCombo->setEnabled(true);
}

void LEPoint::disableTools()
{
    ui->scaleSlider->setEnabled(false);
    ui->rotationSlider->setEnabled(false);
    ui->scaleSlider->setValue(0);
    ui->rotationSlider->setValue(0);
    ui->iconCombo->setCurrentIndex(0);
}

void LEPoint::backgroundImageReady(const QPixmap &background)
{
    showTools();

    // setting background
    QGraphicsPixmapItem *backgroundItem = new QGraphicsPixmapItem();
    backgroundItem->setPixmap(background);

    backgroundHeight_ = background.height();
    backgroundWidth_ = background.width();

    ui->graphicsView->setMaximumHeight(background.height()+2);
    ui->graphicsView->setMinimumHeight(background.height()+2);
    ui->graphicsView->setMaximumWidth(background.width()+2);
    ui->graphicsView->setMinimumWidth(background.width()+2);
    ui->graphicsView->setSceneRect(background.rect());
    scene_->setSceneRect(background.rect());
    scene_->addItem(backgroundItem);

    //setting point item (hand, point etc)

    QSettings settings("wallch", "Settings");

    unsigned int imagesCount = settings.beginReadArray("le_mark_points");

    if (imagesCount == 0) {
        // load default settings
        MarkItem *pointItem = new MarkItem();
        pointItem->setPointType(PointItemType::Point1);
        pointItem->setScale(DEFAULT_SCALE);
        pointItem->setPos(DEFAULT_POS);
        pointItem->setSelected(false);
        marks_.append(pointItem);
        scene_->addItem(pointItem);
    } else {
        for (unsigned int i = 0; i < imagesCount; i++) {
            settings.setArrayIndex(i);

            MarkItem *pointItem = new MarkItem();
            pointItem->setPointType(PointItemType::CustomPoint, settings.value("icon", ":/images/point1.png").toString());
            pointItem->setScale(settings.value("scale", 1).toReal());
            pointItem->setRotation(settings.value("rotation", 0).toReal());

            QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint();

            pointItem->setPos(pos.x(), pos.y());
            pointItem->setSelected(false);

            marks_.append(pointItem);
            scene_->addItem(pointItem);
        }
    }

    settings.endArray();
}

void LEPoint::removeItem(){
    Q_FOREACH(MarkItem *pointItem, marks_){
        if(pointItem->isSelected()){
            marks_.removeAll(pointItem);
            scene_->removeItem((QGraphicsItem*)pointItem);
            break;
        }
    }
}

void LEPoint::pointItemFocusChanged(QGraphicsItem *newFocusItem, QGraphicsItem *oldFocusItem, Qt::FocusReason reason){
    Q_UNUSED(reason)

    if(oldFocusItem != NULL){
        qDebug() << "Old focus is not null";
        oldFocusItem->setSelected(false);
    } else {
        qDebug() << "Old focus is null";
    }

    MarkItem *newItem = (MarkItem*)newFocusItem;

    if(newItem != NULL){
        qDebug() << "new focus is not null";
        enableTools();

        lastSelectedItem_ = newItem;
        newItem->setSelected(true);

        if(marks_.contains(newItem)){
            setOptionsValues(newItem);
        }

    } else {
        qDebug() << "new focus is null";
        disableTools();
    }
}

void LEPoint::sceneChanged(){
    Q_FOREACH(MarkItem *item, marks_){
        if(!scene_->items().contains((QGraphicsItem*)item)){
            marks_.removeAll(item);
            delete item;
        }
    }
}

void LEPoint::cannotFetchLeImage()
{
    fetchFailed_ = true;
    ui->progressBar->hide();
    ui->infoLabel->setText(tr("The Live Earth image used for setting the mark points, failed to download. Please check your internet connection or try later."));
    this->resize(this->minimumWidth(), this->minimumHeight());
    this->move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center());
}

void LEPoint::leImageFetchSuccess(const QByteArray &array)
{
    QPixmap background;
    background.loadFromData(array);
    background.save(gv.wallchHomePath+LE_POINT_IMAGE);

    backgroundImageReady(background);
}

void LEPoint::on_scaleSlider_valueChanged(int value)
{
    Q_FOREACH(MarkItem *pointItem, marks_){
        if(pointItem->isSelected()){
            pointItem->setScale(value/1000.0);
            break;
        }
    }
}

void LEPoint::on_rotationSlider_valueChanged(int value)
{
    Q_FOREACH(MarkItem *pointItem, marks_){
        if(pointItem->isSelected()){
            pointItem->setRotation(value);
            break;
        }
    }

    ui->rotationLabel->setText(QObject::tr("%1 degrees").arg(QString::number(value)));
}

void LEPoint::on_addButton_clicked()
{
    MarkItem *pointItem = new MarkItem();

    switch(ui->iconCombo->currentIndex()){
    default:
    case 0:
        pointItem->setPointType(PointItemType::Point1);
        break;
    case 1:
        pointItem->setPointType(PointItemType::Point2);
        break;
    case 2:
        pointItem->setPointType(PointItemType::CustomPoint, ui->iconCombo->itemData(ui->iconCombo->currentIndex(), Qt::UserRole).toString());
        break;
    }

    pointItem->setScale(DEFAULT_SCALE);
    pointItem->setRotation(0);
    pointItem->setPos(DEFAULT_POS);

    marks_.append(pointItem);
    scene_->addItem(pointItem);

    pointItem->setSelected(true);
}

void LEPoint::applyCustomIconToCombobox(const QString &path){
    alteringIndexesFromCode_ = true;
    ui->iconCombo->removeItem(2);
    if(ui->iconCombo->count() == 3){
        ui->iconCombo->removeItem(2);
    }
    ui->iconCombo->addItem(QIcon(QPixmap(path)), basenameOf(path), path);
    ui->iconCombo->addItem(tr("Custom Image"));
    ui->iconCombo->setCurrentIndex(2);
    alteringIndexesFromCode_ = false;
}

void LEPoint::on_iconCombo_currentIndexChanged(int index)
{
    if(alteringIndexesFromCode_ || !ui->iconCombo->isEnabled()){
        //do not run this function unless user generated the action through the UI
        return;
    }

    if(index == (ui->iconCombo->count()-1)){
        //select new custom image
        QString path = QFileDialog::getOpenFileName(this, tr("Select Image"), QDir::homePath());
        if(isValidImage(path)){
            applyCustomIconToCombobox(path);
            on_iconCombo_currentIndexChanged(2);
        }
        else
        {
            ui->iconCombo->setCurrentIndex(previousIconComboIndex);
        }
    }
    else
    {
        //one of the existent images was selected
        MarkItem *selectedItem = NULL;
        Q_FOREACH(MarkItem *pointItem, marks_){
            if(pointItem->isSelected()){
                selectedItem = pointItem;
                break;
            }
        }

        if (selectedItem == NULL) {
            return;
        }

        switch(index){
        default:
        case 0:
            selectedItem->setPointType(PointItemType::Point1);
            break;
        case 1:
            selectedItem->setPointType(PointItemType::Point2);
            break;
        case 2:
            selectedItem->setPointType(PointItemType::CustomPoint, ui->iconCombo->itemData(index, Qt::UserRole).toString());
            break;
        }

        setOptionsValues(selectedItem);
        selectedItem->setSelected(true);
    }
    previousIconComboIndex = ui->iconCombo->currentIndex();
}

QString LEPoint::basenameOf(const QString &path){
    /*
     * Returns the fullname of the file that 'path' points to. E.g.:
     * path='/home/alex/a.txt' -> basenameOf(path)=='a.txt'
     * This can be done with the QFileInfo as well, but it is too time&resource consuming
     */

    short pathCount=path.count();
    bool isItselfDir=false;
    for(short i=pathCount-1; i>=0; i--){
        if(path.at(i)=='/' || path.at(i)=='\\' ){
            if(i==(pathCount-1)){
                isItselfDir=true;
                continue; //it was a directory
            }
            if(isItselfDir){
                QString withDirSeparator = path.right(pathCount-i-1);
                return withDirSeparator.left(withDirSeparator.count()-1);
            }
            else
            {
                return path.right(pathCount-i-1);
            }
        }
    }

    return QString(); //it means that '/' or '\' does not exist
}

bool LEPoint::isValidImage(const QString &filename){
    if(filename.isEmpty()){
        return false;
    }
    QImageReader reader(filename);
    reader.setDecideFormatFromContent(true);
    return reader.canRead();
}

void LEPoint::setOptionsValues(MarkItem *item){
    ui->scaleSlider->setValue(item->scale()*1000);
    ui->rotationSlider->setValue(item->rotation());

    PointItemType::Value itemType = item->getType();

    switch(itemType){
    default:
    case PointItemType::Point1:
        ui->iconCombo->setCurrentIndex(0);
        break;
    case PointItemType::Point2:
        ui->iconCombo->setCurrentIndex(1);
        break;
    case PointItemType::CustomPoint:
        if(ui->iconCombo->itemData(2, Qt::UserRole).toString() != item->imagePath())
            applyCustomIconToCombobox(item->imagePath());
        break;
    }

}

void LEPoint::on_ok_clicked()
{
    if (fetchFailed_) {
        on_cancel_clicked();
        return;
    }
    QSettings settings("wallch", "Settings");

    // clean the array to start writing to it anew
    settings.beginWriteArray("le_mark_points", 0);
    settings.remove("");
    settings.endArray();

    settings.beginWriteArray("le_mark_points");
    int index = 0;
    Q_FOREACH(MarkItem *item, marks_){
        if(item->willFinallyBeVisible()){
            //item will be visible in the final picture, so add it to the settings!
            settings.setArrayIndex(index);
            settings.setValue("pos", QPoint(item->pos().x(), item->pos().y()));
            settings.setValue("scale", item->scale());
            settings.setValue("rotation", item->rotation());
            settings.setValue("icon", item->imagePath());
            index++;
        }
    }
    settings.endArray();
    settings.sync();

    Q_EMIT lePreferencesChanged();

    this->close();
}

void LEPoint::on_cancel_clicked()
{
    if (tryFetch_ != NULL) {
        tryFetch_->abort();
    }
    this->close();
}
