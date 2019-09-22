#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

APPNAME="TstSuite"

M68K_CFLAGS="-palmos4 -O3"
ARM_CFLAGS="-flto -march=armv5t -Os -g -ggdb3 -I. -ffunction-sections -fdata-sections -ffixed-r9 -fpic -Wno-multichar -Wall"
ARM_LDFLAGS="-flto -Os -g -ggdb3 -Wl,--gc-sections -Wl,-T armLib.lkr -march=armv5t -fpic -ffixed-r9"

if [ "$1" = "debug" ]; then
   M68K_CFLAGS="$M68K_CFLAGS -DDEBUG -g"
fi

# PRC config
m68k-palmos-multigen $APPNAME.def
m68k-palmos-gcc $M68K_CFLAGS -c $APPNAME-sections.s -o $APPNAME-sections.o

#68K compiling
declare -a M68K_C_FILES=($(ls -d *.c))
for I in "${M68K_C_FILES[@]}"; do
   m68k-palmos-gcc $M68K_CFLAGS -c $I -o $I.o
done
m68k-palmos-gcc -o $APPNAME *.o $APPNAME-sections.ld
rm -rf *.o *.a $APPNAME-sections.s $APPNAME-sections.ld

#ARM compiling
cd ./armSideCode
declare -a ARM_C_FILES=($(ls -d *.c))
declare -a ARM_ASM_FILES=($(ls -d *.S))
for I in "${ARM_C_FILES[@]}"; do
   arm-none-eabi-gcc $ARM_CFLAGS -c $I -o $I.o
done
for I in "${ARM_ASM_FILES[@]}"; do
   arm-none-eabi-gcc $ARM_CFLAGS -c $I -o $I.o
done
arm-none-eabi-gcc -o ../armCode $ARM_LDFLAGS *.o
arm-none-eabi-objcopy -I elf32-littlearm -O binary ../armCode armc0000.bin -j.vec -j.text -j.rodata -j.data
rm -rf *.o *.a
cd ../


# if possible generate icon trees
if type "MakePalmBitmap" &> /dev/null; then
   MakePalmBitmap icon ./appIcon.svg ./
fi

pilrc $APPNAME.rcp
build-prc $APPNAME.def $APPNAME armCode *.bin
# rm -rf $APPNAME armCode *.bin
