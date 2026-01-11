/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef CUSTOMWEBPAGE_H
#define CUSTOMWEBPAGE_H

//#include <QWebPage>

class CustomWebPage : public QObject {
protected:
    //virtual QString chooseFile(QWebFrame *, const QString&) { return QString(); }
    //virtual void javaScriptAlert (QWebFrame*, const QString &) {}
    //virtual bool javaScriptConfirm(QWebFrame *, const QString &) { return false; }
    //virtual bool javaScriptPrompt(QWebFrame *, const QString &, const QString &, QString *) { return false; }
};

#endif // CUSTOMWEBPAGE_H
