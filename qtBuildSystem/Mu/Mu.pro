#-------------------------------------------------
#
# Project created by QtCreator 2018-04-11T12:57:49
#
#-------------------------------------------------

QT += core gui multimedia svg

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
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000 # disables all the APIs deprecated before Qt 6.0.0

CONFIG += support_palm_os5

windows{
    RC_ICONS = windows/Mu.ico
    *msvc*{
        QMAKE_CFLAGS += -openmp
        QMAKE_CXXFLAGS += -openmp
        DEFINES += "_Pragma=__pragma"
    }
    *-g++{
        QMAKE_CFLAGS += -fopenmp
        QMAKE_CXXFLAGS += -fopenmp
        QMAKE_LFLAGS += -fopenmp
    }
    DEFINES += EMU_MULTITHREADED
    CONFIG += cpu_x86_32 # this should be auto detected in the future
}

macx{
    QMAKE_CFLAGS += -std=c89 -D__STDBOOL_H -Dinline= -Dbool=char -Dtrue=1 -Dfalse=0 # tests C89 mode
    ICON = macos/Mu.icns
    QMAKE_INFO_PLIST = macos/Info.plist
    DEFINES += EMU_MULTITHREADED
    CONFIG += cpu_x86_64 # Mac OS is only x86_64
}

linux-g++{
    QMAKE_CFLAGS += -fopenmp
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
    DEFINES += EMU_MULTITHREADED
    CONFIG += cpu_x86_64 # this should be auto detected in the future
}

android{
    QMAKE_CFLAGS += -fopenmp
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
    DEFINES += EMU_MULTITHREADED
    CONFIG += cpu_armv7 # this should be auto detected in the future
}


CONFIG(debug, debug|release){
    # debug build, be accurate, fail hard, and add logging
    DEFINES += EMU_DEBUG EMU_CUSTOM_DEBUG_LOG_HANDLER EMU_SANDBOX
    # DEFINES += EMU_SANDBOX_LOG_MEMORY_ACCESSES # checks all reads and writes to memory and logs certain events
    # DEFINES += EMU_SANDBOX_OPCODE_LEVEL_DEBUG # for breakpoints
    # DEFINES += EMU_SANDBOX_LOG_JUMPS # log large jumps
    # DEFINES += EMU_SANDBOX_LOG_APIS # for printing sysTrap* calls, EMU_SANDBOX_OPCODE_LEVEL_DEBUG must be on too
    DEFINES += NO_TRANSLATION # easier to debug with
    macx|linux-g++{
        # also check for any buffer overflows and memory leaks
        # -fsanitize=undefined,leak
        QMAKE_CFLAGS += -fstack-protector-strong -fsanitize=address -Werror=array-bounds
        QMAKE_CXXFLAGS += -fstack-protector-strong -fsanitize=address -Werror=array-bounds
        QMAKE_LFLAGS += -fsanitize=address
    }
}else{
    # release build, go fast
    DEFINES += EMU_NO_SAFETY
}

support_palm_os5{
    DEFINES += EMU_SUPPORT_PALM_OS5 # the Qt build will not be supporting anything too slow to run OS 5

    !no_dynarec{
        # Windows is only supported in 32 bit mode right now(this is a limitation of the dynarec)
        # iOS needs IS_IOS_BUILD set, but the Qt port does not support iOS currently

        cpu_x86_32{
            SOURCES += \
                ../../src/armv5te/translate_x86.c
        }

        cpu_x86_64{
            SOURCES += \
                ../../src/armv5te/translate_x86_64.c
        }

        cpu_armv7{
            SOURCES += \
                ../../src/armv5te/translate_arm.cpp
        }

        cpu_armv8{
            SOURCES += \
                ../../src/armv5te/translate_aarch64.cpp
        }
    }

    cpu_x86_32{
        SOURCES += \
            ../../src/armv5te/asmcode_x86.S
    }
    else{
        # x86 has this implemented in asmcode_x86.S
        SOURCES += \
            ../../src/armv5te/asmcode.c
    }

    cpu_x86_64{
        SOURCES += \
            ../../src/armv5te/asmcode_x86_64.S
    }

    cpu_armv7{
        SOURCES += \
            ../../src/armv5te/asmcode_arm.S
    }

    cpu_armv8{
        SOURCES += \
            ../../src/armv5te/asmcode_aarch64.S
    }

    windows{
        SOURCES += \
            ../../src/armv5te/os/os-win32.c
    }

    macx|linux-g++|android{
        SOURCES += \
            ../../src/armv5te/os/os-linux.c
    }

    SOURCES += \
        ../../src/pxa255/pxa255_mem.c \
        ../../src/pxa255/pxa255_DMA.c \
        ../../src/pxa255/pxa255_DSP.c \
        ../../src/pxa255/pxa255_GPIO.c \
        ../../src/pxa255/pxa255_IC.c \
        ../../src/pxa255/pxa255_LCD.c \
        ../../src/pxa255/pxa255_PwrClk.c \
        ../../src/pxa255/pxa255_RTC.c \
        ../../src/pxa255/pxa255_TIMR.c \
        ../../src/pxa255/pxa255_UART.c \
        ../../src/pxa255/pxa255.c \
        ../../src/armv5te/arm_interpreter.cpp \
        ../../src/armv5te/cpu.cpp \
        ../../src/armv5te/coproc.cpp \
        ../../src/armv5te/emuVarPool.c \
        ../../src/armv5te/thumb_interpreter.cpp \
        ../../src/armv5te/mem.c \
        ../../src/armv5te/mmu.c \
        ../../src/tungstenCBus.c

    HEADERS += \
        ../../src/pxa255/pxa255_CPU.h \
        ../../src/pxa255/pxa255_mem.h \
        ../../src/pxa255/pxa255_DMA.h \
        ../../src/pxa255/pxa255_DSP.h \
        ../../src/pxa255/pxa255_GPIO.h \
        ../../src/pxa255/pxa255_IC.h \
        ../../src/pxa255/pxa255_LCD.h \
        ../../src/pxa255/pxa255_PwrClk.h \
        ../../src/pxa255/pxa255_RTC.h \
        ../../src/pxa255/pxa255_TIMR.h \
        ../../src/pxa255/pxa255_UART.h \
        ../../src/pxa255/pxa255_types.h \
        ../../src/pxa255/pxa255_math64.h \
        ../../src/pxa255/pxa255.h \
        ../../src/armv5te/os/os.h \
        ../../src/armv5te/asmcode.h \
        ../../src/armv5te/bitfield.h \
        ../../src/armv5te/cpu.h \
        ../../src/armv5te/emu.h \
        ../../src/armv5te/mem.h \
        ../../src/armv5te/translate.h \
        ../../src/armv5te/cpudefs.h \
        ../../src/armv5te/debug.h \
        ../../src/armv5te/mmu.h \
        ../../src/armv5te/armsnippets.h \
        ../../src/armv5te/literalpool.h \
        ../../src/tungstenCBus.h
}

CONFIG += c++11

INCLUDEPATH += $$PWD/qt-common/include

SOURCES += \
    ../../src/dbvz.c \
    debugviewer.cpp \
    emuwrapper.cpp \
    main.cpp \
    mainwindow.cpp \
    statemanager.cpp \
    touchscreen.cpp \
    settingsmanager.cpp \
    ../../src/audio/blip_buf.c \
    ../../src/debug/sandbox.c \
    ../../src/ads7846.c \
    ../../src/emulator.c \
    ../../src/flx68000.c \
    ../../src/pdiUsbD12.c \
    ../../src/sdCard.c \
    ../../src/sed1376.c \
    ../../src/silkscreen.c \
    ../../src/m68k/m68kcpu.c \
    ../../src/m68k/m68kdasm.c \
    ../../src/m68k/m68kopac.c \
    ../../src/m68k/m68kopdm.c \
    ../../src/m68k/m68kopnz.c \
    ../../src/m68k/m68kops.c \
    ../../src/expansionHardware.c \
    ../../src/m515Bus.c

HEADERS += \
    ../../src/dbvz.h \
    ../../src/pxa255/pxa255Accessors.c.h \
    debugviewer.h \
    emuwrapper.h \
    mainwindow.h \
    statemanager.h \
    touchscreen.h \
    settingsmanager.h \
    ../../src/audio/blip_buf.h \
    ../../src/debug/sandbox.h \
    ../../src/debug/sandboxTrapNumToName.c.h \
    ../../src/debug/trapNames.h \
    ../../src/m68k/m68k.h \
    ../../src/m68k/m68kconf.h \
    ../../src/m68k/m68kcpu.h \
    ../../src/m68k/m68kexternal.h \
    ../../src/m68k/m68kops.h \
    ../../src/specs/dragonballVzRegisterSpec.h \
    ../../src/ads7846.h \
    ../../src/emulator.h \
    ../../src/flx68000.h \
    ../../src/pdiUsbD12.h \
    ../../src/portability.h \
    ../../src/sdCard.h \
    ../../src/sed1376.h \
    ../../src/sed1376Accessors.c.h \
    ../../src/silkscreen.h \
    ../../src/specs/sed1376RegisterSpec.h \
    ../../src/specs/pdiUsbD12CommandSpec.h \
    ../../src/specs/emuFeatureRegisterSpec.h \
    ../../src/specs/sdCardCommandSpec.h \
    ../../src/expansionHardware.h \
    ../../src/sdCardAccessors.c.h \
    ../../src/sdCardCrcTables.c.h \
    ../../src/m515Bus.h \
    ../../src/dbvzRegisterAccessors.c.h \
    ../../src/dbvzTiming.c.h \

FORMS += \
    mainwindow.ui \
    debugviewer.ui \
    statemanager.ui \
    settingsmanager.ui

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
    images/settingsManager.svg \
    images/stateManager.svg \
    images/stop.svg \
    images/todo.svg \
    images/up.svg

RESOURCES += \
    mainwindow.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
