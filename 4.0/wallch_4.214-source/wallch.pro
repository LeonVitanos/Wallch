DEFINES += "APP_VERSION=4.214"

TEMPLATE = app
TARGET = wallch
DEPENDPATH += . data/translations
INCLUDEPATH += .

CONFIG += c++11

QT += network widgets webkitwidgets

win32 {
    QT += gui-private
    RC_ICONS += data/win-icon/wallch.ico
    VERSION = 4.214
    QMAKE_TARGET_COMPANY = "Mellori Studio"
    QMAKE_TARGET_PRODUCT = "Wallch"
    QMAKE_TARGET_DESCRIPTION = "Wallch"
    QMAKE_TARGET_COPYRIGHT = "GNU General Public License"
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += unity libnotify libexif gtk+-2.0
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
