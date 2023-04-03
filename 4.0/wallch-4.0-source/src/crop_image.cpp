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

#include <QPainter>
#include <QScrollBar>
#include <QShortcut>

#include "crop_image.h"
#include "ui_crop_image.h"
#include "glob.h"

#define PEN_COLOR 0, 0, 0, 180
#define BRUSH_COLOR 255, 255, 255, 120

CropImage::CropImage(QImage *image, const QRect &curCropRect, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::crop_image)
{
    ui->setupUi(this);

    selectionStarted_=shiftKeyDown_=false;

    websiteImage_=image;

    imageWidth_=websiteImage_->width();
    imageHeight_=websiteImage_->height();
    createButtonsLayout();
    image_label = new CropLabel(this);
    image_label->setSelectionRect(curCropRect);
    image_label->setMinimumSize(imageWidth_, imageHeight_);
    image_label->setPixmap(QPixmap::fromImage(*websiteImage_));
    mainLayout_ = new QVBoxLayout;
    image_label->setToolTip(tr("If you want to select a large area,\njust click on the beggining of it,\nhold the Shift key and click on the end of it."));
    ui->scrollArea->setWidget(image_label);
    mainLayout_->addWidget(ui->scrollArea);
    mainLayout_->addLayout(buttonsLayout_);
    setLayout(mainLayout_);
    ui->scrollArea->setMaximumWidth(imageWidth_);
    ui->scrollArea->setMaximumHeight(imageHeight_);

    (void) new QShortcut(Qt::Key_Escape, this, SLOT(close()));
}

CropLabel::CropLabel(QWidget *parent)
    : QLabel(parent)
{
    this->setCursor(Qt::CrossCursor);
}

CropImage::~CropImage()
{
    delete websiteImage_;
    delete ui;
}

void CropImage::createButtonsLayout()
{
    buttonsLayout_ = new QHBoxLayout;
    buttonsLayout_->addStretch();
    buttonsLayout_->addWidget(ui->coordinates);
    buttonsLayout_->addWidget(ui->cancel);
    buttonsLayout_->addWidget(ui->ok);
}

void CropLabel::paintEvent(QPaintEvent *e)
{
    QLabel::paintEvent(e);
    QPainter painter(this);
    painter.setPen(QPen(QBrush(QColor(PEN_COLOR)), 4, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(BRUSH_COLOR)));
    painter.drawRect(selectionRect_);
}

void CropImage::mousePressEvent(QMouseEvent *e)
{
    selectionStarted_=true;
    QPoint point=ui->scrollArea->mapFromParent(e->pos()+QPoint(ui->scrollArea->horizontalScrollBar()->value(), ui->scrollArea->verticalScrollBar()->value()));
    if(shiftKeyDown_){
        pointX_=point.x();
        pointY_=point.y();
        image_label->selectionRect().setBottomRight(point);
        image_label->update();
        int x=image_label->selectionRect().topLeft().x();
        int y=image_label->selectionRect().topLeft().y();
        int w=pointX_-image_label->selectionRect().topLeft().x();
        int h=pointY_-image_label->selectionRect().topLeft().y();
        ui->coordinates->setText("("+QString::number(x)+", "+QString::number(y)+", "+QString::number(w)+", "+QString::number(h)+")");
    }
    else
    {
        image_label->selectionRect().setTopLeft(point);
        image_label->selectionRect().setBottomRight(point);
    }
}

void CropImage::mouseMoveEvent(QMouseEvent *e)
{
    if (selectionStarted_)
    {
        QPoint point=ui->scrollArea->mapFromParent(e->pos()+QPoint(ui->scrollArea->horizontalScrollBar()->value(), ui->scrollArea->verticalScrollBar()->value()));
        pointX_=point.x();
        pointY_=point.y();
        if(pointX_>imageWidth_ || pointY_>imageHeight_ || pointX_<0 || pointY_<0){
            return;
        }
        image_label->selectionRect().setBottomRight(point);
        repaint();
        image_label->update();

        int x=image_label->selectionRect().topLeft().x();
        int y=image_label->selectionRect().topLeft().y();
        int w=pointX_-image_label->selectionRect().topLeft().x();
        int h=pointY_-image_label->selectionRect().topLeft().y();
        ui->coordinates->setText("("+QString::number(x)+", "+QString::number(y)+", "+QString::number(w)+", "+QString::number(h)+")");
        if((y+h-ui->scrollArea->verticalScrollBar()->value())>ui->scrollArea->height()-15){
            moveScroll(1);
        }
        if(x+w-ui->scrollArea->horizontalScrollBar()->value()>ui->scrollArea->width()-15){
            moveScroll(2);
        }
        if(y+h<ui->scrollArea->verticalScrollBar()->value()+15){
            moveScroll(3);
        }
        if(x+w<ui->scrollArea->horizontalScrollBar()->value()+15){
            moveScroll(4);
        }
    }
}

void CropImage::mouseReleaseEvent()
{
    selectionStarted_=false;
}

void CropImage::moveScroll(short direction){
    /*
     * Moves the scrollbars, depending on the direction.
     * this is being done when the pointer hits to the edges of the scrollbar
     * 'direction':
     * 1 = down, 2 = right, 3 = up, 4 = left
     */
    switch(direction){
        case 1:
            ui->scrollArea->verticalScrollBar()->setValue(ui->scrollArea->verticalScrollBar()->value()+10);
            break;
        case 2:
            ui->scrollArea->horizontalScrollBar()->setValue(ui->scrollArea->horizontalScrollBar()->value()+10);
            break;
        case 3:
            ui->scrollArea->verticalScrollBar()->setValue(ui->scrollArea->verticalScrollBar()->value()-10);
            break;
        case 4:
            ui->scrollArea->horizontalScrollBar()->setValue(ui->scrollArea->horizontalScrollBar()->value()-10);
            break;
        default:
            break;
    }
}

void CropImage::on_cancel_clicked()
{
    this->close();
}

void CropImage::sendNewCoordinates(QRect coords){
    /*
     * If something is negative, make it like the rectangle started from the (last) top left edge of it
     * because QImage::copy() doesn't seem to accept negatives as width and height.
     */
    int old_width = coords.width();
    int old_height = coords.height();
    if(old_width<0){
        coords.setX(coords.x()+old_width);
        coords.setWidth(-1*old_width);
    }
    if(old_height<0){
        coords.setY(coords.y()+old_height);
        coords.setHeight(-1*old_height);
    }
    websiteImage_->copy(coords).save(gv.wallchHomePath+LW_PREVIEW_IMAGE, "PNG", 100);
    Q_EMIT coordinates(coords);
}

void CropImage::on_ok_clicked()
{
    sendNewCoordinates(image_label->selectionRect());
    this->close();
}

void CropImage::keyPressEvent(QKeyEvent *event){
    /*
     * This is a way to detect if the shift key is down when the
     * user clicks on the website image. I know that this doesn't
     * work in the (extremely) rare case that:
     * User holds shift, clicks on image, Wallch loses focus for some
     * reason, user releases the Shift key, Wallch gains focus again.
     * Now Wallch thinks that user holds the Shift key while he does not.
     * I tried to solve it with focusOutEvent() signal without success.
     */
    if(static_cast<QKeyEvent*>(event)->key() == Qt::Key_Shift){
        shiftKeyDown_=true;
    }
}

void CropImage::keyReleaseEvent(QKeyEvent *event){
    if(static_cast<QKeyEvent*>(event)->key() == Qt::Key_Shift){
        shiftKeyDown_=false;
    }
}
