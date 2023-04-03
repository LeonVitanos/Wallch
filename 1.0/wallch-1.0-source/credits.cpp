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

#include "credits.h"
#include "ui_credits.h"

credits::credits(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::credits)
{
    ui->setupUi(this);
}

credits::~credits()
{
    delete ui;
}

void credits::changeEvent(QEvent *e)
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

void credits::on_pushButton_clicked()
{
    close();
}
