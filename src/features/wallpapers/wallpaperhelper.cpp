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

#include "wallpaperhelper.h"
#include "glob.h"

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

    //TODO
    (void) wallpaperManager_;
}
