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

#include "screenshot.h"
#include "ui_screenshot.h"
#include "glob.h"

#include <QFileDialog>
#include <QTimer>
#include <QString>
#include <QDir>
#include <QDesktopWidget>


screenshot::screenshot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::screenshot)
{
    ui->setupUi(this);
    shootScreen();
    ui->delaySpinBox->setValue(5);
       if(_language_==2) {
       screenshot::setWindowTitle(QString::fromUtf8("Βγάλε Screenshot"));
       ui->label_2->setText(QString::fromUtf8("Επιλογές"));
       ui->label->setText(QString::fromUtf8("Καθυστέρηση Screenshot"));
       ui->hideThisWindowCheckBox->setText(QString::fromUtf8("Κρύψε αυτό το παράθυρο"));
       ui->newScreenshotButton->setText(QString::fromUtf8("Νέο Screenshot"));
       ui->saveScreenshot->setText(QString::fromUtf8("Αποθήκευση"));
       ui->QuitButton->setText(QString::fromUtf8("Έξοδος"));

       }
}

screenshot::~screenshot()
{
    delete ui;
}

void screenshot::on_newScreenshotButton_clicked()
{
    if (ui->hideThisWindowCheckBox->isChecked())
        hide();
    ui->newScreenshotButton->setDisabled(true);

    QTimer::singleShot(ui->delaySpinBox->value() * 1000, this, SLOT(shootScreen()));
}



void screenshot::on_saveScreenshot_clicked()
{
    QString format = "png";
    QString initialPath = QDir::currentPath() + tr("/untitled.") + format;
    QString fileName;
    if(_language_==2)
        fileName = QFileDialog::getSaveFileName(this, QString::fromUtf8("Αποθήκευση ως"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(format.toUpper())
                               .arg(format));
    else
        fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(format.toUpper())
                               .arg(format));
    if (!fileName.isEmpty())
        originalPixmap.save(fileName, format.toAscii());
}

void screenshot::shootScreen()
{
    if (ui->delaySpinBox->value() != 0)
        qApp->beep();
    originalPixmap = QPixmap(); // clear image for low memory situations
                                // on embedded devices.
    originalPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
    updateScreenshotLabel();

    ui->newScreenshotButton->setDisabled(false);
    if (ui->hideThisWindowCheckBox->isChecked())
        show();
}

void screenshot::updateScreenshotLabel()
{
    ui->screenshotLabel->setPixmap(originalPixmap.scaled(ui->screenshotLabel->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation));
}

void screenshot::on_QuitButton_clicked()
{
    close();
}
