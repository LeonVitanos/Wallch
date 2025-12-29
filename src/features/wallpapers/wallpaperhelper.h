#ifndef WALLPAPERHELPER_H
#define WALLPAPERHELPER_H

#include <QString>
#include <QObject>
#include <QListWidget>
#include "wallpapermanager.h"

class WallpaperHelper {
public:
    explicit WallpaperHelper(WallpaperManager* wallpaperManager, QListWidget* wallpapersList);

    QString getPathOfListItem();
private:
    WallpaperManager* wallpaperManager_;
    QListWidget* wallpapersList_;
};

#endif // WALLPAPERHELPER_H
