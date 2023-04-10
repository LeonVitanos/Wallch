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

#ifndef CUSTOMWEBPAGE_H
#define CUSTOMWEBPAGE_H
#define QT_NO_KEYWORDS

//#include <QWebPage>

class CustomWebPage : public QObject {
protected:
    //virtual QString chooseFile(QWebFrame *, const QString&) { return QString(); }
    //virtual void javaScriptAlert (QWebFrame*, const QString &) {}
    //virtual bool javaScriptConfirm(QWebFrame *, const QString &) { return false; }
    //virtual bool javaScriptPrompt(QWebFrame *, const QString &, const QString &, QString *) { return false; }
};

#endif // CUSTOMWEBPAGE_H
