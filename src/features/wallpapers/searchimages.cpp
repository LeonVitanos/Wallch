#include "searchimages.h"

SearchImages::SearchImages(QListWidget* wallpapersList, QLineEdit* searchBox, QWidget* searchWidget,
                           QPushButton* searchUp, QPushButton* searchDown, QPushButton* searchClose,
                           QStringList* allWallpapers, QObject *parent)
    : QObject(parent),
      wallpapersList_(wallpapersList), searchBox_(searchBox), searchWidget_(searchWidget), allWallpapers_(allWallpapers)
{
    openCloseSearch_ = new QPropertyAnimation(searchWidget_, "maximumHeight", this);
    connect(openCloseSearch_, SIGNAL(finished()), this, SLOT(openCloseSearchAnimationFinished()));
    openCloseSearch_->setDuration(GENERAL_ANIMATION_DURATION);

    //for manually searching for files or re-selecting a picture after a folder contents have changed
    match_.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    connect(searchBox_, &QLineEdit::textChanged, this, &SearchImages::searchFor);
    connect(searchBox_, &QLineEdit::returnPressed, this, &SearchImages::enterPressed);

    searchClose->setIcon(QIcon::fromTheme("window-close", QIcon(":/images/window-close.png")));
    searchDown->setIcon(QIcon::fromTheme("go-down", QIcon(":/images/go-down.png")));
    searchUp->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/go-up.png")));

    connect(searchUp, &QPushButton::clicked, this, &SearchImages::handleSearchUpClick);
    connect(searchDown, &QPushButton::clicked, this, &SearchImages::handleSearchDownClick);
    connect(searchClose, &QPushButton::clicked, this, &SearchImages::hideSearch);

    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), wallpapersList_);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (wallpapersList_->isVisible())
            showHideSearchBox();
    });
}

void SearchImages::searchFor(const QString &term){
    if (term.isEmpty() || !searchIsOn_)
        return;

    wallpapersList_->clearSelection();

    // create list of wallpapers basenames
    if(allWallpapers_->count() != allWallpapersBasename_.count()){
        allWallpapersBasename_.clear();
        Q_FOREACH(QString wallpaper, *allWallpapers_)
            allWallpapersBasename_ << QFileInfo(wallpaper).baseName();
    }

    matchIndex_ = 0;
    QRegularExpression regex(term, QRegularExpression::CaseInsensitiveOption);

    // Using filter to find matching strings
    filteredList_ = allWallpapersBasename_.filter(regex);
    if (!filteredList_.isEmpty()) {
        selectItem();
        doesMatch();
    } else
        doesntMatch();
}

void SearchImages::continueToPreviousMatch()
{
    wallpapersList_->clearSelection();  // Clear the selection in the list

    if(filteredList_.count()==0){
        return;
    } else if (matchIndex_ == 0){
        matchIndex_ = filteredList_.count() - 1;
    } else {
        matchIndex_--;
    }
    selectItem();
}

void SearchImages::continueToNextMatch(){
    wallpapersList_->clearSelection();

    if(matchIndex_ == filteredList_.count() - 1){
        matchIndex_ = 0;
    } else {
        matchIndex_++;
    }
    selectItem();
}

void SearchImages::selectItem(){
    if(matchIndex_ > filteredList_.count() - 1 )
        return;

    int index = allWallpapersBasename_.indexOf(filteredList_[matchIndex_]);

    wallpapersList_->scrollToItem(wallpapersList_->item(index));
    wallpapersList_->setCurrentItem(wallpapersList_->item(index));
    wallpapersList_->item(index)->setSelected(true);
}

void SearchImages::doesntMatch(){
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::red);
    searchBox_->setPalette(pal);
}

void SearchImages::doesMatch(){
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::green);
    searchBox_->setPalette(pal);
}

void SearchImages::showHideSearchBox() {
    openCloseSearch_->setStartValue(searchWidget_->maximumHeight());

    if (!searchIsOn_)
    {
        searchWidget_->show();
        openCloseSearch_->setEndValue(30);
        searchBox_->setFocus();
        searchIsOn_ = true;
        searchFor(searchBox_->text()); //in case a previous search exists
    }
    else
    {
        //search to be closed
        searchList_.clear();
        openCloseSearch_->setEndValue(0);

        //more icons may become available once the search box closes
        Q_EMIT launchTimerToUpdateIcons();
        searchIsOn_ = false;
    }
    openCloseSearch_->start();
}

void SearchImages::handleSearchUpClick()
{
    if(searchBox_->text().isEmpty())
        return;

    searchBox_->setFocus();

    continueToPreviousMatch();
}

void SearchImages::handleSearchDownClick()
{
    searchBox_->setFocus();
    enterPressed();
}

void SearchImages::openCloseSearchAnimationFinished()
{
    if(openCloseSearch_->endValue()==0)
        searchWidget_->hide();
}

bool SearchImages::hideSearch()
{
    if(searchIsOn_){
        showHideSearchBox();
        return true;
    }
    else
        return false;
}

void SearchImages::enterPressed(){
    if(!searchBox_->text().isEmpty())
        continueToNextMatch();
}

void SearchImages::clearSearchBox(){
    hideSearch();
    searchBox_->clear();
}

void SearchImages::saveSelection()
{
    if (wallpapersList_->selectedItems().isEmpty()) {
        savedSelection_.clear();
        return;
    }

    QListWidgetItem *item = wallpapersList_->selectedItems().first();
    if (wallpapersList_->viewMode() == QListView::IconMode) {
        savedSelection_ = item->statusTip().isEmpty() ? item->toolTip() : item->statusTip();
    } else {
        savedSelection_ = item->text();
    }
}

void SearchImages::restoreSelection()
{
    if (savedSelection_.isEmpty())
        return;

    for (int i = 0; i < wallpapersList_->count(); ++i) {
        QListWidgetItem *item = wallpapersList_->item(i);
        QString itemPath = (wallpapersList_->viewMode() == QListView::IconMode) ? (item->statusTip().isEmpty() ? item->toolTip() : item->statusTip()) : item->text();

        if (itemPath == savedSelection_) {
            wallpapersList_->clearSelection();
            wallpapersList_->setCurrentItem(item);
            item->setSelected(true);
            wallpapersList_->scrollToItem(item);

            if (searchIsOn_ && !searchBox_->text().isEmpty()) {
                QRegularExpression regex(searchBox_->text(), QRegularExpression::CaseInsensitiveOption);
                // If the restored item is part of the current search, update matchIndex_
                if (i < allWallpapersBasename_.count() && allWallpapersBasename_.at(i).contains(regex)) {
                    int newMatchIndex = 0;
                    for (int j = 0; j < i; ++j) {
                        if (allWallpapersBasename_.at(j).contains(regex)) {
                            newMatchIndex++;
                        }
                    }
                    matchIndex_ = newMatchIndex;
                }
            }
            break;
        }
    }
}

void SearchImages::updateSearch()
{
    allWallpapersBasename_.clear();
    if (searchIsOn_) {
        searchFor(searchBox_->text());
    }
}
