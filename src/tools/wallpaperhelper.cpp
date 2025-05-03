#include "wallpaperhelper.h"

#ifdef Q_OS_LINUX
    #include "desktopenvironment.h"
#endif

WallpaperHelper::WallpaperHelper(WallpaperManager* wallpaperManager, QListWidget* wallpapersList)
    : wallpaperManager_(wallpaperManager), wallpapersList_(wallpapersList) {}

QString WallpaperHelper::getPathOfListItem(){
    /*
     * Returns the full path of the listwidget item at 'index'
     * Returns the full path of the selected item's path if index==-1
     */

   int index = wallpapersList_->currentRow();

    if (index < 0)
       return QString();

    if(gv.iconMode){
        if(wallpapersList_->item(index)->statusTip().isEmpty())
            return wallpapersList_->item(index)->toolTip();
        else
            return wallpapersList_->item(index)->statusTip();
    }
    else
        return wallpapersList_->item(index)->text();
}
