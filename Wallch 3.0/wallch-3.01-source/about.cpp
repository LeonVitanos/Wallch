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

#include "about.h"
#include "ui_about.h"
#include "iostream"
#include <QDebug>

about::about(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);

    QString start_string="<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Sans'; font-size:10pt;\">";
    QString middle_string1=" &lt;</span><a href=\"mailto:";
    QString middle_string2="\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">";
    QString end_string="</span></a><span style=\" font-family:'Sans'; font-size:10pt;\">&gt;</span></p>";

    ui->label_5->setText(QString("<a href=\"wall-changer.sourceforge.net"+middle_string2+"%1</span></a>").arg(tr("Website")));
    ui->label_4->setText(QString::fromUtf8("© 2010-2011 ") + tr("The Wallch (Wallpaper Changer) authors"));

    //CODED BY
    ui->textBrowser->setText(QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1%2").arg(tr("In random order")).arg(":</p>"\

    +start_string+"Leon Vitanos"+middle_string1+"leon.check.me@gmail.com"+middle_string2+"leon.check.me@gmail.com"+end_string+

    start_string+"Alex Solanos"+middle_string1+"alexsol.developer@gmail.com"+middle_string2+"alexsol.developer@gmail.com"+end_string+

    "</body></html>"));

    //TRANSLATED BY
    ui->textBrowser_2->setText(QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1%2").arg(tr("In random order")).arg(":</p>"\

    +start_string+"Leon Vitanos"+middle_string1+"leon.check.me@gmail.com"+middle_string2+"leon.check.me@gmail.com"+end_string+

    start_string+"Alex Solanos"+middle_string1+"alexsol.developer@gmail.com"+middle_string2+"alexsol.developer@gmail.com"+end_string+

    start_string+"Martin Bra"+middle_string1+"damahevi@yahoo.com"+middle_string2+"damahevi@yahoo.com"+end_string+

    start_string+"Mohamed Mostafa"+middle_string1+"mohamed.am.mostafa@hotmail.com"+middle_string2+"mohamed.am.mostafa@hotmail.com"+end_string+

    start_string+"Christer Landstedt"+middle_string1+"christer.landstedt@gmail.com"+middle_string2+"christer.landstedt@gmail.com"+end_string+

    start_string+"Esed Alihodzic"+middle_string1+"th3.fault@gmail.com"+middle_string2+"th3.fault@gmail.com"+end_string+

    start_string+"Theis F. Hinz"+middle_string1+"theis@shafh.dk"+middle_string2+"theis@shafh.dk"+end_string+

    start_string+"Mathijs Groothuis"+middle_string1+"mathijs.groothuis@gmail.com"+middle_string2+"mathijs.groothuis@gmail.com"+end_string+

    start_string+"Eyal Seelig"+middle_string1+"eyal.seelig@gmail.com"+middle_string2+"eyal.seelig@gmail.com"+end_string+

    start_string+""+QString::fromUtf8("Marián Polťák")+""+middle_string1+"marianpoltak@centrum.sk"+middle_string2+"marianpoltak@centrum.sk"+end_string+

    start_string+"Evandro Pires Alves"+middle_string1+"evandro.pa@gmail.com"+middle_string2+"evandro.pa@gmail.com"+end_string+

    start_string+"Njord Iversen"+middle_string1+"njordiversen@gmail.com"+middle_string2+"njordiversen@gmail.com"+end_string+

    start_string+"Michal Newiak"+middle_string1+"manveru1986@gmail.com"+middle_string2+"manveru1986@gmail.com"+end_string+

    start_string+"Camilo Romero Perilla"+middle_string1+"kmorope@gmail.com"+middle_string2+"kmorope@gmail.com"+end_string+

    start_string+"Fabio Falcone"+middle_string1+"itwfabio@gmail.com"+middle_string2+"itwfabio@gmail.com"+end_string+

    start_string+""+QString::fromUtf8("Евгений Буря")+""+middle_string1+"burya.e@gmail.comm"+middle_string2+"burya.e@gmail.com"+end_string+

    start_string+"Cenk Soykan"+middle_string1+"soykan.cenk@gmail.com"+middle_string2+"soykan.cenk@gmail.com"+end_string+

    start_string+"Sylvester Michael Johnson"+middle_string1+"xskydevilx@mac.hush.com"+middle_string2+"xskydevilx@mac.hush.com"+end_string+

    start_string+"Theil Sebastien"+middle_string1+"seb.theil@gmail.com"+middle_string2+"seb.theil@gmail.com"+end_string+

    "</body></html>"));
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
    if(system("xdg-open http://wall-changer.sourceforge.net"))
        std::cerr << "Error while opening http://wall-changer.sourceforge.net\n";
}
