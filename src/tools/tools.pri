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

INCLUDEPATH += $$PWD/desktop

HEADERS += $$PWD/glob.h \
           $$PWD/colormanager.h \
           $$PWD/desktop/desktopenvironment.h \
           $$PWD/dialoghelper.h \
           $$PWD/imageorientationhandler.h \
           $$PWD/nonguimanager.h \
           $$PWD/settingsmanager.h \
           $$PWD/timermanager.h \
           $$PWD/traymanager.h \
           $$PWD/customwebpage.h \
           $$PWD/wallpapermanager.h \
           $$PWD/imagefetcher.h \
           $$PWD/tryhard.h

SOURCES += $$PWD/glob.cpp \
           $$PWD/colormanager.cpp \
           $$PWD/desktop/desktopenvironment.cpp \
           $$PWD/dialoghelper.cpp \
           $$PWD/imageorientationhandler.cpp \
           $$PWD/nonguimanager.cpp \
           $$PWD/settingsmanager.cpp \
           $$PWD/timermanager.cpp \
           $$PWD/traymanager.cpp \
           $$PWD/wallpapermanager.cpp \
           $$PWD/imagefetcher.cpp \
           $$PWD/tryhard.cpp

unix:!macx {
    HEADERS += $$PWD/desktop/desktopenvironment_kde.h
    SOURCES += $$PWD/desktop/desktopenvironment_kde.cpp
}
