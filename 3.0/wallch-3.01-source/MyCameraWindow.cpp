/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vitanos

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

#include "MyCameraWindow.h"
#include "QPushButton"

#include <QLineEdit>
#include <QFileDialog>
#include <QDir>
#include <QMetaObject>

#include <opencv/highgui.h>
#include <iostream>
#include "glob.h"

using namespace std;

CvCapture * camera;
IplImage *frame;

bool update_active=false;
bool first_time=true;
bool black=true;

int counter;

void MyCameraWindow::close_now(){
    if(camera!=NULL)
        cvReleaseCapture(&camera);
    if(update_active){
        update_image->stop();
        update_active=false;
    }
    camera_open=0;
    close();
}

void MyCameraWindow::closeEvent( QCloseEvent * )
{
    close_now();
}

MyCameraWindow::MyCameraWindow(QWidget *parent) : QWidget(parent) {
    camera=cvCreateCameraCapture(0);
    if(camera==NULL){
        camera_open=0;
        cerr << "Camera is NULL! This means that you either don't have camera or it is busy by other application.\n";
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        camera_success=0;
        return;
    }
    frame=cvQueryFrame(camera);
    if(frame==NULL){
        camera_open=0;
        cerr << "Image is NULL! This means that you either don't have camera or it is busy by other application.\n";
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        camera_success=0;
        return;
    }

    camera_success=1;
    capturebtn = new QPushButton;
    closebtn = new QPushButton;
    newbtn = new QPushButton;
    savebtn = new QPushButton;
    blink = new QLabel;

    savebtn->setText(tr("Save as"));
    newbtn->setText(tr("New Image"));
    capturebtn->setText(tr("Take Picture"));
    closebtn->setText(tr("Close"));
    capturebtn->setText(tr("Capture"));
    update_image = new QTimer(this);
    countdown = new QTimer(this);
    color_changer = new QTimer(this);
    connect(newbtn, SIGNAL(clicked()), this, SLOT(new_image()));
    connect(capturebtn, SIGNAL(clicked()), this, SLOT(take_image()));
    connect(closebtn, SIGNAL(clicked()), this, SLOT(close_now()));
    connect(savebtn,SIGNAL(clicked()),this,SLOT(save_image()));
    connect(update_image, SIGNAL(timeout()), this, SLOT(image_update()));
    connect(countdown, SIGNAL(timeout()), this, SLOT(update_text()));
    connect(color_changer, SIGNAL(timeout()), this, SLOT(update_color()));
    cvwidget = new QOpenCVWidget(this);
    layout = new QVBoxLayout;
    layout->addWidget(cvwidget);
    layout->addWidget(capturebtn);
    layout->addWidget(closebtn);
    layout->addWidget(blink);
    setLayout(layout);
    resize(500, 400);
    cvwidget->putImage(frame);
    first_time=true;
    update_image->start(1);
    update_active=true;
 }

void MyCameraWindow::image_update(){
    frame=cvQueryFrame(camera);
    cvwidget->putImage(frame);
    if(first_time){
        first_time=false;
        this->setMaximumWidth(this->width());
        this->setMinimumWidth(this->width());
        this->setMaximumHeight(this->height());
        this->setMinimumHeight(this->height());
    }
}

void MyCameraWindow::new_image(){
    if(!update_active){
        update_image->start();
        update_active=true;
    }
    layout->removeWidget(savebtn);
    layout->removeWidget(newbtn);
    layout->removeWidget(closebtn);
    savebtn->hide();
    newbtn->hide();
    layout->addWidget(capturebtn);
    layout->addWidget(closebtn);
    capturebtn->show();
    closebtn->show();

}
void MyCameraWindow::save_image(){

    QString filename;
    QString format = "jpg";
    QString save;
    save = tr("Save as");
    filename = QFileDialog::getSaveFileName(this, save, QDir::homePath()+"/untiltled."+format,tr("%1 Files (*.%2);;All Files (*)").arg(format.toUpper()).arg(format));
    if(!filename.isEmpty()){
        cvSaveImage(filename.toLocal8Bit().data() ,frame);
    }

}

void MyCameraWindow::update_text(){
    if(--counter==0){
        blink->hide();
        color_changer->stop();
        countdown->stop();
        if(update_active){
            update_image->stop();
            update_active=false;
        }
        capturebtn->hide();
        layout->addWidget(savebtn);
        layout->addWidget(newbtn);
        layout->addWidget(closebtn);
        savebtn->show();
        newbtn->show();
        closebtn->show();
    }
}

void MyCameraWindow::update_color(){
    if(black){
        black=false;
        blink->setText("<center><font size=100 color=#000000>"+QString::number(counter)+"</font></center>");
    }
    else
    {
        black=true;
        blink->setText("<center><font size=100 color=#FF0000>"+QString::number(counter)+"</font></center>");
    }
}

void MyCameraWindow::take_image(){
    counter=3;
    layout->removeWidget(closebtn);
    layout->removeWidget(capturebtn);
    closebtn->hide();
    capturebtn->hide();

    layout->addWidget(blink);
    blink->show();
    blink->setText("<center><font size=100 color=#FF0000>"+QString::number(counter)+"</font></center>");
    countdown->start(1000);
    color_changer->start(200);
}
