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

#include "screenshot.h"
#include "ui_screenshot.h"
#include "glob.h"

#include <QFileDialog>
#include <QTimer>
#include <QDir>
#include <QDesktopWidget>


screenshot::screenshot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::screenshot)
{
    ui->setupUi(this);
    shootScreen();
    ui->delaySpinBox->setValue(5);
    createOptionsGroupBox();
    createButtonsLayout();
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(ui->screenshotLabel);
    mainLayout->addWidget(optionsGroupBox);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

screenshot::~screenshot()
{
    delete ui;
}

void screenshot::on_newScreenshotButton_clicked()
{
    if (ui->hideThisWindowCheckBox->isChecked())
        hide();
    if (ui->hideMainWindow->isChecked())
        Q_EMIT hide_it();

    ui->newScreenshotButton->setDisabled(true);

    QTimer::singleShot(ui->delaySpinBox->value() * 1000, this, SLOT(shootScreen()));
}


void screenshot::on_saveScreenshot_clicked()
{
    QString format = "png";
    QString initialPath = QDir::currentPath() + "/untitled." + format;
    QString fileName;
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
    if (ui->hideMainWindow->isChecked())
        Q_EMIT show_it();
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

void screenshot::resizeEvent(QResizeEvent * /* event */)
{
    QSize scaledSize = originalPixmap.size();
    scaledSize.scale(ui->screenshotLabel->size(), Qt::KeepAspectRatio);
    if (!ui->screenshotLabel->pixmap()
            || scaledSize != ui->screenshotLabel->pixmap()->size())
        updateScreenshotLabel();
}

void screenshot::createOptionsGroupBox()
{
    optionsGroupBox = new QGroupBox(tr("Options"));

    connect(ui->delaySpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateCheckBox()));

    optionsGroupBoxLayout = new QGridLayout;
    optionsGroupBoxLayout->addWidget(ui->label, 0, 0);
    optionsGroupBoxLayout->addWidget(ui->delaySpinBox, 0, 1);
    optionsGroupBoxLayout->addWidget(ui->hideThisWindowCheckBox, 1, 0, 1, 2);
    optionsGroupBoxLayout->addWidget(ui->hideMainWindow, 10, 0, 1, 2);
    optionsGroupBox->setLayout(optionsGroupBoxLayout);
}

void screenshot::createButtonsLayout()
{
    buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(ui->newScreenshotButton);
    buttonsLayout->addWidget(ui->saveScreenshot);
    buttonsLayout->addWidget(ui->QuitButton);
}

void screenshot::updateCheckBox()
{
    if (ui->delaySpinBox->value() == 0) {
        ui->hideThisWindowCheckBox->setDisabled(true);
        ui->hideThisWindowCheckBox->setChecked(false);
        ui->hideMainWindow->setDisabled(true);
        ui->hideMainWindow->setChecked(false);
    }
    else {
       ui-> hideThisWindowCheckBox->setDisabled(false);
       ui->hideMainWindow->setDisabled(false);
    }
}
