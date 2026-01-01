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
    QMAKE_TARGET_COMPANY = "Mellori Studio"
    QMAKE_TARGET_PRODUCT = "Wallch"
    QMAKE_TARGET_DESCRIPTION = "Wallch"
    QMAKE_TARGET_COPYRIGHT = "GNU General Public License"
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
