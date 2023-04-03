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

#include "about.h"
#include "ui_about.h"
#include "glob.h"

#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>

#define PERSON(name, email) name " &lt;<a href=\"mailto:" email "\">" email "</a>&gt;"
#define TRANSLATION(translation) translation " Translation:"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);

    easterEggCounter_=0;

    ui->wallch_version_label->setText("Wallch " + QString::number(APP_VERSION, 'f', 3));

    ui->website_label->setText(QString("<a href=\"http://melloristudio.com\"><span style=\"text-decoration: underline; color: #ff5500;\">%1</span></a>").arg(tr("Website")));
    ui->label_4->setText(QString::fromUtf8("© 2010-2014 ") + tr("The Wallch (Wallpaper Changer) authors"));

    ui->about_button->setChecked(true);

    QFile file(":/themes/default.css");
    if(!file.open(QIODevice::ReadOnly)){
        Global::error("Could not load the theme file.");
        return;
    }

    QString css=file.readAll();

    file.close();
    ui->writtenBy->document()->setDefaultStyleSheet(css);
    ui->translatedBy->document()->setDefaultStyleSheet(css);
    ui->artworkBy->document()->setDefaultStyleSheet(css);

    ui->writtenBy->append(tr("In random order:"));
    ui->writtenBy->append(PERSON("Leon Vitanos", "leon.vitanos@gmail.com"));
    ui->writtenBy->append(PERSON("Alex Solanos", "alexsol.developer@gmail.com"));

    ui->translatedBy->append(TRANSLATION("Greek"));
    ui->translatedBy->append(PERSON("Leon Vitanos", "leon.vitanos@gmail.com"));
    ui->translatedBy->append(PERSON("Alex Solanos", "alexsol.developer@gmail.com"));

    ui->artworkBy->append(tr("The logo of")+" Mellori Studio:");
    ui->artworkBy->append(QString("Kostis Solanos &lt;<a href=\"http://kozysolanos.tumblr.com\">kozysolanos.tumblr.com</a>, ")
                          +"<a href=\"http://solanos.deviantart.com\">solanos.deviantart.com</a>&gt;");
    ui->artworkBy->append(tr("\nThe screen used for the preview:"));
    ui->artworkBy->append("Vlad Gerasimov &lt;<a href=\"http://www.vladstudio.com\">vladstudio.com</a>&gt;");

    ui->stackedWidget->setCurrentIndex(0);
}

About::~About()
{
    delete ui;
}

void About::changeEvent(QEvent *e)
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

void About::on_closeButton_clicked()
{
    close();
}

void About::on_about_button_clicked()
{
    easterEggCounter_=0;
    ui->about_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
}

void About::on_credits_button_clicked()
{
    if((++easterEggCounter_)==5){
        QMessageBox::information(this, "Wallch", tr("Don't click it again."));
    }
    else if(easterEggCounter_>5){
        easterEggCounter_=0;
        Global::openUrl("http://melloristudio.com/wallch/easterEggRedirect");
    }
    ui->credits_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void About::on_license_button_clicked()
{
    easterEggCounter_=0;
    ui->license_button->setChecked(true);
    ui->stackedWidget->setCurrentIndex(2);
}

void About::on_website_label_linkActivated()
{
    Global::openUrl("http://melloristudio.com");
}
