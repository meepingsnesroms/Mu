#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="TstSuite"
ICONTEXT="HWTests"
APPID="TstS"
CREATORID="GuiC"
RESFILE=$APPNAME.rcp
PRC=$APPNAME.prc

if [ "$1" = "clean" ]; then
   rm -rf *.o *.a $APPNAME $PRC *.bin *.grc
   exit
fi

declare -a FILES=("testSuite" "viewer" "tools" "tests" "cpu" "irda" "emuFunctions" "ugui")
# SRCS="testSuite.c viewer.c tools.c tests.c cpu.c irda.c emuFunctions.c ugui.c"
DEFINES="-DHW_TEST"
CFLAGS="-O3 $DEFINES"

if [ "$1" = "debug" ]; then
   DEFINES="$DEFINES -DDEBUG"
#SRCS="$SRCS debug.c"
   CFLAGS="$CFLAGS -g"
fi

m68k-palmos-multigen $APPNAME.def
for I in "${FILES[@]}"; do
   m68k-palmos-gcc -palmos3.5 $CFLAGS -c $I.c -o $I.o
done
m68k-palmos-gcc -o $APPNAME *.o $APPNAME-sections.ld

# m68k-palmos-gcc -O3 $SRCS -o $APPNAME $APPNAME-sections.ld
# m68k-palmos-obj-res $APPNAME
pilrc $RESFILE
build-prc $PRC $APPNAME.def $APPNAME *.bin
# $ICONTEXT $CREATORID
# build-prc $PRC $ICONTEXT $CREATORID *.grc *.bin
