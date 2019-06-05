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

#include "about.h"
#include "ui_about.h"
#include "credits.h"
#include "license.h"
#include <iostream>

about::about(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);
    Credits = new credits( this );
    License = new license( this );
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

void about::closeEvent( QCloseEvent * )
{
    if( Credits->isVisible())
        Credits->close();
    if(License->isVisible())
        License->close();
}

void about::on_closeButton_clicked()
{
    close();
    if( Credits->isVisible())
        Credits->close();
    if(License->isVisible())
        License->close();
}

void about::on_pushButton_2_clicked()
{
    if( Credits->isVisible() )
    {Credits->raise();
    Credits->activateWindow();}
      else
      Credits->show();
}

void about::on_pushButton_3_clicked()
{
    if( License->isVisible() )
    {License->raise();
    License->activateWindow();}
      else
      License->show();
}

void about::on_label_5_linkActivated()
{
    if(system("xdg-open http://www.wallch.t35.com/"))
        std::cout << "Error while opening http://www.wallch.t35.com/\n";
}
