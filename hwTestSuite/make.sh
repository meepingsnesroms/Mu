#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="TstSuite"
ICONTEXT="HWTests"
APPID="TstS"
CREATORID="guiC"
RESFILE=$APPNAME.rcp
PRC=$APPNAME.prc

if [ "$1" = "clean" ]; then
   rm -rf *.o *.a $APPNAME $PRC *.bin *.grc
   exit
fi

SRCS="testSuite.c viewer.c tools.c tests.c cpu.c emuFunctions.c ugui.c"
DEFINES="-DHW_TEST"

if [ "$1" = "debug" ]; then
   DEFINES="$DEFINES -DDEBUG"
   SRCS="$SRCS debug.c"
fi

CFLAGS="-O2 -g $DEFINES"

m68k-palmos-gcc -palmos3.5 $CFLAGS $SRCS -o $APPNAME
m68k-palmos-obj-res $APPNAME
pilrc $RESFILE
build-prc $PRC $ICONTEXT $CREATORID *.grc *.bin
