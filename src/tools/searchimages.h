#ifndef SEARCHIMAGES_H
#define SEARCHIMAGES_H

#include "qpropertyanimation.h"
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QString>
#include <QShortcut>
#include <glob.h>

class SearchImages : public QObject
{
    Q_OBJECT

public:
    SearchImages(QListWidget* wallpapersList, QLineEdit* searchBox, QWidget* searchWidget,
                 QPushButton* searchUp, QPushButton* searchDown, QPushButton* searchClose,
                 QStringList* allWallpapers);

    void continueToNextMatch();
    void continueToPreviousMatch();
    void doesMatch();
    void doesntMatch();
    void clearSearchBox();
    int matchIndex_;
    QStringList filteredList_;
    void selectItem();

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

Q_SIGNALS:
    void launchTimerToUpdateIcons();
};

#endif // SEARCHIMAGES_H
