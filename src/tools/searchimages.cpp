#include "searchimages.h"

SearchImages::SearchImages(QListWidget* wallpapersList, QLineEdit* searchBox, QWidget* searchWidget, QStringList* allWallpapers)
    : wallpapersList_(wallpapersList), searchBox_(searchBox), searchWidget_(searchWidget), allWallpapers_(allWallpapers)
{
    openCloseSearch_ = new QPropertyAnimation(searchWidget_, "maximumHeight");
    connect(openCloseSearch_, SIGNAL(finished()), this, SLOT(openCloseSearchAnimationFinished()));
    openCloseSearch_->setDuration(GENERAL_ANIMATION_DURATION);

    //for manually searching for files or re-selecting a picture after a folder contents have changed
    match_.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    connect(searchBox_, SIGNAL(textChanged(const QString &)), this, SLOT(searchFor(const QString &)));
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

void SearchImages::showHideSearchBoxMenu(){
    if(searchIsOn_)
        searchBox_->setFocus();
    else
        showHideSearchBox();
}

void SearchImages::on_search_up_clicked()
{
    if(searchBox_->text().isEmpty())
        return;

    searchBox_->setFocus();

    continueToPreviousMatch();
}

void SearchImages::on_search_down_clicked()
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
    if(searchBox_->hasFocus() && searchIsOn_){
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
