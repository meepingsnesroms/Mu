#-------------------------------------------------
#
# Project created by QtCreator 2018-04-11T12:57:49
#
#-------------------------------------------------

QT += core gui svg

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

android {
    QMAKE_CFLAGS += -fopenmp
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
}

CONFIG(debug, debug|release){
    DEFINES += EMU_DEBUG EMU_SANDBOX EMU_CUSTOM_DEBUG_LOG_HANDLER
    DEFINES += EMU_SANDBOX_OPCODE_LEVEL_DEBUG
    DEFINES += EMU_SANDBOX_LOG_APIS
}

DEFINES += EMU_MULTITHREADED
QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/qt-common/include

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    touchscreen.cpp \
    src/m68k/m68kcpu.c \
    src/m68k/m68kdasm.c \
    src/m68k/m68kopac.c \
    src/m68k/m68kopdm.c \
    src/m68k/m68kopnz.c \
    src/m68k/m68kops.c \
    src/emulator.c \
    src/hardwareRegisters.c \
    src/memoryAccess.c \
    src/sdCard.c \
    src/sed1376.c \
    src/silkscreen.c \
    src/ads7846.c \
    debugviewer.cpp \
    emuwrapper.cpp \
    src/debug/sandbox.c \
    statemanager.cpp \
    src/m68328.c \
    src/pdiUsbD12.c

HEADERS += \
    src/m68k/m68k.h \
    src/m68k/m68kconf.h \
    src/m68k/m68kcpu.h \
    src/m68k/m68kops.h \
    src/emulator.h \
    src/hardwareRegisters.h \
    src/memoryAccess.h \
    src/portability.h \
    src/sdCard.h \
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
    src/specs/sed1376RegisterNames.h \
    emuwrapper.h \
    src/debug/sandbox.h \
    src/debug/sandboxTrapNumToName.c.h \
    statemanager.h \
    src/m68328.h \
    src/pdiUsbD12.h \
    src/specs/pdiUsbD12Commands.h \
    src/debug/trapNames.h

FORMS += \
    mainwindow.ui \
    debugviewer.ui \
    statemanager.ui

CONFIG += mobility
MOBILITY = 

DISTFILES += \
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
    images/addressBook.svg \
    images/calendar.svg \
    images/center.svg \
    images/debugger.svg \
    images/down.svg \
    images/install.svg \
    images/left.svg \
    images/notes.svg \
    images/pause.svg \
    images/play.svg \
    images/power.svg \
    images/right.svg \
    images/screenshot.svg \
    images/stateManager.svg \
    images/stop.svg \
    images/todo.svg \
    images/up.svg

RESOURCES += \
    mainwindow.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
