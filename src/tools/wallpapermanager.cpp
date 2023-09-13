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

#ifdef Q_OS_WIN
    #include "windows.h"
#else
    #include "desktopenvironment.h"
#endif

#include <QMessageBox>
#include <QProcess>
#include <QImageReader>
#include <QColor>
#include <QPainter>

WallpaperManager::WallpaperManager(QObject *parent):
    QObject(parent)
{}

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
    int counter = 0;
    QImageReader reader;
    reader.setDecideFormatFromContent(true);
    do
    {
        if(randomMode_){
            if(currentRandomWallpaperIndex_ >= (randomWallpapersRow_.count()-1)){
                //Start over. Generate a new random order without the first image of it to be the last image of now
                generateRandomImages(randomWallpaperIndexButNot(randomWallpapersRow_[currentRandomWallpaperIndex_]));
                currentRandomWallpaperIndex_ = 0;
            }
            else
            {
                currentRandomWallpaperIndex_++;
            }
            //go to the next random image
            nextImage = allWallpapers_.at(randomWallpapersRow_[currentRandomWallpaperIndex_]);
        }
        else
        {

            if(currentWallpaperIndex_ >= (allWallpapers_.count()-1)){
                //start over
                currentWallpaperIndex_ = 0;
            }
            else
            {
                currentWallpaperIndex_++;
            }
            nextImage = allWallpapers_.at(currentWallpaperIndex_);

        }
        counter++;
        reader.setFileName(nextImage);
    } while(!reader.canRead() && counter < allWallpapers_.count());
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
    setBackground(randomButNotCurrentWallpaper(), true, true, 1);
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

unsigned int WallpaperManager::wallpapersCount(){
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
#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        currentImage = DesktopEnvironment::gsettingsGet("org.gnome.desktop.background", DesktopEnvironment::getPictureUriName());
        if(currentImage.startsWith("file://")){
            currentImage=currentImage.right(currentImage.count()-7);
        }
    }
    else if(currentDE == DE::XFCE){
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("image-path") || entry.contains("last-image")){
                currentImage=DesktopEnvironment::runCommand("xfconf-query", false, QStringList() << entry, true).at(0);
            }
        }
    }
    else if(currentDE == DE::LXDE){
        currentImage=DesktopEnvironment::getPcManFmValue("wallpaper");
    }
#else
# ifdef Q_OS_WIN
    char *current_image = (char*) malloc(MAX_PATH*sizeof(char));
    if(current_image == NULL){
        return QString("");
    }
#  ifdef UNICODE
    bool result = (bool) SystemParametersInfoA(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID) current_image, 0);
#  else
    bool result = (bool) SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID) current_image, 0);
#  endif
    if(result){
        currentImage = QString(current_image);
    }
    free(current_image);

# endif
#endif
    return currentImage;
}

bool WallpaperManager::currentBackgroundExists()
{
    if(imageIsNull(currentBackgroundWallpaper()))
    {
        QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("This file maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
        return false;
    }
    return true;
}

void WallpaperManager::setBackground(const QString &image, bool changeAverageColor, bool showNotification, short feature){
    if(image!="" && !QFile::exists(image)){
        Global::error("Image doesn't exist, background cannot be changed!");
        return;
    }

    bool result=true;

#ifdef Q_OS_LINUX
    if(gv.rotateImages){
        Global::rotateImageBasedOnExif(image);
    }

    switch(currentDE){
    case DE::Gnome:
    case DE::Mate:{
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", DesktopEnvironment::getPictureUriName(), "file://"+image);
        if(DesktopEnvironment::gsettingsGet("org.gnome.desktop.background", "picture-options") == "none"){
            DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "picture-options", "zoom");
            Q_EMIT updateImageStyle();
        }
        break;
    }
    case DE::LXDE:
        result = DesktopEnvironment::runPcManFm(QStringList() << "-w" << image);
        break;
    case DE::XFCE:
        result = false;
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("image-path") || entry.contains("last-image")){
                result = DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << image);
                if(result)
                    break; //TODO: CHECK WITH XFCE
            }
        }
        break;
    default:
        result=false;
        break;
    }

    if (!result)
        QMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Failed to change wallpaper. If your Desktop Environment is not listed at \"Preferences->Integration->Current Desktop Environment\", then it is not supported."));
#else
    QString currentBackground = currentBackgroundWallpaper();
    bool update_image_style = currentBackground == "" || currentBackground == gv.wallchHomePath+COLOR_IMAGE;

# ifdef Q_OS_WIN
#  ifdef UNICODE
    result = (bool) SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID) image.toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#  else
    result = (bool) SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID) image.toLocal8Bit().data(), SPIF_UPDATEINIFILE);
#  endif

    if(!result)
        QMessageBox::warning(0, QObject::tr("Error"),
                             QObject::tr("Failed to change wallpaper. Maybe your Windows version is not supported? "
                                         "Usually Windows XP fails to change JPEG images."));
# else
#  ifdef Q_OS_MAC
    QProcess::startDetached("osascript", QStringList() << "-e" << "tell application \"Finder\" to set desktop picture to POSIX file \"" +  QDir::toNativeSeparators(image) + "\"" );
#  endif
# endif
    if (result && update_image_style)
        Q_EMIT updateImageStyle();

#endif

    if(result && feature!=0){
        if(gv.setAverageColor && changeAverageColor)
            Global::setAverageColor(image);

        if(gv.showNotification && showNotification)
            Global().desktopNotify(QObject::tr("Current wallpaper has been changed!"), true, image);

        if(gv.saveHistory)
            Global::saveHistory(image, feature);

        settings->setValue("images_changed", settings->value("images_changed", 0).toUInt()+1);
        settings->sync();
        gv.wallpapersChangedCurrentSession++;
    }
}

QColor WallpaperManager::getAverageColorOf(const QString &image){
    return getAverageColorOf(QImage(image));
}

QColor WallpaperManager::getAverageColorOf(const QImage &image){
    //QImageReader does not return good results
    return QColor::fromRgb(image.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::FastTransformation).pixel(0, 0));
}

short WallpaperManager::randomWallpaperIndexButNot(short indexToExclude){
    int imagesNumber = allWallpapers_.count();
    int index = 0;
    if(imagesNumber != 1)
    {
        srand(time(0));
        do
        {
            index = rand()%imagesNumber;
        } while(index == indexToExclude);
    }
    return index;
}

bool WallpaperManager::imageIsNull(const QString &filename){
    QImageReader reader(filename);
    reader.setDecideFormatFromContent(true);
    return !reader.canRead();
}

QImage WallpaperManager::indexed8ToARGB32(const QImage &image){
    QImage newImage = QImage(QSize(image.width(), image.height()), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), image);
    painter.end();
    return newImage;
}

short WallpaperManager::getCurrentFit(){
#ifdef Q_OS_LINUX
    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        QString style=DesktopEnvironment::gsettingsGet("org.gnome.desktop.background", "picture-options");
        if(style=="none")
            return 0;
        else if (style=="wallpaper")
            return 1;
        else if (style=="zoom")
            return 2;
        else if (style=="centered")
            return 3;
        else if (style=="scaled")
            return 4;
        else if (style=="stretched")
            return 5;
        else if (style=="spanned")
            return 6;
    }
    else if(currentDE == DE::XFCE){
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("image-style")){
                QString imageStyle = DesktopEnvironment::runCommand("xfconf-query", false, QStringList() << entry).at(0);
                return imageStyle.toInt();
            }
        }
    }
    else if(currentDE == DE::LXDE)
        return DesktopEnvironment::getPcManFmValue("wallpaper_mode").toInt();
#else
# ifdef Q_OS_WIN
    QSettings desktop_settings("HKEY_CURRENT_USER\\Control Panel\\Desktop", QSettings::NativeFormat);

    QString currentBackground = currentBackgroundWallpaper();
    if(currentBackground == "" || currentBackground == gv.wallchHomePath+COLOR_IMAGE)
        return 0;

    switch(desktop_settings.value("WallpaperStyle").toInt())
    {
    case 0:
        if(desktop_settings.value("TileWallpaper")!=0)
            return 1;
        else
            return 2;
    case 2:
        return 3;
    case 6:
        return 4;
    case 10:
        return 5;
    case 22:
        return 6;
    }
# endif
#endif

    return 1;
}

void WallpaperManager::setCurrentFit(short index){
#ifdef Q_OS_LINUX
    if(index == getCurrentFit())
        return;

    bool result = true;

    if(currentDE == DE::Gnome || currentDE == DE::Mate){
        QString type;
        switch(index){
        case 0:
            type="none";
            break;
        case 1:
            type="wallpaper";
            break;
        case 2:
            type="zoom";
            break;
        case 3:
            type="centered";
            break;
        case 4:
            type="scaled";
            break;
        case 5:
            type="stretched";
            break;
        case 6:
            type="spanned";
            break;
        default:
            type="wallpaper";
        }
        DesktopEnvironment::gsettingsSet("org.gnome.desktop.background", "picture-options", type);
    }
    else if(currentDE == DE::XFCE){
        result = false;
        Q_FOREACH(QString entry, DesktopEnvironment::runCommand("xfconf-query", true)){
            if(entry.contains("image-style"))
                result = DesktopEnvironment::runXfconf(QStringList() << entry << "-s" << QString::number(index));
            if(result)
                break;
        }
    }
    else if(currentDE == DE::LXDE){
        QString style;
        switch(index){
        default:
        case 0:
            style="color";
            break;
        case 1:
            style="stretch";
            break;
        case 2:
            style="fit";
            break;
        case 3:
            style="center";
            break;
        case 4:
            style="tile";
            break;
        }

        result = DesktopEnvironment::runPcManFm(QStringList() << "--wallpaper-mode=" + style);
    }
#else
# ifdef Q_OS_WIN
    QString currentBg = currentBackgroundWallpaper();

    if(index == 0){
        if(currentBg != gv.wallchHomePath+COLOR_IMAGE && !currentBg.isEmpty())
            settings->setValue("last_wallpaper", currentBg);

        if (settings->value("ShadingType", "solid") == "solid")
            setBackground("", false, false, 0);
        else
            setBackground(gv.wallchHomePath+COLOR_IMAGE, false, false, 0);
        return;
    }
    else{
        if(index == getCurrentFit())
            return;
    }

    /*
     * ---------- Reference: https://learn.microsoft.com/en-us/windows/win32/controls/themesfileformat-overview#control-paneldesktop-section ------------
     * Two registry values are set in the Control Panel\Desktop key.
     * TileWallpaper
     * 0: The wallpaper picture should not be tiled
     * 1: The wallpaper picture should be tiled
     * WallpaperStyle
     * 0:  The image is centered if TileWallpaper=0 or tiled if TileWallpaper=1
     * 2:  The image is stretched to fill the screen
     * 6:  The image is resized to fit the screen while maintaining the aspect
     *     ratio. (Windows 7 and later)
     * 10: The image is resized and cropped to fill the screen while
     *     maintaining the aspect ratio. (Windows 7 and later)
     * 22: (Not in the documentation) Span style
     * -----------------------------------------------------------------------------------------------------
     */

    /*
     * Unfortunately QSettings can't manage to change a registry DWORD,
     * so we have to go with the Windows' way.
     */

    HRESULT hr = S_OK;

    HKEY hKey = NULL;
    hr = HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_CURRENT_USER,
                                         L"Control Panel\\Desktop", 0, KEY_READ | KEY_WRITE, &hKey));
    if (SUCCEEDED(hr))
    {
        PWSTR pszWallpaperStyle;
        PWSTR pszTileWallpaper;

        wchar_t zero[10] = L"0";
        wchar_t one[10] = L"1";
        wchar_t two[10] = L"2";
        wchar_t six[10] = L"6";
        wchar_t ten[10] = L"10";
        wchar_t twentytwo[10] = L"22";

        switch (index)
        {
        case 1: //tile
            pszWallpaperStyle = zero;
            pszTileWallpaper = one;
            break;
        case 2: //center
            pszWallpaperStyle = zero;
            pszTileWallpaper = zero;
            break;
        case 3: //stretch
            pszWallpaperStyle = two;
            pszTileWallpaper = zero;
            break;
        case 4: //fit (Windows 7 and later)
            pszWallpaperStyle = six;
            pszTileWallpaper = zero;
            break;
        case 5: //fill (Windows 7 and later)
            pszWallpaperStyle = ten;
            pszTileWallpaper = zero;
            break;
        case 6: //span (Windows 8 and later)
            pszWallpaperStyle = twentytwo;
            pszTileWallpaper = zero;
            break;
        }

        //set the WallpaperStyle and TileWallpaper registry values.
        DWORD cbData = lstrlen(pszWallpaperStyle) * sizeof(*pszWallpaperStyle);
        hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"WallpaperStyle", 0, REG_SZ,
                                              reinterpret_cast<const BYTE *>(pszWallpaperStyle), cbData));
        if (SUCCEEDED(hr))
        {
            cbData = lstrlen(pszTileWallpaper) * sizeof(*pszTileWallpaper);
            hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"TileWallpaper", 0, REG_SZ,
                                                  reinterpret_cast<const BYTE *>(pszTileWallpaper), cbData));
        }

        RegCloseKey(hKey);
    }

    // Update desktop background with the style changed
    if(currentBg.isEmpty() || currentBg == gv.wallchHomePath+COLOR_IMAGE)
        setBackground(settings->value("last_wallpaper", getPreviousWallpaper()).toString(), false, false, 0);
    else
       setBackground(currentBg, false, false, 0);
# endif
#endif
}

// Functions regarding current background image
void WallpaperManager::openCurrentBackgroundImage(){
    if(!currentBackgroundExists())
        return;

    Global::openUrl("file:///"+WallpaperManager::currentBackgroundWallpaper());
}

void WallpaperManager::openCurrentBackgroundFolder(){
    if(!currentBackgroundExists())
        return;

    FileManager::openFolderOf(currentBackgroundWallpaper());
}

void WallpaperManager::copyCurrentBackgroundImage(){
    if(!currentBackgroundExists())
        return;

    Global::copyImageToClipboard(currentBackgroundWallpaper());
}

void WallpaperManager::copyCurrentBackgroundPath(){
    if(!currentBackgroundExists())
        return;

    Global::copyTextToClipboard(currentBackgroundWallpaper());
}

void WallpaperManager::deleteCurrentBackgroundImage(){
    if(!currentBackgroundExists())
        return;

    if(QMessageBox::question(0, tr("Confirm deletion"), tr("Are you sure you want to permanently delete the current image?"))==QMessageBox::Yes &&
        !QFile::remove(currentBackgroundWallpaper()))
            QMessageBox::warning(0, tr("Error"), tr("There was a problem deleting the current image. Please make sure you have the permission to delete the image or that the image exists."));
}

