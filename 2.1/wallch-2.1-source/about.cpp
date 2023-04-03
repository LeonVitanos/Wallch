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

#include "about.h"
#include "ui_about.h"
#include "iostream"

about::about(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);
    ui->label_5->setText(QString("<a href=\"http://www.wallch.t35.com/\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">%1</span></a>").arg(tr("Website")));
    ui->textBrowser->setText(QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1%2").arg(tr("In random order")).arg(":</p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Leon Vytanos &lt;</span><a href=\"mailto:leon.check.me@gmail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">leon.check.me@gmail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Alex Solanos &lt;</span><a href=\"mailto:alexsol.developer@gmail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">alexsol.developer@gmail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p></body></html>"));
    ui->textBrowser_2->setText(QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1%2").arg(tr("In random order")).arg(":</p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Leon Vytanos &lt;</span><a href=\"mailto:leon.check.me@gmail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">leon.check.me@gmail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Alex Solanos &lt;</span><a href=\"mailto:alexsol.developer@gmail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">alexsol.developer@gmail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Raphael Isemann &lt;</span><a href=\"mailto:teemperor@googlemail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">teemperor@googlemail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">Soroush Rabiel &lt;</span><a href=\"mailto:soroush.rabiei@gmail.com\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">soroush.rabiei@gmail.com</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p></body></html>"));
    ui->label_4->setText(QString::fromUtf8("© 2010-2011 ") + tr("The Wallch (Wallpaper Changer) authors"));
}

about::~about()
{
    delete ui;
}

void about::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void about::on_closeButton_clicked()
{
    close();
}

void about::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void about::on_pushButton_2_clicked()
{
     ui->stackedWidget->setCurrentIndex(1);
}

void about::on_pushButton_3_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void about::on_label_5_linkActivated()
{
    if(system("xdg-open http://www.wallch.t35.com/"))
        std::cout << "Error while opening http://www.wallch.t35.com/\n";
}
