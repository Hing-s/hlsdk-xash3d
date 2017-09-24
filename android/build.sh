#!/bin/sh
export ANDROID_HOME=$PWD/../../android-sdk-linux
export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:$PWD/android-ndk
#ndk-build -j1
ndk-build -j1 APP_ABI="armeabi-v7a-hard"
ant debug
#jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ../myks.keystore bin/mod-unsigned.apk xashdroid -tsa https://timestamp.geotrust.com/tsa
#zipalign 4 bin/cs16-client-unsigned.apk bin/mod.apk
