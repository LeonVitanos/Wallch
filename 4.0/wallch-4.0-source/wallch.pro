TEMPLATE = app
TARGET = wallch
DEPENDPATH += . data/translations src/
INCLUDEPATH += .

CONFIG += c++11

QT += network widgets webkitwidgets

win32 {
    QT += gui-private
    DEFINES += ON_WIN32
} else {
    DEFINES += ON_LINUX
    CONFIG += link_pkgconfig
    PKGCONFIG += appindicator-0.1 unity libnotify libexif keybinder
}

isEmpty(PREFIX) {
    PREFIX = /usr
}

DEFINES += "PREFIX=\\\"$$PREFIX\\\""

# Input
VPATH += ./src
HEADERS += about.h \
           glob.h \
           history.h \
           mainwindow.h \
           preferences.h \
           properties.h \
           statistics.h \
           website_preview.h \
           crop_image.h \
           colors_gradients.h \
           nongui.h \
           potd_viewer.h \
           notification.h \
           potd_preview.h \
           websitesnapshot.h \
           customwebpage.h

FORMS += about.ui \
         history.ui \
         mainwindow.ui \
         preferences.ui \
         properties.ui \
         statistics.ui \
         website_preview.ui \
         crop_image.ui \
         colors_gradients.ui \
         potd_viewer.ui \
         notification.ui \
         potd_preview.ui

SOURCES += about.cpp \
           glob.cpp \
           history.cpp \
           main.cpp \
           mainwindow.cpp \
           preferences.cpp \
           properties.cpp \
           statistics.cpp \
           website_preview.cpp \
           crop_image.cpp \
           colors_gradients.cpp \
           nongui.cpp \
           potd_viewer.cpp \
           notification.cpp \
           potd_preview.cpp \
           websitesnapshot.cpp

RESOURCES += icons.qrc

TRANSLATIONS += data/translations/wallch_el.ts

configfiles.files += data/to_usr_share/*
configfiles.path = $$PREFIX/share
shortcutfiles.files += data/wallch-nautilus.desktop
shortcutfiles.path = /usr/share/applications/
unix:configfiles.extra = lrelease -silent data/translations/wallch_el.ts -qm data/to_usr_share/wallch/translations/wallch_el.qm
INSTALLS += shortcutfiles
INSTALLS += configfiles
