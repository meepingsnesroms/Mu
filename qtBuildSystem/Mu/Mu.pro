#-------------------------------------------------
#
# Project created by QtCreator 2018-04-11T12:57:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Mu
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# ios {
#     QMAKE_INFO_PLIST = ios/Info.plist
# }

macx {
    QMAKE_INFO_PLIST = macos/Info.plist
}

CONFIG(debug, debug|release){
    DEFINES += EMU_DEBUG EMU_CUSTOM_DEBUG_LOG_HANDLER EMU_LOG_REGISTER_ACCESS_UNKNOWN
#   DEFINES += EMU_OPCODE_LEVEL_DEBUG EMU_LOG_APIS
}

DEFINES += EMU_MULTITHREADED
QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/qt-common/include

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    touchscreen.cpp \
    src/bps/crc32.c \
    src/bps/libbps.c \
    src/m68k/m68kcpu.c \
    src/m68k/m68kdasm.c \
    src/m68k/m68kopac.c \
    src/m68k/m68kopdm.c \
    src/m68k/m68kopnz.c \
    src/m68k/m68kops.c \
    src/68328Functions.c \
    src/emulator.c \
    src/hardwareRegisters.c \
    src/memoryAccess.c \
    src/sdcard.c \
    src/sed1376.c \
    src/silkscreen.c \
    src/trapNumToName.c \
    src/ads7846.c \
    debugviewer.cpp \
    emuwrapper.cpp

HEADERS += \
    src/bps/crc32.h \
    src/bps/global.h \
    src/bps/libbps.h \
    src/m68k/m68k.h \
    src/m68k/m68kconf.h \
    src/m68k/m68kcpu.h \
    src/m68k/m68kops.h \
    src/68328Functions.h \
    src/emulator.h \
    src/hardwareRegisters.h \
    src/memoryAccess.h \
    src/portability.h \
    src/sdcard.h \
    src/sed1376.h \
    src/silkscreen.h \
    mainwindow.h \
    touchscreen.h \
    src/hardwareRegistersTiming.c.h \
    src/sed1376Accessors.c.h \
    src/hardwareRegistersAccessors.c.h \
    src/endianness.h \
    src/ads7846.h \
    debugviewer.h \
    src/specs/emuFeatureRegistersSpec.h \
    src/specs/hardwareRegisterNames.h \
    src/specs/irdaCommands.h \
    src/specs/sed1376RegisterNames.h \
    emuwrapper.h

FORMS += \
    mainwindow.ui \
    debugviewer.ui

CONFIG += mobility
MOBILITY = 

DISTFILES += \
    images/addressBook.png \
    images/center.png \
    images/down.png \
    images/left.png \
    images/notes.png \
    images/power.png \
    images/right.png \
    images/up.png \
    images/todo.png \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    images/pause.png \
    images/play.png \
    images/saveLoad.png \
    images/screenshot.png \
    images/stop.png \
    images/calendar.png

RESOURCES += \
    mainwindow.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
