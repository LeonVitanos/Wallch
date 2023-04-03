#include "cachemanager.h"
#include "glob.h"

#include <QDir>
#include <QCryptographicHash>
#include <QImageReader>

CacheManager::CacheManager(QObject *parent) :
    QObject(parent)
{
}

void CacheManager::fixCacheSizeWithCurrentFoldersBeing(const QStringList &wallpaperFolders){
    /*
     * Fixes the cache size depending on the current folders (the deletion begins from the invalid
     * cached images (non-existent original images) to the ones not being in the current wallpaper
     * folders to the ones being in the current wallpaper folders)
    */
    if(maxCacheSize_ == -1){
        return; // unlimited
    }

    updateCacheStatus();
    if(cacheEntry_.size <= maxCacheSize_){
        return;
    }

    //step 1: Remove all cached images that refer to non-existent images and all cached images that refer to images that have changed (same filename, different image)
    for(unsigned int i=0; i < cacheEntry_.imageCount; i++){
        QString cachedImage = cacheEntry_.images.at(i);
        QString originalImage = cacheToOriginalName(cachedImage);
        if(!QFile::exists(originalImage) || md5Of(originalImage) != extractMd5OfCachedName(cachedImage)){
            QFile::remove(gv.cachePath+cachedImage);
            cacheEntry_.size -= cacheEntry_.imagesSizeList.at(i);
            cacheEntry_.images.removeAt(i);
            cacheEntry_.imagesSizeList.removeAt(i);
            cacheEntry_.imageCount--;
            i--;
        }
    }

    if(cacheEntry_.size <= maxCacheSize_){
        return;
    }

    //step 2: Now all cached images are perfectly valid (pointing to existing images). Remove the images that are not under Wallpapers' current path
    for(unsigned int i = 0; i < cacheEntry_.imageCount && cacheEntry_.size > maxCacheSize_; i++){
        QString cachedImageName = cacheEntry_.images.at(i);
        QString originalImage = cacheToOriginalName(cachedImageName);
        QString directoryName = Global::dirnameOf(originalImage);
        bool isSubfolder = false;

        Q_FOREACH(QString curWallpapersFolder, wallpaperFolders){
            if(Global::isSubfolder(directoryName, curWallpapersFolder)){
                //the current image's folder is subfolder of one of the current folders
                isSubfolder = true;
                break;
            }
        }

        if(!isSubfolder){
            QFile::remove(gv.cachePath+cachedImageName);
            cacheEntry_.size-=cacheEntry_.imagesSizeList.at(i);
            cacheEntry_.images.removeAt(i);
            cacheEntry_.imagesSizeList.removeAt(i);
            cacheEntry_.imageCount--;
            i--;
        }
    }

    if(cacheEntry_.size <= maxCacheSize_){
        return;
    }

    //step 3: The only thing left is to delete the images that ARE into the default path.
    for(int i = cacheEntry_.imageCount-1; i >= 0 && cacheEntry_.size > maxCacheSize_; i--){
        QFile::remove(gv.cachePath+cacheEntry_.images.at(i));
        cacheEntry_.size -= cacheEntry_.imagesSizeList.at(i);
        cacheEntry_.images.removeAt(i);
        cacheEntry_.imagesSizeList.removeAt(i);
    }
}

void CacheManager::setMaxCache(qint64 maxCache){
    maxCacheSize_ = maxCache;
    beginFixCacheSize();
}

qint64 CacheManager::getMaxCacheSize(){
    return maxCacheSize_;
}

qint64 CacheManager::getCurrentCacheSize(){
    qint64 finalSize = 0;
    Q_FOREACH(QString file, QDir(gv.cachePath, QString(), QDir::Name, QDir::Files).entryList()){
        finalSize += QFile(gv.cachePath+file).size();
    }
    return finalSize;
}

QString CacheManager::originalToCacheName(QString imagePath){
    /*
     * How the cache works:
     * 1: All / are converted to ^ (or the cacheToChar)
     * 2: If ^ is already on the string, the indexes of it are saved at the start of the cached filename
     * 3: Lastly, the size of the original image is saved at the start of the cached image to prevent
     * having a different image with the same filename.
     */
    QString checksum = md5Of(imagePath);
#ifdef Q_OS_WIN
    imagePath.remove(1, 1); //removing the ":" from the filename
#endif

    if(imagePath.contains(cacheToChar_)){
        //imagePath already includes the cacheToChar, so save the index(es) of it
        QString finalString = imagePath;
        short currentIndex = 0;
        while((currentIndex = imagePath.indexOf(cacheToChar_, currentIndex+1)) != -1)
            finalString.insert(0, QString::number(currentIndex)+cacheAlreadyIncludedDelimiter_);
        finalString.insert(0, checksum+cacheCheckSumChar_);
        return finalString.replace(cacheFromChar_, cacheToChar_);
    }
    else
    {
        imagePath.insert(0, checksum+cacheCheckSumChar_);
        return imagePath.replace(cacheFromChar_, cacheToChar_);
    }
}

QString CacheManager::cacheExcludeMd5(const QString &cacheName){
    QChar mchar;
    short i = 0, extrasCount = 1, cacheNameCount = cacheName.count();
    for(mchar = cacheName.at(i); mchar != cacheCheckSumChar_ && i < cacheNameCount; i++, mchar = cacheName.at(i)){
        extrasCount++;
    }
    return cacheName.right(cacheName.count()-extrasCount);
}

QString CacheManager::cacheToOriginalName(QString cacheName){
    cacheName = cacheExcludeMd5(cacheName);
    short cacheNameCount = cacheName.count();

    if(!cacheName.startsWith(cacheToChar_)){
        //the rare case where the original filename already included the cacheToChar
        QStringList sNums = cacheName.split(cacheAlreadyIncludedDelimiter_);
        QList<short> nums;
        short ocCount = sNums.count();
        short lengthToOmit=0, numsCount=0;
        for(short i = 0; i < ocCount; i++){
            bool isNum = false;
            short num = sNums.at(i).toShort(&isNum);
            if(isNum){
                lengthToOmit += (sNums.at(i).length()+1); //the +1 for the cacheAlreadyIncludedDelimiter
                nums.append(num);
                numsCount++;
            }
            else{
                break;
            }
        }
        cacheName = cacheName.right(cacheNameCount-lengthToOmit);
        cacheName.replace(cacheToChar_, cacheFromChar_);
        for(short i = 0; i < numsCount; i++)
            cacheName.replace(nums.at(i), 1, cacheToChar_);
#ifdef Q_OS_WIN
        return cacheName.insert(1, ':'); //adding the ":" to the filename
#else
        return cacheName;
#endif //#ifdef Q_OS_WIN
    }
    else
    {
#ifdef Q_OS_WIN
        return cacheName.replace(cacheToChar_, cacheFromChar_).insert(1, ':'); //adding the ":" to the filename
#else
        return cacheName.replace(cacheToChar_, cacheFromChar_);
#endif //#ifdef Q_OS_WIN
    }
}

QString CacheManager::extractMd5OfCachedName(const QString &cacheName){
    //returns the size of the original image of cacheName, which is included inside cacheName
    QChar curChar;
    short i = 0, extraCount = 0, cacheNameCount = cacheName.count();
    for(curChar = cacheName.at(i); curChar != cacheCheckSumChar_ && i < cacheNameCount; curChar = cacheName.at(++i))
        extraCount++;
    return cacheName.left(extraCount);
}

void CacheManager::updateCacheStatus(){
    qint64 finalSize = 0; int counter = 0;
    cacheEntry_.images = QDir(gv.cachePath, QString(), QDir::Name, QDir::Files).entryList();
    Q_FOREACH(QString file, cacheEntry_.images){
        qint64 currentSize = QFile(gv.cachePath+file).size();
        finalSize += currentSize;
        cacheEntry_.imagesSizeList.append(currentSize);
        counter++;
    }
    cacheEntry_.imageCount = counter;
    cacheEntry_.size = finalSize;
}

bool CacheManager::addToCacheSize(qint64 size){
    /*
     * Returns true if the current cache size exceeds the max cache size
    */
    cacheEntry_.size += size;
    return (cacheEntry_.size > maxCacheSize_ && maxCacheSize_ != -1);
}

bool CacheManager::removeFromCacheSize(qint64 size){
    cacheEntry_.size -= size;
    return (cacheEntry_.size > maxCacheSize_ && maxCacheSize_ != -1);
}

QString CacheManager::getCacheFullPath(const QString &originalName){
    return gv.cachePath+originalToCacheName(originalName);
}

QImage CacheManager::generateThumbnail(const QString &originalName){
    QImageReader reader(originalName);
    reader.setDecideFormatFromContent(true);
    if(!reader.canRead()){
        return QImage();
    }
    reader.setScaledSize(QSize(CACHED_IMAGES_SIZE));
    return reader.read();
}

bool CacheManager::cacheEnabled(){
    return (maxCacheSize_ != 0);
}

QString CacheManager::md5Of(const QString &filename){
    QCryptographicHash crypto(QCryptographicHash::Md5);
    QFile file(filename);

    if(!file.open(QFile::ReadOnly)){
        return QString();
    }
    crypto.addData(file.read(2000));
    file.close();

    return QString(crypto.result().toHex());
}

QImage CacheManager::controlCache(const QString &originalImagePath){
    /*
     * Controls the returned image based on the original image path. For example, if the image is already
     * cached, it will return the cached copy, or if it not, depending on whether caching is enabled or
     * not, it will scale down the image, cache it and return it.
    */
    QString cachedImagePath = getCacheFullPath(originalImagePath);
    bool haveToCache = true;

    QString imagePathToLoad;

    if(cacheEnabled() && QFile(cachedImagePath).exists())
    {
        //thumbnail already exists
        imagePathToLoad = cachedImagePath;
        haveToCache = false;
    }
    else
    {
        imagePathToLoad = originalImagePath;
    }

    QImageReader reader(imagePathToLoad);
    reader.setDecideFormatFromContent(true);

    if(!reader.canRead()){
        return QImage();
    }
    QImage image;
    if(haveToCache){
        //means that have to read the image from the original source.
        /*
         * The reason that here a check for the dimensions of the image is not being done
         * (so as not to cache it if it is already too small) is because of this bug:
         * https://bugreports.qt-project.org/browse/QTBUG-24831
         * I could create a very small symlink to the target of the image but QFile::size
         * does not return the size of the symlink but the size of the actual file and
         * this creates problems with managing the size of the cache. Also, saving images
         * slightly smaller than CACHED_IMAGES_SIZE as resized (to CACHED_IMAGES_SIZE)
         * resulted in smaller images than the originals(!).
         */
        reader.setScaledSize(QSize(CACHED_IMAGES_SIZE));
        image = reader.read();
        if(cacheEnabled()){
            if(image.save(cachedImagePath, "JPG")){
                cacheSizeChanged(cachedImagePath); //todo
            }
        }
    }
    else
    {
        //just read the already cached value
        image = reader.read();
    }
    return image;
}

void CacheManager::cacheSizeChanged(const QString &newCacheImage /* = QString() */){
    if(newCacheImage.isEmpty() || addToCacheSize(QFile(newCacheImage).size())){
        //if cache size has exceeded the limit, then it goes through here
        if(cacheSizeFixer_ == NULL){
            cacheSizeFixer_ = new QTimer(this);
            connect(cacheSizeFixer_, SIGNAL(timeout()), this, SLOT(beginFixCacheSize()));
            cacheSizeFixer_->setSingleShot(true);
        }
        if(cacheSizeFixer_->isActive()){
            cacheSizeFixer_->stop();
        }
        cacheSizeFixer_->start(CHECK_CACHE_SIZE_TIMEOUT);
    }
}

void CacheManager::beginFixCacheSize(){
    //request the current controlling folders so as to begin fixing the cache size!
    Q_EMIT requestCurrentFolders();
}

QString CacheManager::removeCacheOf(const QString &originalImagePath){
    /*
     * Removes the potentially cached image of the original image path and returns
     * the cache filename of it.
    */
    QString cacheFilename = getCacheFullPath(originalImagePath);
    if(QFile(cacheFilename).exists()){
        qint64 fileSize = QFile(cacheFilename).size();
        if(QFile::remove(cacheFilename)){
            removeFromCacheSize(fileSize);
        }
    }
    return cacheFilename;
}

QImage CacheManager::forceCache(const QString &originalImagePath){
    /*
     * Forcefully recaches the image.
    */

    QString cacheFullPath = removeCacheOf(originalImagePath);
    QImage image = generateThumbnail(originalImagePath);

    if(cacheEnabled() && image.save(cacheFullPath, "JPG")){
        cacheSizeChanged(cacheFullPath);
    }
    return image;
}
