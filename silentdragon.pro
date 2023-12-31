# Copyright 2018-2022 The Hush Developers
# Released under the GPLv3
# Project created by QtCreator 2018-10-05T09:54:45

QT       += core gui network

CONFIG += precompile_header

PRECOMPILED_HEADER = src/precompiled.h

QT += widgets

TARGET = silentdragon

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += \
    QT_DEPRECATED_WARNINGS

INCLUDEPATH  += src/3rdparty/
INCLUDEPATH  += src/

mac: LIBS+= -Wl,-dead_strip
mac: LIBS+= -Wl,-dead_strip_dylibs
mac: LIBS+= -Wl,-bind_at_load

RESOURCES     = application.qrc

MOC_DIR = bin
OBJECTS_DIR = bin
UI_DIR = src

CONFIG += c++14

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/rpc.cpp \
    src/balancestablemodel.cpp \
    src/3rdparty/qrcode/BitBuffer.cpp \
    src/3rdparty/qrcode/QrCode.cpp \
    src/3rdparty/qrcode/QrSegment.cpp \
    src/settings.cpp \
    src/sendtab.cpp \
    src/senttxstore.cpp \
    src/txtablemodel.cpp \
    src/peerstablemodel.cpp \
    src/bannedpeerstablemodel.cpp \
    src/qrcodelabel.cpp \
    src/connection.cpp \
    src/fillediconlabel.cpp \
    src/addressbook.cpp \
    src/logger.cpp \
    src/addresscombo.cpp \
    src/validateaddress.cpp \
    src/recurring.cpp \
    src/requestdialog.cpp \
    src/memoedit.cpp \
    src/viewalladdresses.cpp

HEADERS += \
    src/guiconstants.h \
    src/mainwindow.h \
    src/precompiled.h \
    src/rpc.h \
    src/balancestablemodel.h \
    src/3rdparty/qrcode/BitBuffer.hpp \
    src/3rdparty/qrcode/QrCode.hpp \
    src/3rdparty/qrcode/QrSegment.hpp \
    src/settings.h \
    src/txtablemodel.h \
    src/peerstablemodel.h \
    src/bannedpeerstablemodel.h \
    src/senttxstore.h \
    src/qrcodelabel.h \
    src/connection.h \
    src/fillediconlabel.h \
    src/addressbook.h \
    src/logger.h \
    src/addresscombo.h \
    src/validateaddress.h \
    src/recurring.h \
    src/requestdialog.h \
    src/memoedit.h \
    src/viewalladdresses.h

FORMS += \
    src/getblock.ui \
    src/mainwindow.ui \
    src/qrcode.ui \
    src/rescandialog.ui \
    src/settings.ui \
    src/about.ui \
    src/confirm.ui \
    src/privkey.ui \
    src/viewkey.ui \
    src/memodialog.ui \
    src/viewalladdresses.ui \
    src/validateaddress.ui \
    src/viewalladdresses.ui \
    src/connection.ui \
    src/addressbook.ui \
    src/viewalladdresses.ui \
    src/createhushconfdialog.ui \
    src/recurringdialog.ui \
    src/newrecurring.ui \
    src/requestdialog.ui


TRANSLATIONS = res/silentdragon_be.ts \
               res/silentdragon_bg.ts \
               res/silentdragon_de.ts \
               res/silentdragon_es.ts \
               res/silentdragon_fi.ts \
               res/silentdragon_fil.ts \
               res/silentdragon_fr.ts \
               res/silentdragon_hr.ts \
               res/silentdragon_id.ts \
               res/silentdragon_it.ts \
               res/silentdragon_nl.ts \
               res/silentdragon_pl.ts \
               res/silentdragon_pt.ts \
               res/silentdragon_ro.ts \
               res/silentdragon_ru.ts \
               res/silentdragon_sr.ts \
               res/silentdragon_tr.ts \
               res/silentdragon_uk.ts \
               res/silentdragon_zh.ts

include(singleapplication/singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=QApplication _FORTIFY_SOURCE=2

QMAKE_INFO_PLIST = res/Info.plist

win32: RC_ICONS = res/icon.ico
ICON = res/logo.icns

libsodium.target = $$PWD/res/libsodium.a
libsodium.commands = res/libsodium/buildlibsodium.sh

QMAKE_EXTRA_TARGETS += libsodium
QMAKE_CLEAN += res/libsodium.a

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/res/ -llibsodium
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/res/ -llibsodiumd
else:unix: LIBS += -L$$PWD/res/ -lsodium

INCLUDEPATH += $$PWD/res
DEPENDPATH += $$PWD/res

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/res/liblibsodium.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/res/liblibsodium.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/res/libsodium.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/res/libsodiumd.lib
else:unix: PRE_TARGETDEPS += $$PWD/res/libsodium.a
