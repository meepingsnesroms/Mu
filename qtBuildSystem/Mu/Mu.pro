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

CONFIG(debug, debug|release){
    DEFINES += EMU_DEBUG EMU_OPCODE_LEVEL_DEBUG
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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
    src/trapNumToName.c

HEADERS += \
    qt-common/include/boolean.h \
    qt-common/include/retro_endianness.h \
    qt-common/include/retro_inline.h \
    src/bps/crc32.h \
    src/bps/global.h \
    src/bps/libbps.h \
    src/m68k/m68k.h \
    src/m68k/m68kconf.h \
    src/m68k/m68kcpu.h \
    src/m68k/m68kops.h \
    src/68328Functions.h \
    src/emuFeatureRegistersSpec.h \
    src/emulator.h \
    src/hardwareRegisterNames.h \
    src/hardwareRegisters.h \
    src/memoryAccess.h \
    src/portability.h \
    src/sdcard.h \
    src/sed1376.h \
    src/silkscreen.h \
    mainwindow.h \
    touchscreen.h

FORMS += \
        mainwindow.ui

CONFIG += mobility c++11
MOBILITY = 

DISTFILES += \
    images/addressBook.png \
    images/calender.png \
    images/center.png \
    images/down.png \
    images/left.png \
    images/notes.png \
    images/power.png \
    images/right.png \
    images/up.png \
    images/todo.png

RESOURCES += \
    mainwindow.qrc
