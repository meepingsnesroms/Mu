#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="MuExpDriver"

if [ "$1" = "clean" ]; then
   rm -rf *.o *.a $APPNAME $APPNAME.prc *.bin
   exit
fi

declare -a FILES=("muExpDriver" "patcher" "hires" "armv5" "soundDriver" "globals" "gui" "traps" "debug" "memalign")
CFLAGS="-palmos4 -O3"

if [ "$1" = "debug" ]; then
   CFLAGS="$CFLAGS -DDEBUG -g"
fi

for I in "${FILES[@]}"; do
   m68k-palmos-gcc $CFLAGS -c $I.c -o $I.o
done
m68k-palmos-gcc -o $APPNAME *.o

# if possible generate icon trees
if type "MakePalmBitmap" &> /dev/null; then
   MakePalmBitmap icon ./appIcon.svg ./
fi

pilrc $APPNAME.rcp
build-prc $APPNAME.def $APPNAME *.bin
