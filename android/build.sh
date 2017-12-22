#!/bin/sh
ndk-build -j2 $1 && ant debug
#ndk-build -j2 APP_ABI="armeabi-v7a-hard" && ant debug
