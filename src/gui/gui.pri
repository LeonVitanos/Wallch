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

HEADERS += $$PWD/about.h \
           $$PWD/history.h \
           $$PWD/mainwindow.h \
           $$PWD/preferences.h \
           $$PWD/properties.h \
           $$PWD/colors_gradients.h \
           $$PWD/notification.h \

SOURCES += $$PWD/about.cpp \
           $$PWD/history.cpp \
           $$PWD/mainwindow.cpp \
           $$PWD/preferences.cpp \
           $$PWD/properties.cpp \
           $$PWD/colors_gradients.cpp \
           $$PWD/notification.cpp \

FORMS += $$PWD/about.ui \
         $$PWD/history.ui \
         $$PWD/mainwindow.ui \
         $$PWD/preferences.ui \
         $$PWD/properties.ui \
         $$PWD/colors_gradients.ui \
         $$PWD/notification.ui \
