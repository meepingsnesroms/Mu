#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="TstSuite"

CFLAGS="-palmos4 -O3"

if [ "$1" = "debug" ]; then
   CFLAGS="$CFLAGS -DDEBUG -g"
fi

# PRC config
m68k-palmos-multigen $APPNAME.def
m68k-palmos-gcc $CFLAGS -c $APPNAME-sections.s -o $APPNAME-sections.o

m68k-palmos-gcc $CFLAGS -c *.c -o $I.o

#68K compiling
declare -a M68K_C_FILES=($(ls -d *.c))
for I in "${M68K_C_FILES[@]}"; do
   m68k-palmos-gcc $CFLAGS -c $I -o $I.o
done
m68k-palmos-gcc -o $APPNAME *.o $APPNAME-sections.ld
rm -rf *.o *.a $APPNAME-sections.s $APPNAME-sections.ld

#ARM compiling
cd ./armSideCode
declare -a ARM_C_FILES=($(ls -d *.c))
declare -a ARM_ASM_FILES=($(ls -d *.S))
for I in "${ARM_C_FILES[@]}"; do
   arm-palmos-gcc -fPIC $CFLAGS -c $I -o $I.o
done
for I in "${ARM_ASM_FILES[@]}"; do
   arm-palmos-gcc -fPIC $CFLAGS -c $I -o $I.o
done
arm-palmos-gcc -fPIC -nostartfiles *.o -o ../armc0000.bin
rm -rf *.o *.a
cd ../


# if possible generate icon trees
if type "MakePalmBitmap" &> /dev/null; then
   MakePalmBitmap icon ./appIcon.svg ./
fi

pilrc $APPNAME.rcp
build-prc $APPNAME.def $APPNAME *.bin
rm -rf $APPNAME *.bin
