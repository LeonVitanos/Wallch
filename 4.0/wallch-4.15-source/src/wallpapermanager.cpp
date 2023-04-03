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

#include "wallpapermanager.h"
#include "glob.h"

#include <QMessageBox>
#include <QProcess>

WallpaperManager::WallpaperManager()
{
}

void WallpaperManager::addToPreviousWallpapers(const QString &wallpaper){
    previousWallpapers_ << wallpaper;
    if(previousWallpapers_.count()>maxPreviousCount_){
        previousWallpapers_.removeFirst();
    }
}

QString WallpaperManager::getPreviousWallpaper(){
    if(randomMode_){
        if(previousWallpapers_.count()>=2){
            //a previous background is available!
            previousWallpapers_.removeLast();
            return previousWallpapers_.last();
        }
        else
        {
            if(currentRandomWallpaperIndex_>=1){
                //return the previous random wallpaper
                return allWallpapers_.at(randomWallpapersRow_.at(currentRandomWallpaperIndex_));
            }
            else
            {
                //the previous random wallpaper is not available. Return a truly random wallpaper.
                return randomButNotCurrentWallpaper();
            }
        }
    }
    else
    {
        //return the previous wallpaper in the row
        currentWallpaperIndex_--;
        if(currentWallpaperIndex_<0){
            //start over
            currentWallpaperIndex_=allWallpapers_.count()-1;
        }
        return allWallpapers_.at(currentWallpaperIndex_);
    }
}

QString WallpaperManager::getNextWallpaper(){
    QString nextImage;
    int counter=0;
    do
    {
        if(randomMode_){
            if(currentRandomWallpaperIndex_>=(randomWallpapersRow_.count()-1)){
                //Start over. Generate a new random order without the first image of it to be the last image of now
                generateRandomImages(randomWallpaperIndexButNot(randomWallpapersRow_[currentRandomWallpaperIndex_]));
                currentRandomWallpaperIndex_=0;
            }
            else
            {
                currentRandomWallpaperIndex_++;
            }
            //go to the next random image
            nextImage=allWallpapers_.at(randomWallpapersRow_[currentRandomWallpaperIndex_]);
        }
        else
        {

            if(currentWallpaperIndex_>=(allWallpapers_.count()-1)){
                //start over
                currentWallpaperIndex_=0;
            }
            else
            {
                currentWallpaperIndex_++;
            }
            nextImage=allWallpapers_.at(currentWallpaperIndex_);

        }
        counter++;
    }
    while(QImage(nextImage).isNull() && counter < allWallpapers_.count());
    return nextImage;
}

QString WallpaperManager::randomButNotCurrentWallpaper(){

    int imagesNumber = allWallpapers_.count();
    int index=-1;
    if(imagesNumber==1){
        index=0;
    }
    else
    {
        srand(time(0));
        QString currentImage=currentBackgroundWallpaper();
        do
        {
            index=rand()%imagesNumber;
        }
        while(allWallpapers_.at(index)==currentImage);
    }
    return allWallpapers_.at(index);
}

void WallpaperManager::convertRandomToNormal(){
    setRandomMode(false);
    if(currentRandomWallpaperIndex_>(randomWallpapersRow_.count()-1)){
        currentWallpaperIndex_=-1;
    }
    else
    {
        currentWallpaperIndex_=randomWallpapersRow_[currentRandomWallpaperIndex_];
    }
}

void WallpaperManager::startOver(){
    if(randomMode_){
        setRandomMode(true);
    }
    else
    {
        currentWallpaperIndex_=-1;
    }
}

void WallpaperManager::setRandomWallpaperAsBackground(){
    WallpaperManager::setBackground(randomButNotCurrentWallpaper(), true, true, 1);
}

QStringList WallpaperManager::getCurrentWallpapers(){
    return allWallpapers_;
}

short WallpaperManager::indexOfCurrentBackgroundWallpaper(){
    return allWallpapers_.indexOf(currentBackgroundWallpaper());
}

void WallpaperManager::setRandomMode(bool randomMode, short indexToExclude /*=-1*/, short indexToInclude /*=-1*/){
    if( (randomMode_=randomMode) == true){
        if(indexToInclude!=-1){
            //random mode with a specific image as first
            generateRandomImages(indexToInclude);
        }
        else
        {
            if(indexToExclude==-1){
                //no image specified to exclude. Exclude the image that is currently desktop background
                indexToExclude=indexOfCurrentBackgroundWallpaper();
            }
            generateRandomImages(randomWallpaperIndexButNot(indexToExclude));
        }
    }
}

void WallpaperManager::setCurrentWallpapers(const QStringList &wallpapers){
    allWallpapers_=wallpapers;
    if(randomMode_){
        generateRandomImages();
    }
}

void WallpaperManager::addWallpaper(const QString &wallpaper){
    allWallpapers_ << wallpaper;
}

void WallpaperManager::addWallpapers(const QString &wallpapers){
    allWallpapers_ << wallpapers;
}

void WallpaperManager::clearWallpapers(){
    allWallpapers_.clear();
}

short WallpaperManager::wallpapersCount(){
    return allWallpapers_.count();
}

bool WallpaperManager::randomModeEnabled(){
    return randomMode_;
}

short WallpaperManager::currentWallpaperIndex(){
    return currentWallpaperIndex_;
}

void WallpaperManager::setNextToBe(short index){
    currentWallpaperIndex_=(index-1);
}

void WallpaperManager::generateRandomImages(int firstOne /*=-1*/){
    /*
     * Generates and shuffles the list randomImagesRow_.
     * This way there wouldn't be any occurrence of the
     * same image while on random mode.
     */

    //initializing...
    currentRandomWallpaperIndex_=-1;

    randomWallpapersRow_.clear();

    int imagesNumber=allWallpapers_.count();

    for(int i=0;i<imagesNumber;i++){
        randomWallpapersRow_ << i;
    }

    srand(time(0));

    int random, temp;
    if(firstOne<0 || firstOne>=imagesNumber){
        for(int i = 0; i < imagesNumber; i++)
        {
            random = rand() % imagesNumber;
            temp = randomWallpapersRow_[i];
            randomWallpapersRow_[i] = randomWallpapersRow_[random];
            randomWallpapersRow_[random] = temp;
        }
    }
    else
    {
        if(firstOne!=0){
            temp=randomWallpapersRow_[0];
            randomWallpapersRow_[0] = firstOne;
            randomWallpapersRow_[firstOne]=temp;
        }

        //explicitly specify the first image
        for(int i = 1; i < imagesNumber; i++)
        {
            random = (rand() % (imagesNumber-1))+1;
            temp = randomWallpapersRow_[i];
            randomWallpapersRow_[i] = randomWallpapersRow_[random];
            randomWallpapersRow_[random] = temp;
        }
    }
}

QString WallpaperManager::currentBackgroundWallpaper(){
    //returns the image currently set as desktop background
    QString currentImage;
#ifdef ON_LINUX
    if(gv.currentDE==UnityGnome || gv.currentDE==Gnome){
        currentImage=Global::gsettingsGet("org.gnome.desktop.background", "picture-uri");
        if(currentImage.startsWith("file://")){
            currentImage=currentImage.right(currentImage.count()-7);
        }
    }
    else if(gv.currentDE==Mate){
        currentImage=Global::gsettingsGet("org.mate.background", "picture-filename");
    }
    else if(gv.currentDE==XFCE){
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-path") || entry.contains("last-image")){
                currentImage=Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry).replace("\n", "");
            }
        }
    }
    else if(gv.currentDE==LXDE){
        currentImage=Global::getPcManFmValue("wallpaper");
    }
#else
    char *current_image = (char*) malloc(MAX_PATH*sizeof(char));
    if(current_image==NULL){
        return QString("");
    }
#ifdef UNICODE
    bool result = (bool) SystemParametersInfoA(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID) current_image, 0);
#else
    bool result = (bool) SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID) current_image, 0);
#endif //#ifdef UNICODE
    if(result){
        currentImage = QString(current_image);
    }
    free(current_image);

#endif //#ifdef ON_LINUX
    return currentImage;
}

void WallpaperManager::openFolderOf(QString image /* = "" */){
    /*
     * Function for opening an image's path.
     * If 'img' is empty then the current image's (set as background)
     * path open, but, if img has been specified,
     * then the specified image's path opens.
     * To open the background wallpaper's path works
     * only in GNOME 3.
     * If nautilus is installed and running then the
     * image is being selected as well.
     */

    if(image.isEmpty()){
        image=currentBackgroundWallpaper();
    }

    if(image==gv.wallchHomePath+NULL_IMAGE){
        QMessageBox::information(0, QObject::tr("Hey!"), QObject::tr("You just won the game."));
    }

#ifdef ON_WIN32
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(image));
#else
    if(QFile::exists("/usr/bin/nautilus")){
        //opening the current path and selecting the image with nautilus
        QProcess::startDetached("nautilus", QStringList() << image);
    }
    else
    {
        //opening the current path with the default files viewer.
        Global::openUrl("file://"+Global::dirnameOf(image));
    }
#endif //#ifdef ON_WIN32
}

void WallpaperManager::setBackground(const QString &image, bool changeAverageColor, bool showNotification, short feature){
    if(!QFile::exists(image)){
        Global::error("Image doesn't exist, background cannot be changed!");
        return;
    }

    bool result=true;

#ifdef ON_LINUX
    if(gv.rotateImages){
        Global::rotateImageBasedOnExif(image);
    }

    switch(gv.currentDE){
    case Gnome:
    case UnityGnome:
        Global::gsettingsSet("org.gnome.desktop.background", "picture-uri", "file://"+image);
        break;
    case Mate:
        Global::gsettingsSet("org.mate.background", "picture-filename", image);
        break;
    case LXDE:
        QProcess::startDetached("pcmanfm", QStringList() << "-w" << image);
        break;
    case XFCE:
        Q_FOREACH(QString entry, Global::getOutputOfCommand("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-l").split("\n")){
            if(entry.contains("image-path") || entry.contains("last-image")){
                QProcess::startDetached("xfconf-query", QStringList() << "-c" << "xfce4-desktop" << "-p" << entry << "-s" << image);
            }
        }
        break;
    default:
        QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Failed to change wallpaper. If your Desktop Environment is not listed at \"Preferences->Integration-> Current Desktop Environment\", then it is not supported."));
        result=false;
        break;
    }

#else

#ifdef UNICODE
    result = (bool) SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID) image.toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#else
    result = (bool) SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID) image.toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#endif //#ifdef UNICODE
    if(!result){
        QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Failed to change wallpaper. Maybe your Windows version is not supported? Usually Windows XP fails to change JPEG images."));
    }

#endif //#ifdef ON_LINUX

    if(result && feature!=0){
        if(gv.setAverageColor && changeAverageColor){
            Global::setAverageColor(image);
        }

        if(gv.showNotification && showNotification){
            Global(false).desktopNotify(QObject::tr("Current wallpaper has been changed!"), true, image);
        }

        if(gv.saveHistory){
            Global::saveHistory(image, feature);
        }
        settings->setValue("images_changed", settings->value("images_changed", 0).toUInt()+1);
        settings->sync();
        gv.wallpapersChangedCurrentSession++;
    }
}

short WallpaperManager::randomWallpaperIndexButNot(short indexToExclude){
    int imagesNumber = allWallpapers_.count();
    int index=0;
    if(imagesNumber!=1)
    {
        srand(time(0));
        do
        {
            index=rand()%imagesNumber;
        }
        while(index==indexToExclude);
    }
    return index;
}
