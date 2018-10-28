#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR/jni

ndk-build clean
ndk-build
adb push ../libs/armeabi-v7a/libretro.so /data/local/tmp/mu_libretro_android.so
adb shell su -c \"mv -f /data/local/tmp/mu_libretro_android.so /data/data/com.retroarch/cores\"
adb shell su -c \"restorecon -R /data/data/com.retroarch\"
