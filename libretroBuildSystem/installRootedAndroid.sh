#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR/jni

if [ -z $1 ]; then
   echo "You must pick an architecture: armeabi, armeabi-v7a, arm64-v8a, x86, x86_64, mips, mips64"
   exit
fi

ndk-build APP_ABI=$1 clean
ndk-build APP_ABI=$1
adb push ../libs/$1/libretro.so /data/local/tmp/mu_libretro_android.so
adb push ../mu_libretro.info /data/local/tmp/mu_libretro.info
adb shell su -c \"mv -f /data/local/tmp/mu_libretro_android.so /data/data/com.retroarch/cores\"
adb shell su -c \"mv -f /data/local/tmp/mu_libretro.info /data/data/com.retroarch/info\"
adb shell su -c \"restorecon -R /data/data/com.retroarch\"
