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

DEFINES += "APP_VERSION=4.214"

TEMPLATE = app
TARGET = wallch
DEPENDPATH += . data/translations
INCLUDEPATH += .

CONFIG += c++11

QT += network widgets

win32 {
    QT += gui-private
    RC_ICONS += data/win-icon/wallch.ico
    VERSION = 4.214
    QMAKE_TARGET_COMPANY = "Leon Vitanos"
    QMAKE_TARGET_PRODUCT = "Wallch"
    QMAKE_TARGET_DESCRIPTION = "A Modern, Cross-Platform Wallpaper Changer"
    QMAKE_TARGET_COPYRIGHT = "© 2010-2015 A. Solanos, L. Vitanos; 2025-2026 L. Vitanos (Modernization)"
}

unix:!macx {
    # Use no_keywords to prevent Qt's 'signals'/'slots' from conflicting
    # with 3rd-party system libraries on Linux (e.g., when including gio.h).
    # This requires using Q_SIGNALS/Q_SLOTS macros in the C++ code.
    # See: https://doc.qt.io/qt-6/signalsandslots.html#using-qt-with-3rd-party-signals-and-slots
    CONFIG += link_pkgconfig no_keywords
    PKGCONFIG += libnotify libexif
    QT += dbus
}

macx {
    CONFIG += sdk_no_version_check
}

isEmpty(PREFIX) {
    PREFIX = /usr
}

DEFINES += "PREFIX=\\\"$$PREFIX\\\""

SOURCES += src/main.cpp
RESOURCES += src/resources.qrc

TRANSLATIONS += data/translations/wallch_el.ts

configfiles.files += data/to_usr_share/*
configfiles.path = $$PREFIX/share
unix:configfiles.extra = lrelease -silent data/translations/wallch_el.ts -qm data/to_usr_share/wallch/translations/wallch_el.qm
INSTALLS += configfiles

include(src/tools/tools.pri)
include(src/gui/gui.pri)
include(src/common/common.pri)
include(src/features/features.pri)
