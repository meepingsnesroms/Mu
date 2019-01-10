#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="MuExpDriver"

if [ "$1" = "clean" ]; then
   rm -rf *.o *.a $APPNAME ./armExit ./armCall68k $APPNAME.prc *.bin
   exit
fi

declare -a FILES=("muExpDriver" "armv5" "soundDriver" "globals" "traps" "debug")
CFLAGS="-palmos4 -O3"

if [ "$1" = "debug" ]; then
   CFLAGS="$CFLAGS -DDEBUG -g"
fi

# -O0 and -g need to be set to prevent register writes from being removed from compiled code
arm-palmos-gcc -palmos-none -c ./armSrc/armExit.c -o ./armSrc/armExit.o
arm-palmos-gcc -palmos-none -c ./armSrc/armCall68k.c -o ./armSrc/armCall68k.o
arm-palmos-gcc -nostartfiles -o ./armExit ./armSrc/armExit.o
arm-palmos-gcc -nostartfiles -o ./armCall68k ./armSrc/armCall68k.o
for I in "${FILES[@]}"; do
   m68k-palmos-gcc $CFLAGS -c $I.c -o $I.o
done
m68k-palmos-gcc -o $APPNAME *.o

# if possible generate icon trees
if type "MakePalmIcon" &> /dev/null; then
   MakePalmIcon ./appIcon.svg ./
fi

pilrc $APPNAME.rcp
build-prc $APPNAME.def $APPNAME ./armExit ./armCall68k *.bin
