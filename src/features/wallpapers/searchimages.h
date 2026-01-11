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

#ifndef SEARCHIMAGES_H
#define SEARCHIMAGES_H

#include <QPropertyAnimation>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QString>
#include <QShortcut>

class SearchImages : public QObject
{
    Q_OBJECT

public:
    SearchImages(QListWidget* wallpapersList, QLineEdit* searchBox, QWidget* searchWidget,
                 QPushButton* searchUp, QPushButton* searchDown, QPushButton* searchClose,
                 QStringList* allWallpapers, QObject *parent = nullptr);

    void continueToNextMatch();
    void continueToPreviousMatch();
    void doesMatch();
    void doesntMatch();
    void clearSearchBox();
    int matchIndex_;
    QStringList filteredList_;
    void selectItem();
    void saveSelection();
    void restoreSelection();
    void updateSearch();

public Q_SLOTS:
    void showHideSearchBox();
    void searchFor(const QString &term);
    void handleSearchDownClick();
    void handleSearchUpClick();
    bool hideSearch();

private Q_SLOTS:
    void openCloseSearchAnimationFinished();
    void enterPressed();

private:
    QListWidget* wallpapersList_;
    QLineEdit* searchBox_;
    QWidget* searchWidget_;
    int currentSearchItemIndex_;
    QStringList searchList_;
    QRegularExpression match_;
    QPropertyAnimation *openCloseSearch_;

    bool searchIsOn_ = false;
    QStringList* allWallpapers_;
    QStringList allWallpapersBasename_;
    QString savedSelection_;

Q_SIGNALS:
    void launchTimerToUpdateIcons();
};

#endif // SEARCHIMAGES_H
