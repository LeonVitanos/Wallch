#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H
#define QT_NO_KEYWORDS

#include <QObject>
#include <QStringList>
#include <QImage>
#include <QHash>

#define CACHED_IMAGES_SIZE 80, 80
#define BYTES_PER_MiB 1048576.0

#define CHECK_CACHE_SIZE_TIMEOUT 5000

typedef struct
{
    QStringList images;
    unsigned int imageCount;
    QList<int> imagesSizeList;
    qint64 size;
} CacheEntry;

class CacheManager : public QObject
{
    Q_OBJECT

public:
    explicit CacheManager(QObject *parent = 0);
    void fixCacheSizeWithCurrentFoldersBeing(const QStringList &wallpaperFolders);
    qint64 getMaxCacheSize();
    static qint64 getCurrentCacheSize();

    bool cacheEnabled();
    QHash<int, QPair<QString, qint64>> maxCacheIndexes = initCacheIndex();
    QImage controlCache(const QString &originalImagePath);
    QImage forceCache(const QString &originalImagePath);
    QString removeCacheOf(const QString &originalImagePath);

public Q_SLOTS:
    void setMaxCache(qint64 maxCache);

private Q_SLOTS:
    void beginFixCacheSize();

private:
    QString extractMd5OfCachedName(const QString &cacheName);
    QString cacheExcludeMd5(const QString &cacheName);
    QString md5Of(const QString &filename);
    QString originalToCacheName(QString imagePath);
    QString cacheToOriginalName(QString cacheName);
    QString getCacheFullPath(const QString &originalName);
    void updateCacheStatus();
    void cacheSizeChanged(const QString &newCacheImage = QString());
    QHash<int, QPair<QString, qint64>> initCacheIndex(){
        QHash<int, QPair<QString, qint64>> indexHash;
        indexHash.insert(0, QPair<QString, qint64>(tr("Disabled"), 0));
        indexHash.insert(1, QPair<QString, qint64>("2MiB", Q_INT64_C(2097152)));
        indexHash.insert(2, QPair<QString, qint64>("10MiB", Q_INT64_C(10485760)));
        indexHash.insert(3, QPair<QString, qint64>("20MiB", Q_INT64_C(20971520)));
        indexHash.insert(4, QPair<QString, qint64>("50MiB", Q_INT64_C(52428800)));
        indexHash.insert(5, QPair<QString, qint64>("100MiB", Q_INT64_C(104857600)));
        indexHash.insert(6, QPair<QString, qint64>("200MiB", Q_INT64_C(209715200)));
        indexHash.insert(7, QPair<QString, qint64>("500MiB", Q_INT64_C(524288000)));
        indexHash.insert(8, QPair<QString, qint64>("1GiB", Q_INT64_C(1073741824)));
        indexHash.insert(9, QPair<QString, qint64>("5GiB", Q_INT64_C(5368709120)));
        indexHash.insert(10, QPair<QString, qint64>(tr("Unlimited"), Q_INT64_C(-1)));
        return indexHash;
    }
    bool addToCacheSize(qint64 size);
    bool removeFromCacheSize(qint64 size);
    QImage generateThumbnail(const QString &originalName);

    const QChar cacheFromChar_ = '/'; //do NOT change
    const QChar cacheToChar_ = '^'; //change, but it CANNOT be the same as cacheAlreadyIncludedDelimiter_
    const QChar cacheAlreadyIncludedDelimiter_ = '.'; //cannot be the same with the cacheToChar_
    const QChar cacheCheckSumChar_ = '=';

    CacheEntry cacheEntry_;
    qint64 maxCacheSize_ = Q_INT64_C( 10485760 ); //default max cache
    qint64 currentCacheSize_ = 0;
    QTimer *cacheSizeFixer_ = NULL;

Q_SIGNALS:
    /*
     * Requests the current folders so as to be able to prioritize the deletion
     * of the cache images. When this signal is emitted it is the time to call
     * the fixCacheSizeWithCurrentFoldersBeing function with the current folders
     * as parameter
    */
    void requestCurrentFolders();
};

#endif // CACHEMANAGER_H
