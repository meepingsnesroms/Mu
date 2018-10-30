#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR/jni

ndk-build APP_ABI=armeabi-v7a clean 
ndk-build APP_ABI=armeabi-v7a
adb push ../libs/armeabi-v7a/libretro.so /data/local/tmp/mu_libretro_android.so
adb shell su -c \"mv -f /data/local/tmp/mu_libretro_android.so /data/data/com.retroarch/cores\"
adb shell su -c \"restorecon -R /data/data/com.retroarch\"
