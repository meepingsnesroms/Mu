#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="TstSuite"
ICONTEXT="HWTests"
APPID="TstS"
CREATORID="guiC"
RESFILE=$APPNAME.rcp
PRC=$APPNAME.prc

SRCS="testSuite.c viewer.c tools.c cpu.c emuFunctions.c debug.c ugui.c"
DEFINES="-DHW_TEST -DDEBUG"
CFLAGS="-O2 -g $DEFINES"

if [ "$1" = "clean" ]; then
   rm -rf *.o *.a $APPNAME $PRC *.bin *.grc
   exit
fi

m68k-palmos-gcc -palmos3.5 $CFLAGS $SRCS -o $APPNAME
m68k-palmos-obj-res $APPNAME
pilrc $RESFILE
build-prc $PRC $ICONTEXT $CREATORID *.grc *.bin
