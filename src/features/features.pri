# Wallch - A Modern, Cross-Platform Wallpaper Changer
#
# Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
# Copyright © 2025-2026, Leon Vitanos (Modernization)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

INCLUDEPATH += $$PWD/wallpapers
INCLUDEPATH += $$PWD/website
INCLUDEPATH += $$PWD/liveearth
INCLUDEPATH += $$PWD/potd

HEADERS += $$PWD/wallpapers/wallpapersfeature.h \
    $$PWD/wallpapers/searchimages.h \
    $$PWD/wallpapers/filemanager.h \
    $$PWD/wallpapers/cachemanager.h \
    $$PWD/wallpapers/wallpaperhelper.h \
    $$PWD/wallpapers/pictures_locations.h \
    $$PWD/featurecontroller.h \
    $$PWD/liveearth/liveearthfeature.h \
    $$PWD/liveearth/markitem.h \
    $$PWD/liveearth/lepoint.h \
    $$PWD/potd/potdfeature.h \
    $$PWD/potd/potd_viewer.h \
    $$PWD/potd/potd_preview.h \
    $$PWD/website/websitefeature.h \
    $$PWD/website/websitesnapshot.h \
    $$PWD/website/crop_image.h \
    $$PWD/website/website_preview.h

SOURCES += $$PWD/wallpapers/wallpapersfeature.cpp \
    $$PWD/wallpapers/searchimages.cpp \
    $$PWD/wallpapers/filemanager.cpp \
    $$PWD/wallpapers/cachemanager.cpp \
    $$PWD/wallpapers/wallpaperhelper.cpp \
    $$PWD/wallpapers/pictures_locations.cpp \
    $$PWD/featurecontroller.cpp \
    $$PWD/liveearth/liveearthfeature.cpp \
    $$PWD/liveearth/markitem.cpp \
    $$PWD/liveearth/lepoint.cpp \
    $$PWD/potd/potdfeature.cpp \
    $$PWD/potd/potd_viewer.cpp \
    $$PWD/potd/potd_preview.cpp \
    $$PWD/website/websitefeature.cpp \
    $$PWD/website/crop_image.cpp \
    $$PWD/website/websitesnapshot.cpp \
    $$PWD/website/website_preview.cpp

FORMS += $$PWD/wallpapers/pictures_locations.ui \
    $$PWD/liveearth/lepoint.ui \
    $$PWD/potd/potd_viewer.ui \
    $$PWD/potd/potd_preview.ui \
    $$PWD/website/website_preview.ui \
    $$PWD/website/crop_image.ui
