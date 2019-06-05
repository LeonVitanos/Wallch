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

#include "properties.h"
#include "ui_properties.h"
#include "glob.h"

#include <QDir>
#include <QImage>
#include <QFileInfo>

using namespace std;

QString image;

properties::properties(QString& img, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::properties)
{
    ui->setupUi(this);
    createOptionsGroupBox();
    createButtonsLayout();
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(ui->label);
    mainLayout->addWidget(optionsGroupBox);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
    image=img;
    QImage qimg(image);

    ui->image_height->setText(QString::number(qimg.height()) + " px");
    ui->image_width->setText(QString::number(qimg.width()) + " px");
    QFileInfo qfile(image);
    float size=float(qfile.size())/1048576;
    float rounded_size=((int)(size*100+0.5)/100.0); //we won't have negative numbers so I can use this B)
    ui->image_size->setText(QString::number(rounded_size) + " MB");
    ui->image_name->setText(qfile.baseName());
    ui->image_location->setText(qfile.absoluteDir().path());

    string image_type = "";
    QString qcommand1="file -b \""+image+"\"";
    char buffer[128];
    FILE *pipe = popen(qcommand1.toLocal8Bit().data(), "r");
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            image_type += buffer;
    }
    pclose(pipe);
    QString qimage_type = QString::fromStdString(image_type).replace(QString("\n"),QString(""));
    ui->image_type->setText(qimage_type);
    ui->image_type->setCursorPosition(0);
    ui->image_location->setCursorPosition(0);
    originalPixmap = QPixmap();
    originalPixmap = QPixmap().fromImage(qimg);
    updateScreenshotLabel();
}

void properties::closeEvent( QCloseEvent * )
{
    prop=1;
}

properties::~properties()
{
    delete ui;
}

void properties::on_pushButton_clicked()
{
    close();
}

void properties::resizeEvent(QResizeEvent * /* event */)
{
    QSize scaledSize = originalPixmap.size();
    scaledSize.scale(ui->label->size(), Qt::KeepAspectRatio);
    if (!ui->label->pixmap() || scaledSize != ui->label->pixmap()->size())
        updateScreenshotLabel();
}

void properties::createOptionsGroupBox()
{
    optionsGroupBox = new QGroupBox();
    optionsGroupBoxLayout = new QGridLayout;
    optionsGroupBoxLayout->addWidget(ui->label_2, 0, 0);
    optionsGroupBoxLayout->addWidget(ui->image_name, 0, 1);
    optionsGroupBoxLayout->addWidget(ui->label_4, 1, 0);
    optionsGroupBoxLayout->addWidget(ui->image_size, 1, 1);
    optionsGroupBoxLayout->addWidget(ui->label_7, 2, 0);
    optionsGroupBoxLayout->addWidget(ui->image_width, 2, 1);
    optionsGroupBoxLayout->addWidget(ui->label_6, 3, 0);
    optionsGroupBoxLayout->addWidget(ui->image_height, 3, 1);
    optionsGroupBoxLayout->addWidget(ui->label_3, 4, 0);
    optionsGroupBoxLayout->addWidget(ui->image_type, 4, 1);
    optionsGroupBoxLayout->addWidget(ui->label_5, 5, 0);
    optionsGroupBoxLayout->addWidget(ui->image_location, 5, 1);
    optionsGroupBox->setLayout(optionsGroupBoxLayout);
}

void properties::createButtonsLayout()
{
    buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(ui->pushButton);
}

void properties::updateScreenshotLabel()
{
    ui->label->setPixmap(originalPixmap.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
